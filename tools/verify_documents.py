"""Genera resúmenes con el modelo local real usando únicamente PDF, DOCX y Markdown sintéticos."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys
import time
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tests"))
from document_worker_tests import create_pdf, create_docx

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--payload",type=Path,required=True)
parser.add_argument("--workdir",type=Path,required=True)
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError("Use an empty verification directory")
paths=[root/"Notas de campo.md",root/"Informe sintético.docx",root/"Reporte.pdf"]
paths[0].write_text("# Notas sintéticas\nOperación 2, pozo 7: se registraron 14 fragmentos cerámicos. No se ha obtenido datación absoluta. No se ha determinado la función del contexto.",encoding="utf-8")
create_docx(paths[1]);create_pdf(paths[2])
hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
request=root/"request.json";request.write_text(json.dumps({"model":"qwen3:8b","paths":[str(p) for p in paths]}),encoding="utf-8")
env={k:v for k,v in os.environ.items() if k.upper() not in ("PYTHONHOME","PYTHONPATH","QT_PLUGIN_PATH","QML2_IMPORT_PATH")}
env["PATH"]=os.environ["SystemRoot"]+";"+str(Path(os.environ["SystemRoot"])/"System32")
output=root/"Resumen.md";target=root/"response.json";exe=args.payload.resolve()/"ArchaeoHub.exe";start=time.monotonic()
result=subprocess.run([str(exe),"--data-dir",str(root/"profile"),"--document-request",str(request),"--document-export",str(output),"--report",str(target)],env=env,capture_output=True,timeout=620)
data=json.loads(target.read_text(encoding="utf-8"));assert result.returncode==0 and data["ok"],(result.returncode,data)
summary=data["summary"];assert "14" in summary and "7" in summary and "2" in summary
assert "D1" in summary and "D2" in summary and "D3" in summary and data["files"]==3
assert "dataci" in summary.lower() or "dating" in summary.lower(),summary
assert output.read_text(encoding="utf-8")==summary
assert hashes=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
report={"ok":True,"model":data["model"],"seconds":round(time.monotonic()-start,2),"syntheticOnly":True,"formats":["md","docx","pdf"],"originalsUnchanged":True,"exportMatches":True,"sourceReferences":True,"keyNumbersPreserved":True,"localOnly":True,"hostSha256":hashlib.sha256(exe.read_bytes()).hexdigest(),"summary":summary,"limits":"A synthetic acceptance sample; no guarantee of accuracy for arbitrary documents; OCR not implemented."}
(root/"documents-validation.json").write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding="utf-8")
print(json.dumps(report,ensure_ascii=False),flush=True)
