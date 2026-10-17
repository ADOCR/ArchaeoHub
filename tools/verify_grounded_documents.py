"""Aceptación con modelo local real: métodos relacionados, homónimo sin relación y evidencia exacta."""
from pathlib import Path
import argparse,hashlib,json,os,re,subprocess,sys,time
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tests"));sys.path.insert(0,str(ROOT/"modules/documents"))
from document_worker_tests import create_pdf,create_docx
from document_library import analyze,export,digest
from document_analysis import enrich

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--workdir",type=Path,required=True)
parser.add_argument("--payload",type=Path)
parser.add_argument("--model",default="qwen3:8b")
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError("Use an empty isolated directory")
source=root/"Fuentes sintéticas";source.mkdir();destination=root/"Bóveda ñ";destination.mkdir()
(destination/".obsidian").mkdir();(destination/".obsidian/app.json").write_text('{"user":true}')
keep=destination/"Nota del usuario.md";keep.write_text("USER NOTE")
(source/"Teoría del muestreo.md").write_text("""# Muestreo aleatorio y sesgo
Este texto explica el muestreo aleatorio simple como método de selección arqueológica.
Cada unidad del marco de muestreo tiene la misma probabilidad de ser seleccionada.
La selección aleatoria limita el sesgo de elegir únicamente los lugares más visibles.
No garantiza encontrar todos los materiales ni elimina los errores de medición.
El método requiere definir el marco de muestreo antes de seleccionar las unidades.
No se propone una fecha ni una cronología para ningún sitio.
""",encoding="utf-8")
create_pdf(source/"Aplicación de campo.pdf", """The field project applies simple random sampling to 20 archaeological units.
Five units were selected using random numbers from a previously defined sampling frame.
The purpose is to reduce selection bias rather than to choose only visible deposits.
Fourteen ceramic fragments were counted in the selected units.
Random sampling does not guarantee that every deposit is represented.
No absolute dating was performed. No site chronology is inferred.""")
create_docx(source/"Depósito financiero.docx")
# Sustituye el párrafo de prueba y conserva la estructura DOCX sin ejecutar Office.
import zipfile
p=source/"Depósito financiero.docx"
with zipfile.ZipFile(p) as z: contents={n:z.read(n) for n in z.namelist()}
body="Un depósito bancario guarda dinero en una cuenta. El interés remunera el saldo financiero. Este documento trata de finanzas personales y no describe excavaciones, selección aleatoria ni materiales arqueológicos."
contents["word/document.xml"]=('<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>'+body+'</w:t></w:r></w:p></w:body></w:document>').encode()
with zipfile.ZipFile(p,"w") as z:
    for n,b in contents.items():z.writestr(n,b)
before={str(p):digest(p) for p in source.iterdir()};start=time.monotonic()
if args.payload:
    request=root/"request.json";reply=root/"response.json"
    request.write_text(json.dumps({"paths":[str(source)],"destination":str(destination),"research":True,"model":args.model}),encoding="utf-8")
    env={k:v for k,v in os.environ.items() if k.upper() not in ("PYTHONPATH","PYTHONHOME","QT_PLUGIN_PATH","QML2_IMPORT_PATH")}
    env["PATH"]=os.environ["SystemRoot"]+";"+str(Path(os.environ["SystemRoot"])/"System32")
    env["GAMBOA_CONFIG_DIR"]=str(root/"isolated-transcription-profile")
    exe=args.payload.resolve()/"ArchaeoHub.exe"
    result=subprocess.run([str(exe),"--data-dir",str(root/"profile"),"--library-request",str(request),"--report",str(reply)],env=env,capture_output=True,timeout=1800)
    data=json.loads(reply.read_text(encoding="utf-8"));assert result.returncode==0 and data["ok"],data
    folder=Path(data["folder"])
else:
    base=root/"plan.json";plan=analyze({"paths":[str(source)],"planFile":str(base)})
    target=root/"research.json"
    def progress(**values):print(json.dumps(values,ensure_ascii=False),flush=True)
    result=enrich({"planFile":str(base),"planDigest":plan["planDigest"],"analysisFile":str(target),"model":args.model},progress)
    folder=Path(export({"planFile":str(target),"planDigest":result["planDigest"],"destination":str(destination)})["folder"])
manifest=json.loads((folder/"archaeohub-library.json").read_text(encoding="utf-8"))
research=manifest["research"];analysis=research["analysis"]
assert len(analysis["documents"])==3 and all(d["summary"] for d in analysis["documents"])
field_id=next(d["id"] for d in manifest["documents"] if "campo" in d["name"])
field_summary=" ".join(c["text"] for d in analysis["documents"] if d["id"]==field_id for c in d["summary"])
assert any(token in field_summary.casefold() for token in ("14","catorce","fourteen")) and any(token in field_summary.casefold() for token in ("20","veinte","twenty")),field_summary
bank_id=next(d["id"] for d in manifest["documents"] if "financiero" in d["name"])
cross=[r for r in analysis["relations"] if r["scope"]=="cross-document"]
assert cross,"No substantive cross-document relation was produced for the explicit application fixture"
assert not any(e["document"]==bank_id for r in cross for e in r["evidence"]),"Unrelated finance text linked to archaeology"
assert all(len({e["document"] for e in r["evidence"]})==2 for r in cross)
for file,expected in manifest["files"].items():
    path=folder/file;assert digest(path)==expected
    for target,anchor in re.findall(r"\[\[([^\]|#]+)(?:#\^([^\]|]+))?(?:\|[^\]]*)?\]\]",path.read_text(encoding="utf-8")):
        linked=destination/(target+".md");assert linked.is_file(),target
        if anchor:assert "^"+anchor in linked.read_text(encoding="utf-8"),anchor
assert before=={str(p):digest(p) for p in source.iterdir()}
assert keep.read_text()=="USER NOTE" and (destination/".obsidian/app.json").read_text()=='{"user":true}'
report={"ok":True,"syntheticOnly":True,"model":args.model,"documents":3,"allSummariesNonempty":True,
        "connections":len(analysis["relations"]),"crossDocumentConnections":len(cross),"unrelatedHomonymRejected":True,
        "allWikiLinksAndEvidenceAnchorsResolve":True,"originalsAndVaultUnchanged":True,
        "sourceCharactersRead":analysis["statistics"]["sourceCharactersRead"],"modelCalls":analysis["calls"],
        "rejectedProposals":analysis["statistics"]["rejectedProposals"],"seconds":round(time.monotonic()-start,2),
        "limits":"Targeted model acceptance, not proof of factual accuracy for arbitrary documents. Interpretations require review."}
if args.payload:report.update(hostSha256=digest(args.payload/"ArchaeoHub.exe"),workerSha256=digest(args.payload/"modules/documents/engine/DocumentWorker.exe"))
(root/"grounded-documents-validation.json").write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding="utf-8")
print(json.dumps(report,ensure_ascii=False),flush=True)
