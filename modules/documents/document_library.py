"""Lotes documentales locales y notas compatibles con Obsidian, sin descargar recursos enlazados."""
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import math
import os
import re
import shutil
import stat
import unicodedata
import uuid
from urllib.parse import quote, unquote, urlsplit
from document_worker import extract

MAX_FILES = 1000
MAX_TOTAL_TEXT = 8000000
SUPPORTED = {".pdf", ".md", ".markdown", ".docx"}
STOP = set("""a al algo alguna algunas alguno algunos alla alli ambos ante antes aquel aquella aquellas aquellos aqui asi aun aunque bajo bien cada casi como con contra cual cuales cuando de del dentro desde donde dos durante e el ella ellas ello ellos en entre era eran es esa esas ese esos esta estaba estaban estamos estan estar estas este esto estos fue fueron ha haber habia hacia han hasta hay la las le les lo los mas me mi mientras mismo mucha muchas mucho muchos muy ni no nos nuestra nuestro nuevas nuevos o otra otras otro otros para parte pero poco por porque primera primero puede pueden que quien quienes se sea segun ser si sido sin sobre son su sus tal tambien tanto te tener tiene tienen toda todas todo todos tras tres tu un una unas uno unos usar va vez ya y yo the a an and are as at be been being by can could did do does each for from had has have how if in into is it its may more most new no not of on one only or other our out same should so some such than that their them then there these they this those through to two under up use used using very was we were what when where which while who will with would you your texto pagina paginas parrafo documento documentos archivo archivos nombre titulo title source tags campo original informacion informe ejemplo general presente trabajo datos resultados figura tabla chapter page pages document documents file files name text paragraph section http https www md pdf docx""".split())


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def reparse(path):
    info = path.lstat()
    return path.is_symlink() or bool(getattr(info, "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0))


def normalized(text):
    return "".join(c for c in unicodedata.normalize("NFKD", text.casefold()) if not unicodedata.combining(c))


def safe_name(text, limit=75):
    text = re.sub(r'[<>:"/\\|?*#^%\[\]\x00-\x1f]', " ", str(text))
    text = re.sub(r"\s+", " ", text).strip(" .")[:limit].rstrip(" .") or "Documento"
    if text.split(".")[0].upper() in {"CON", "PRN", "AUX", "NUL", *(f"COM{i}" for i in range(1, 10)), *(f"LPT{i}" for i in range(1, 10))}:
        text = "Nota " + text
    return text


def label(text):
    return re.sub(r"[\[\]|\r\n]", " ", text).strip()


def collect(paths, formats=None):
    if not isinstance(paths, list) or not paths or len(paths) > MAX_FILES:
        raise ValueError("Selecciona archivos o carpetas; máximo 1.000 entradas por lote.")
    files, roots, seen = [], [], set()
    def add(path):
        if formats is not None and path.suffix.lower() not in formats: return
        if reparse(path):
            return
        resolved = path.resolve()
        if resolved not in seen:
            seen.add(resolved); files.append(resolved)
            if len(files) > MAX_FILES:
                raise ValueError("La selección contiene más de 1.000 documentos. Divide el lote.")
    scanned = 0
    for value in paths:
        path = Path(value).absolute()
        if not path.exists() or reparse(path):
            raise ValueError(f"La ubicación no está disponible o es un enlace: {path.name}")
        if path.is_file():
            add(path)
        else:
            roots.append(str(path.resolve()))
            def walk_error(error):
                raise ValueError(f"No se pudo revisar una carpeta: {error}")
            for current, folders, names in os.walk(path, followlinks=False, onerror=walk_error):
                folders[:] = sorted(d for d in folders if not d.startswith(".") and not reparse(Path(current)/d))
                if len(Path(current).relative_to(path).parts) > 16:
                    raise ValueError("La carpeta tiene más de 16 niveles. Selecciona una subcarpeta.")
                scanned += len(names)+len(folders)
                if scanned > 100000:
                    raise ValueError("La búsqueda excede 100.000 entradas. Selecciona una carpeta más concreta.")
                for name in sorted(names):
                    if Path(name).suffix.lower() in SUPPORTED:
                        add(Path(current)/name)
    return sorted(files, key=lambda p: str(p).casefold()), roots


def vocabulary(text):
    text = re.sub(r"```.*?```|~~~.*?~~~|`[^`\n]*`", " ", text, flags=re.S)
    text = re.sub(r"https?://\S+|file://\S+", " ", text)
    words = [word for word in re.findall(r"\b[^\W\d_]{4,32}\b", normalized(text)) if word not in STOP]
    return Counter(words)


def shared_topics(items):
    frequencies, counts = Counter(), {}
    for item in items:
        counts[item["id"]] = vocabulary("\n".join(b["text"] for b in item["blocks"]))
        frequencies.update(counts[item["id"]].keys())
    totals = Counter()
    for count in counts.values(): totals.update(count)
    candidates = [word for word, count in frequencies.items() if count >= 2]
    candidates.sort(key=lambda word: (-(1+math.log(totals[word]))*math.log(1+len(items)/frequencies[word]), word))
    topics = []
    used = Counter()
    for word in candidates[:500]:
        documents = [item["id"] for item in items if counts[item["id"]][word] and used[item["id"]] < 12]
        if len(documents) < 2: continue
        topic_id = hashlib.sha256(word.encode()).hexdigest()[:10]
        topics.append({"id": topic_id, "name": word, "note": f"Temas/{safe_name(word)}--{topic_id}", "documents": documents})
        used.update(documents)
        if len(topics) == 80: break
    return topics


def analyze(request, progress=lambda **_: None):
    formats=None
    if "formats" in request:
        formats=request["formats"]
        if not isinstance(formats,list) or not formats or not all(isinstance(f,str) and f in SUPPORTED for f in formats):
            raise ValueError("La selección de formatos no es compatible.")
    files, roots = collect(request.get("paths"),formats)
    if not files: raise ValueError("No se encontraron PDF, Markdown o DOCX en las carpetas seleccionadas.")
    items, rows, characters = [], [], 0
    plan_path = Path(request["planFile"]).absolute()
    for index, path in enumerate(files):
        progress(progress=(index/max(1, len(files)))*.9, message=f"Leyendo {index+1} de {len(files)}: {path.name}")
        row = {"name": path.name, "path": str(path), "status": "error", "message": "", "characters": 0}
        try:
            before = path.stat(); source_hash = digest(path); doc = extract(path, max_characters=2000000)
            if digest(path) != source_hash or path.stat().st_mtime_ns != before.st_mtime_ns:
                raise ValueError("El archivo cambió mientras se leía. Repite el análisis.")
            if characters+doc["characters"] > MAX_TOTAL_TEXT:
                raise ValueError("El lote excede 8 millones de caracteres. Divide la selección; no se truncó.")
            item_id = hashlib.sha256((str(path)+"\0"+source_hash).encode()).hexdigest()[:16]
            item = {**doc, "id": item_id, "sha256": source_hash, "format": path.suffix.lower(), "title": path.stem,
                    "note": f"Documentos/{safe_name(path.stem)}--{item_id}"}
            items.append(item); characters += doc["characters"]
            row.update(id=item_id, status="ready", characters=doc["characters"], message=" · ".join(doc["warnings"][:3])+(f" · {len(doc['warnings'])} avisos en total" if len(doc["warnings"])>3 else ""))
        except Exception as error:
            row["message"] = str(error)
            if "OCR" in str(error): row["status"] = "needs-ocr"
        rows.append(row)
    topics = shared_topics(items) if request.get("topicCandidates",True) else []
    plan = {"schemaVersion": 1, "items": items, "rows": rows, "topics": topics, "roots": roots,
            "createdAtUtc": datetime.now(timezone.utc).isoformat(), "topicMethod": "shared-normalized-vocabulary-v1",
            "purpose":"conversion" if request.get("conversionOnly") else "library"}
    data = json.dumps(plan, ensure_ascii=False).encode()
    if len(data) > 64*1024*1024: raise ValueError("El plan excede el tamaño admitido. Divide el lote.")
    if not plan_path.parent.is_dir() or reparse(plan_path.parent): raise ValueError("No está disponible la caché del lote.")
    with plan_path.open("xb") as stream: stream.write(data)
    progress(progress=1, message="Análisis terminado.")
    return {"rows": rows, "topics": [{"name": t["name"], "documents": len(t["documents"])} for t in topics],
            "ready": len(items), "failed": len(rows)-len(items), "characters": characters,
            "planDigest": hashlib.sha256(data).hexdigest(), "planFile": str(plan_path)}


def rewrite_links(text, item, items, target_prefix=""):
    """Resuelve destinos locales inequívocos; conserva e informa las referencias sin resolver."""
    original = Path(item["path"]); by_path = {Path(i["path"]): i for i in items}
    by_stem = defaultdict(list)
    for candidate in items: by_stem[normalized(Path(candidate["path"]).stem)].append(candidate)
    warnings = []
    def wiki(match):
        embed, target, alias = match.group(1), match.group(2), match.group(3)
        location, marker, heading = target.partition("#")
        if not location: return match.group(0)
        if location.startswith(("//", "\\\\")) or re.match(r"^[A-Za-z][A-Za-z0-9+.-]*:",location): return match.group(0)
        path = Path(os.path.abspath(original.parent/location))
        candidate = by_path.get(path) or by_path.get(Path(str(path)+".md"))
        if not candidate and "/" not in location and "\\" not in location:
            options = by_stem[normalized(location[:-3] if location.lower().endswith(".md") else location)]
            if len(options) == 1: candidate = options[0]
        if not candidate:
            suffix = "/"+location.replace("\\", "/").lstrip("/")
            if not suffix.lower().endswith(".md"): suffix += ".md"
            options = [i for i in items if os.path.normcase(Path(i["path"]).as_posix()).endswith(os.path.normcase(suffix))]
            if len(options) == 1: candidate = options[0]
        if candidate:
            suffix = marker+heading if marker else ""
            return f"{embed}[[{target_prefix}{candidate['note']}{suffix}|{label(alias or location)}]]"
        warnings.append(f"Enlace wiki sin resolver: {target}")
        return match.group(0)
    def markdown(match):
        title, location = match.groups()
        try: parts = urlsplit(location)
        except ValueError: return match.group(0)
        if parts.scheme or parts.netloc or location.startswith(("#", "\\\\")) or '"' in location: return match.group(0)
        path = Path(os.path.abspath(original.parent/unquote(parts.path)))
        candidate = by_path.get(path)
        if candidate:
            return title+"("+quote(Path(candidate["note"]).name+".md")+("#"+parts.fragment if parts.fragment else "")+")"
        warnings.append(f"Recurso externo referenciado sin copiar ni abrir: {location}")
        return title+"("+path.as_uri()+("#"+parts.fragment if parts.fragment else "")+")"
    parts = re.split(r"(```.*?```|~~~.*?~~~|`[^`\n]*`)", text, flags=re.S)
    for index in range(0, len(parts), 2):
        value = re.sub(r"(!?)\[\[([^\]|\r\n]+)(?:\|([^\]\r\n]+))?\]\]", wiki, parts[index])
        parts[index] = re.sub(r"(!?\[[^\]\r\n]*\])\(([^()\r\n]+)\)", markdown, value)
    return "".join(parts), sorted(set(warnings))


def export(request, progress=lambda **_: None):
    plan_path = Path(request["planFile"])
    if not plan_path.is_file() or reparse(plan_path) or plan_path.stat().st_size > 64*1024*1024: raise ValueError("El lote analizado ya no está disponible.")
    data = plan_path.read_bytes()
    if hashlib.sha256(data).hexdigest() != request["planDigest"]: raise ValueError("El plan cambió. Analiza de nuevo antes de exportar.")
    plan = json.loads(data); items = plan["items"]
    if not items: raise ValueError("No hay documentos convertibles en este lote.")
    parent = Path(request["destination"]).absolute()
    if not parent.is_dir() or reparse(parent): raise ValueError("Elige una carpeta de destino disponible, sin enlaces.")
    parent = parent.resolve()
    if ".obsidian" in parent.parts:
        raise ValueError("Elige una carpeta de notas fuera de .obsidian; esa carpeta pertenece a la configuración.")
    if any(parent.is_relative_to(Path(root)) for root in plan["roots"]):
        raise ValueError("Elige un destino fuera de las carpetas de origen para evitar importaciones recursivas.")
    cancel = Path(request["cancelFile"]) if request.get("cancelFile") else None
    def cancelled():
        if cancel and cancel.exists(): raise InterruptedError("Exportación cancelada. Los originales se conservaron.")
    for item in items:
        cancelled()
        path = Path(item["path"])
        if not path.is_file() or reparse(path) or digest(path) != item["sha256"]:
            raise ValueError(f"Cambió o no está disponible {item['name']}. Analiza nuevamente; no se publicó el lote.")
    token = uuid.uuid4().hex
    staging = parent/(".archaeohub-library-"+token)
    final = parent/("ArchaeoHub-Documentos-"+datetime.now().strftime("%Y%m%d-%H%M%S")+"-"+token[:8])
    vault = next((folder for folder in (parent, *parent.parents) if (folder/".obsidian").is_dir()), None)
    target_prefix = final.relative_to(vault).as_posix()+"/" if vault else ""
    staging.mkdir(); (staging/".archaeohub-staging").write_text(token)
    try:
        (staging/"Documentos").mkdir(); (staging/"Temas").mkdir()
        research = None
        if plan.get("analysis"):
            from document_notes import render
            research = render(staging, plan, target_prefix, request.get("automaticLinks", True), cancelled)
        topics = plan["topics"] if request.get("automaticLinks", True) else []
        links = defaultdict(list)
        for topic in topics:
            for item_id in topic["documents"]: links[item_id].append(topic)
        converted = []
        for index, item in enumerate(items):
            cancelled(); progress(progress=(index/len(items))*.85, message=f"Convirtiendo {index+1} de {len(items)}: {item['name']}")
            warnings = list(item["warnings"])
            if item["format"] in (".md", ".markdown"):
                content, link_warnings = rewrite_links(item["blocks"][0]["text"], item, items, target_prefix); warnings += link_warnings
            else:
                properties = {"archaeohub_id": item["id"], "title": item["title"], "source": Path(item["path"]).as_uri(), "source_sha256": item["sha256"]}
                content = "---\n"+"\n".join(key+": "+json.dumps(value, ensure_ascii=False) for key, value in properties.items())+"\n---\n\n# "+label(item["title"])+"\n\n"
                content += "\n\n".join("## "+block["locator"]+"\n\n"+block["text"].replace("<", "&lt;").replace(">", "&gt;") for block in item["blocks"])
            content += "\n\n---\n## ArchaeoHub · "+("procedencia" if plan.get("purpose")=="conversion" else "procedencia y conexiones")+"\n\n"
            content += "[Abrir documento original]("+Path(item["path"]).as_uri()+")\n\nSHA-256: `"+item["sha256"]+"`\n"
            if research:
                content += "\nFicha de lectura: [["+target_prefix+research["fiches"][item["id"]]+"|Resumen, conceptos y evidencias]]\n"
            if links[item["id"]]:
                content += "\nTemas compartidos detectados por coincidencia de vocabulario; requieren interpretación:\n\n"
                content += "\n".join("- [["+target_prefix+topic["note"]+"|"+topic["name"]+"]]" for topic in links[item["id"]])+"\n"
            if warnings: content += "\n### Avisos de conversión\n\n"+"\n".join("- "+str(warning).replace("\n", " ") for warning in warnings)+"\n"
            (staging/(item["note"]+".md")).write_text(content, encoding="utf-8")
            converted.append({key: item[key] for key in ("id", "name", "path", "sha256", "note", "characters")}|{"warnings": warnings})
        lookup = {i["id"]: i for i in items}
        for topic in topics:
            cancelled()
            content = "# "+topic["name"]+"\n\nCoincidencia automática de vocabulario. Compartir un término no demuestra una relación histórica o espacial.\n\n"
            content += "\n".join("- [["+target_prefix+lookup[item_id]["note"]+"|"+label(lookup[item_id]["title"])+"]]" for item_id in topic["documents"])+"\n"
            (staging/(topic["note"]+".md")).write_text(content, encoding="utf-8")
        index = "# Biblioteca documental\n\n"+("Lote dentro de una bóveda existente. Ábrelo desde la bóveda principal; los enlaces incluyen la ruta del lote.\n\n" if vault else "Abre esta carpeta como bóveda de Obsidian. No requiere plugins. Si la mueves dentro de otra bóveda, conserva su estructura y revisa los enlaces desde esa bóveda.\n\n")
        index += "Los originales y adjuntos permanecen en su ubicación; no se han copiado ni modificado. Las referencias externas dependen de esas rutas. "+("Las fichas contienen síntesis con evidencias; las interpretaciones necesitan revisión." if research else "El texto convertido no es un resumen.")+"\n\n## Documentos\n\n"
        index += "\n".join("- [["+target_prefix+i["note"]+"|"+label(i["title"])+"]]" for i in items)
        index += "\n\n## Temas compartidos\n\n"+"\n".join("- [["+target_prefix+t["note"]+"|"+t["name"]+"]]" for t in topics)
        if research: index += research["index"]
        failed = [row for row in plan["rows"] if row["status"] != "ready"]
        if failed: index += "\n\n## Archivos pendientes\n\n"+"\n".join("- "+label(row["name"])+": "+row["message"].replace("\n", " ") for row in failed)
        (staging/"Índice.md").write_text(index+"\n", encoding="utf-8")
        manifest = {"schemaVersion": 1, "createdAtUtc": datetime.now(timezone.utc).isoformat(), "topicMethod": plan["topicMethod"], "documents": converted, "topics": topics, "pending": failed, "originalsCopied": False, "linkRoot": str(vault or final), "wikiPrefix": target_prefix}
        if research:
            manifest["research"] = {key: value for key,value in research.items() if key!="index"}
        manifest["files"] = {p.relative_to(staging).as_posix(): digest(p) for p in staging.rglob("*.md")}
        (staging/"archaeohub-library.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")
        cancelled()
        # Publica un lote nuevo completo. Los archivos existentes de la bóveda y la configuración .obsidian se conservan.
        (staging/".archaeohub-staging").unlink(); staging.rename(final)
        progress(progress=1, message="Biblioteca Markdown publicada.")
        return {"folder": str(final), "documents": len(converted), "topics": len(topics), "pending": len(failed), "originalsUnchanged": True}
    except BaseException:
        # Sólo se puede limpiar este directorio temporal privado que acaba de crearse.
        if staging.parent == parent and staging.name == ".archaeohub-library-"+token and staging.is_dir() and not reparse(staging):
            shutil.rmtree(staging)
        raise
