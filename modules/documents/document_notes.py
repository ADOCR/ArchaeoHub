"""Genera Markdown portable desde datos verificados; el texto del modelo no puede crear enlaces."""
from collections import defaultdict
from pathlib import Path
import re
import html
from document_analysis import KINDS, RELATIONS


def plain(text):
    return re.sub(r"[\[\]`<>#]", "", str(text)).replace("\n", " ").replace("\r", " ")


def quoted(text):
    return html.escape(text,quote=False).replace("[","&#91;").replace("]","&#93;").replace("`","&#96;")


def note_link(prefix, path, caption):
    return "[["+prefix+path+"|"+plain(caption)+"]]"


def render(staging, plan, prefix, links=True, cancelled=lambda:None):
    from document_library import safe_name, label
    analysis = plan["analysis"]
    items = {item["id"]:item for item in plan["items"]}
    documents = {document["id"]:document for document in analysis["documents"]}
    concepts = {c["id"]:c for document in documents.values() for c in document["concepts"]} if links else {}
    relations = analysis["relations"] if links else []
    fiches = {i:"Fichas/"+safe_name(item["title"])+"--"+i for i,item in items.items()}
    concept_paths = {i:"Conceptos/"+safe_name(c["name"])+"--"+i for i,c in concepts.items()}
    relation_paths = {r["id"]:"Conexiones/"+r["id"] for r in relations}
    evidence = {}
    for document in documents.values():
        for entry in document["summary"]+document["concepts"]:
            for value in entry["evidence"]: evidence[value["id"]]=value
    for relation in relations:
        for value in relation["evidence"]: evidence[value["id"]]=value
    by_document = defaultdict(list)
    for value in evidence.values(): by_document[value["document"]].append(value)
    for folder in ("Fichas", "Evidencias", "Conceptos", "Conexiones"):
        (staging/folder).mkdir()

    def references(values):
        return " ".join(note_link(prefix,"Evidencias/"+e["document"]+"#^"+e["id"],
                                  items[e["document"]]["title"]+" · "+e["locator"]) for e in values)

    disclaimer = ("Síntesis e interpretaciones propuestas por el modelo local; requieren revisión. "
                  "Las citas se cotejaron con el texto extraído, lo que no demuestra por sí solo que la interpretación sea correcta.\n\n")
    for document_id, item in items.items():
        cancelled()
        document = documents[document_id]
        text = "# Ficha · "+label(item["title"])+"\n\n"+disclaimer
        text += note_link(prefix,item["note"],"Texto completo convertido")+" · [Original]("+Path(item["path"]).as_uri()+")\n\n"
        text += f"Modelo: {plain(analysis['model']['name'])}. Lectura: {document['passages']} secciones, {document['charactersRead']} caracteres.\n\n"
        if document["status"]!="analyzed":
            text += "## Pendiente de revisión\n\nNo se obtuvieron afirmaciones con evidencia suficiente para redactar una ficha. No se sustituyeron por un resumen inventado.\n\n"
        for kind, title in KINDS.items():
            claims = [c for c in document["summary"] if c["kind"]==kind]
            if not claims: continue
            text += "## "+title+"\n\n"
            text += "\n\n".join(plain(c["text"])+" "+references(c["evidence"]) for c in claims)+"\n\n"
        absent = [title for kind,title in KINDS.items() if not any(c["kind"]==kind for c in document["summary"])]
        if absent: text += "## Aspectos no recogidos en la ficha\n\n"+", ".join(absent)+". Su ausencia en la ficha no demuestra que falten en el original.\n\n"
        own = [c for c in document["concepts"] if c["id"] in concepts]
        if own:
            text += "## Conceptos contextualizados\n\n"+"\n".join("- "+note_link(prefix,concept_paths[c["id"]],c["name"])+": "+plain(c["definition"]) for c in own)+"\n\n"
        associated = [r for r in relations if any(e["document"]==document_id for e in r["evidence"])]
        if associated:
            text += "## Conexiones explicadas\n\n"+"\n".join("- "+note_link(prefix,relation_paths[r["id"]],concepts[r["from"]]["name"]+" → "+concepts[r["to"]]["name"])+": "+plain(r["explanation"]) for r in associated)+"\n"
        if item["warnings"]:
            text += "\n## Alcance de extracción\n\n"+"\n".join("- "+plain(w) for w in item["warnings"])+"\n"
        (staging/(fiches[document_id]+".md")).write_text(text,encoding="utf-8")
        evidence_text = "# Evidencias · "+label(item["title"])+"\n\nCitas cotejadas con la extracción; revisa el original para contexto, tablas y diseño de página.\n\n"
        for value in by_document[document_id]:
            uri = Path(item["path"]).as_uri()
            page = re.match(r"p\. (\d+)",value["locator"])
            if page: uri += "#page="+page[1]
            evidence_text += "## "+plain(value["locator"])+"\n\n> "+quoted(value["quote"])+"\n\n[Ver original]("+uri+") ^"+value["id"]+"\n\n"
        (staging/"Evidencias"/(document_id+".md")).write_text(evidence_text,encoding="utf-8")
    for concept_id, concept in concepts.items():
        cancelled()
        text = "# "+plain(concept["name"])+"\n\n"+disclaimer
        text += "## Significado en esta fuente\n\n"+plain(concept["definition"])+" "+references(concept["evidence"])+"\n\n"
        for variant in concept.get("otherDefinitions",[]):
            text += "- "+plain(variant["definition"])+" "+references(variant["evidence"])+"\n"
        text += "\nEl concepto conserva la identidad de su documento. Un término igual en otro autor no se considera automáticamente equivalente.\n\n"
        text += "## Relaciones propuestas\n\n"
        attached = [r for r in relations if concept_id in (r["from"],r["to"])]
        text += "\n".join("- "+note_link(prefix,relation_paths[r["id"]],concepts[r["from"]]["name"]+" "+RELATIONS[r["type"]]+" "+concepts[r["to"]]["name"]) for r in attached) or "No se generó una relación sustentada para este concepto."
        (staging/(concept_paths[concept_id]+".md")).write_text(text+"\n",encoding="utf-8")
    for relation in relations:
        cancelled()
        title = concepts[relation["from"]]["name"]+" "+RELATIONS[relation["type"]]+" "+concepts[relation["to"]]["name"]
        text = "# "+plain(title)+"\n\n"+disclaimer
        text += "**Estado:** propuesta pendiente de revisión. **Tipo:** "+RELATIONS[relation["type"]]+".\n\n"
        text += note_link(prefix,concept_paths[relation["from"]],concepts[relation["from"]]["name"])+" → "+note_link(prefix,concept_paths[relation["to"]],concepts[relation["to"]]["name"])+"\n\n"
        text += "## Por qué se relacionan\n\n"+plain(relation["explanation"])+"\n\n## Evidencia y contexto\n\n"
        for e in relation["evidence"]:
            text += "- "+plain(items[e["document"]]["title"])+" · "+plain(e["locator"])+": «"+quoted(e["quote"])+"» "+references([e])+"\n"
        text += "\nNo implica identidad de sitios, cronologías, influencia entre autores ni causalidad salvo que la fuente lo sostenga explícitamente.\n"
        (staging/(relation_paths[relation["id"]]+".md")).write_text(text,encoding="utf-8")
    index = "\n\n## Fichas de lectura\n\n"+"\n".join("- "+note_link(prefix,path,items[i]["title"]) for i,path in fiches.items())
    index += "\n\n## Conceptos\n\n"+"\n".join("- "+note_link(prefix,path,concepts[i]["name"]) for i,path in concept_paths.items())
    index += "\n\n## Conexiones explicadas\n\n"+"\n".join("- "+note_link(prefix,relation_paths[r["id"]],concepts[r["from"]]["name"]+" → "+concepts[r["to"]]["name"]) for r in relations)
    compared = analysis["comparisons"]
    index += f"\n\nSe contrastaron {compared['comparedPairs']} pares de {compared['candidatePairs']} candidatos. Cada comparación usa hasta 10 conceptos por documento; no es una revisión exhaustiva de todas las relaciones posibles.\n"
    return {"index":index, "fiches":fiches, "conceptNotes":concept_paths, "relationNotes":relation_paths,
            "evidenceCount":len(evidence), "analysis":analysis}
