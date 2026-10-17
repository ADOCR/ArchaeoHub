"""Prueba conversión por lotes mediante el controlador C++ real y el lector incluido, sin conexión externa."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tests"))
from document_worker_tests import create_pdf, create_docx

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--payload",type=Path,required=True);parser.add_argument("--workdir",type=Path,required=True)
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError("Use an empty verification directory")
source=root/"Fuentes sintéticas";source.mkdir();output=root/"Bóveda ñ";output.mkdir()
settings=output/".obsidian";settings.mkdir();(settings/"app.json").write_bytes(b'{"userSetting":true}')
keep=output/"Nota del usuario.md";keep.write_bytes(b"USER NOTE: never overwrite")
for index in range(300):
    folder=source/f"Grupo {index%4}";folder.mkdir(exist_ok=True)
    theme="cerámica estratigrafía" if index<160 else "fotogrametría cartografía"
    (folder/f"Nota{index:04d}.md").write_text(f"---\ntitle: Registro {index:04d}\n---\n# Contexto {index:04d}\n{theme}. Excavación y documentación de laboratorio. Código QA-{index:04d}.\n[[Nota0001]]\n",encoding="utf-8")
for index in range(40):create_pdf(source/f"Reporte{index:02d}.pdf",f"Ceramica estratigrafia. Operation 2, pit 7, record {index}.")
for index in range(2):create_docx(source/f"Informe{index}.docx")
for index in range(3):create_pdf(source/f"Escaneo{index}.pdf","")
(source/"Texto extenso.md").write_text("Cerámica estratigrafía. "*16000+"MARCADOR FINAL",encoding="utf-8")
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
before={str(p.relative_to(source)):sha(p) for p in source.rglob("*") if p.is_file()}
kept={keep:sha(keep),settings/"app.json":sha(settings/"app.json")}
request=root/"request.json";request.write_text(json.dumps({"paths":[str(source)],"destination":str(output),"automaticLinks":True}),encoding="utf-8")
env={k:v for k,v in os.environ.items() if k.upper() not in ("PYTHONHOME","PYTHONPATH","QT_PLUGIN_PATH","QML2_IMPORT_PATH")}
env["PATH"]=os.environ["SystemRoot"]+";"+str(Path(os.environ["SystemRoot"])/"System32")
env["GAMBOA_CONFIG_DIR"]=str(root/"isolated-transcription-profile")
exe=args.payload.resolve()/"ArchaeoHub.exe";reply=root/"response.json";start=time.monotonic()
result=subprocess.run([str(exe),"--data-dir",str(root/"profile"),"--library-request",str(request),"--report",str(reply)],env=env,capture_output=True,timeout=260)
data=json.loads(reply.read_text(encoding="utf-8"));assert result.returncode==0 and data["ok"],(result.returncode,data)
assert data["ready"]==343 and data["failed"]==3 and data["topics"],data
folder=Path(data["folder"]);assert folder.is_relative_to(output)
manifest=json.loads((folder/"archaeohub-library.json").read_text(encoding="utf-8"))
assert len(manifest["documents"])==343 and len(manifest["pending"])==3
assert "MARCADOR FINAL" in next((folder/"Documentos").glob("Texto extenso--*.md")).read_text(encoding="utf-8")
assert all(row["status"]=="needs-ocr" for row in manifest["pending"])
assert before=={str(p.relative_to(source)):sha(p) for p in source.rglob("*") if p.is_file()}
assert all(sha(path)==value for path,value in kept.items())
edges=0
for path,expected in manifest["files"].items():
    note=folder/path;assert sha(note)==expected
    text=note.read_text(encoding="utf-8")
    for target in re.findall(r"\[\[([^\]|#]+)(?:[^\]]*)\]\]",text):
        assert (Path(manifest["linkRoot"])/(target+".md")).is_file(),(path,target);edges+=1
assert edges>343 and any(t["name"]=="ceramica" for t in manifest["topics"])
assert not (folder/".obsidian").exists() and not list(folder.rglob("*.pdf")) and not list(folder.rglob("*.docx"))
report={"ok":True,"syntheticOnly":True,"documents":343,"ocrPending":3,"topics":len(manifest["topics"]),"resolvedWikiEdges":edges,"seconds":round(time.monotonic()-start,2),"originalsUnchanged":True,"existingVaultNotesAndSettingsUnchanged":True,"manifestHashesMatch":True,"noOriginalCopies":True,"networkUsed":False,"hostSha256":sha(exe),"workerSha256":sha(args.payload.resolve()/"modules/documents/engine/DocumentWorker.exe"),"limits":"Vocabulary-based links, no semantic or archaeological validation; Obsidian GUI not automated."}
(root/"document-library-validation.json").write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding="utf-8")
print(json.dumps(report,ensure_ascii=False),flush=True)
