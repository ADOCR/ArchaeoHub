"""Copia de SQLite y preferencias mediante lectura antes de una actualización local indicada por el usuario."""
from pathlib import Path
from datetime import datetime,timezone
import argparse,hashlib,json,shutil,sqlite3
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--profile',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();source=args.profile.resolve();target=args.output.resolve();target.mkdir(parents=True,exist_ok=False)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();database=source/'workbench.sqlite';before=sha(database)
reader=sqlite3.connect(database.as_uri()+'?mode=ro',uri=True);writer=sqlite3.connect(target/'workbench.sqlite')
try:reader.backup(writer)
finally:writer.close();reader.close()
files={}
for p in [*source.glob('*.ini'),*(source/'tool-manifests').glob('*.json')]:
 relative=p.relative_to(source);out=target/relative;out.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,out);files[str(relative)]=sha(p);assert sha(out)==files[str(relative)]
assert sha(database)==before
report={'ok':True,'createdAtUtc':datetime.now(timezone.utc).isoformat(),'sourceDatabaseUnchanged':True,'sourceDatabaseSha256':before,'preferencesAndManifestFiles':files,'backupDatabaseSha256':sha(target/'workbench.sqlite')}
(target/'backup-validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps({'ok':True,'backup':str(target),'preferencesFiles':len(files),'sourceDatabaseUnchanged':True}))
