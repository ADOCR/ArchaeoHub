"""Lee PDF/Markdown/DOCX locales como datos, sin ejecutar macros ni descargar enlaces."""
from pathlib import Path
import json
import sys
import zipfile
from xml.etree import ElementTree as ET

MAX_CHARACTERS=2000000
MAX_BATCH_CHARACTERS=4000000

def extract(path, max_characters=MAX_CHARACTERS):
    path=Path(path).resolve()
    if not path.is_file() or path.stat().st_size>32*1024*1024:
        raise ValueError(f"No se puede leer {path.name}: máximo 32 MB por documento.")
    suffix=path.suffix.lower();blocks=[];warnings=[];characters=0
    def append(locator,text):
        nonlocal characters
        if not text.strip():return
        characters+=len(text)
        if characters>max_characters:raise ValueError(f"{path.name}: excede {max_characters:,} caracteres; divide el documento. No se ha truncado.".replace(",", "."))
        blocks.append({"locator":locator,"text":text})
    if suffix in (".md",".markdown"):
        text=path.read_text(encoding="utf-8-sig")
        append("texto",text)
    elif suffix==".docx":
        with zipfile.ZipFile(path) as archive:
            if len(archive.infolist())>4096 or sum(e.file_size for e in archive.infolist())>64*1024*1024:
                raise ValueError(f"{path.name}: el contenido expandido excede el límite.")
            names=["word/document.xml"]+[n for n in sorted(archive.namelist()) if n.startswith(("word/header","word/footer")) and n.endswith(".xml")]
            names += [n for n in ("word/footnotes.xml","word/endnotes.xml") if n in archive.namelist()]
            namespace="{http://schemas.openxmlformats.org/wordprocessingml/2006/main}"
            for name in names:
                tree=ET.fromstring(archive.read(name))
                for number,paragraph in enumerate(tree.iter(namespace+"p"),1):
                    text="".join(node.text or "" for node in paragraph.iter(namespace+"t"))
                    append(f"{Path(name).stem} · párr. {number}",text)
    elif suffix==".pdf":
        from pypdf import PdfReader
        reader=PdfReader(path)
        if reader.is_encrypted:
            raise ValueError(f"{path.name}: el PDF está protegido. Usa una copia desbloqueada que puedas leer.")
        if len(reader.pages)>500:raise ValueError(f"{path.name}: máximo 500 páginas por PDF.")
        for number,page in enumerate(reader.pages,1):
            text=page.extract_text() or ""
            if text.strip():append(f"p. {number}",text)
            else:warnings.append(f"Página {number} sin texto extraíble; puede requerir OCR.")
    else:
        raise ValueError(f"{path.name}: admite PDF, Markdown y Word .docx. Convierte los archivos .doc antiguos a .docx.")
    count=sum(len(block["text"]) for block in blocks)
    if count>max_characters:raise ValueError(f"{path.name}: excede {max_characters:,} caracteres; divide el documento. No se ha truncado.".replace(",", "."))
    if not count:raise ValueError(f"{path.name}: no contiene texto extraíble. Si es un PDF escaneado necesita OCR antes de resumir.")
    return {"name":path.name,"path":str(path),"characters":count,"blocks":blocks,"warnings":warnings}

def dispatch(request):
    if request.get("schemaVersion")!=1:raise ValueError("Protocolo de documentos no compatible.")
    paths=request.get("paths",[])
    if not isinstance(paths,list) or not 1<=len(paths)<=16:raise ValueError("Selecciona entre 1 y 16 documentos.")
    documents=[extract(path) for path in dict.fromkeys(paths)]
    if sum(d["characters"] for d in documents)>MAX_BATCH_CHARACTERS:raise ValueError("El conjunto excede 4.000.000 caracteres. Divide el lote; no se ha truncado.")
    return {"documents":documents}

def main():
    try:
        raw=sys.stdin.buffer.read(1024*1024+1)
        if len(raw)>1024*1024:raise ValueError("Demasiadas rutas en la solicitud.")
        request=json.loads(raw)
        if request.get("op", "").startswith("library."):
            from document_library import analyze, export
            if request.get("schemaVersion")!=1:raise ValueError("Protocolo de biblioteca no compatible.")
            def progress(**values):
                sys.stdout.buffer.write((json.dumps({"type":"progress",**values},ensure_ascii=False)+"\n").encode("utf-8"));sys.stdout.buffer.flush()
            if request["op"]=="library.analyze":result=analyze(request,progress)
            elif request["op"]=="library.export":result=export(request,progress)
            elif request["op"]=="library.enrich":
                from document_analysis import enrich
                result=enrich(request,progress)
            else:raise ValueError("Acción documental no compatible.")
        else:result=dispatch(request)
        reply={"ok":True,**result};code=0
    except Exception as exc:reply={"ok":False,"error":str(exc)};code=1
    sys.stdout.buffer.write((json.dumps(reply,ensure_ascii=False)+"\n").encode("utf-8"));return code

if __name__=="__main__":sys.exit(main())
