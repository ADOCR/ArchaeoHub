"""Lee metadatos de un catálogo existente sin bloquearlo, crear Catalog ni migrarlo."""
from pathlib import Path
import argparse
import hashlib
import json
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/"modules/catalog/python"))
from catalog_project import inspect_project
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--catalog",type=Path,required=True);parser.add_argument("--source",type=Path,required=True);parser.add_argument("--report",type=Path,required=True)
args=parser.parse_args();folder=args.catalog.resolve();files=[folder/"estado_catalogador.json",folder/"Entrega_Museo/Catalogo_fotografico_pozos.xlsx"]
def snapshot():return {p.name:{"sha256":hashlib.sha256(p.read_bytes()).hexdigest(),"size":p.stat().st_size,"mtimeNs":p.stat().st_mtime_ns} for p in files}
before=snapshot();project=inspect_project(folder,str(args.source.resolve()));assert snapshot()==before
assert not project["needsSite"] and not project["needsOperation"] and not project["needsSource"]
report={"ok":True,"readOnly":True,"metadataOriginalsUnchanged":True,"project":project,"files":before}
args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding="utf-8")
print(json.dumps({"ok":True,"site":project["site"],"rows":project["rows"],"operations":project["operations"]}))
