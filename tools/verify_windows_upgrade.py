"""Actualización real de la versión anterior a la actual en modo TEST, conservando datos de prueba."""
from pathlib import Path
import argparse
import hashlib
import json
import sqlite3
import subprocess
from verify_windows_installer import install, sha, clean_environment, registry_snapshot

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--previous",type=Path,required=True);parser.add_argument("--current",type=Path,required=True)
parser.add_argument("--payload",type=Path,required=True);parser.add_argument("--workdir",type=Path,required=True)
parser.add_argument("--fixture",type=Path,required=True)
args=parser.parse_args();root=args.workdir.resolve()
root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError("Use an empty verification directory")
installed=root/"Instalación anterior ñ";profile=root/"Perfil con datos";env=clean_environment();before=registry_snapshot()
assert install(args.previous.resolve(),installed)==0
exe=installed/"ArchaeoHub.exe"
def run(name,extra):
    report=root/(name+".json")
    result=subprocess.run([str(exe),"--data-dir",str(profile),"--report",str(report),*extra],env=env,capture_output=True,timeout=35)
    assert result.returncode==0,(name,result.returncode,result.stderr)
    value=json.loads(report.read_text(encoding="utf-8"));assert value["ok"];return value
manifests=profile/"tool-manifests";manifests.mkdir(parents=True,exist_ok=True)
fixture_id="qa.upgrade-fixture"
(manifests/"upgrade.json").write_text(json.dumps({"schemaVersion":1,"id":fixture_id,"version":"1.0.0","name":"Upgrade fixture","description":"Synthetic only","runtime":"external","entrypoint":{"binding":"selected-installation"},"categoryIds":["utilities"],"capabilities":["launch"],"supportedFileTypes":[]}),encoding="utf-8")
old=run("old-configure",["--configure-tool",fixture_id,"--program",str(args.fixture.resolve()),"--self-check"])
parent=root/"Proyectos";parent.mkdir();run("old-project",["--create-project","Proyecto QA", "--parent-folder",str(parent)])
settings=profile/"preferences.ini";settings.write_text("[appearance]\ntheme=dark\nreduceTransparency=true\n",encoding="utf-8")
userfile=installed/"Mis archivos"/"conservar.txt";userfile.parent.mkdir();userfile.write_bytes(b"USER FILE: preserve during upgrade and uninstall")
projectfile=parent/"Proyecto QA"/"dato_usuario.txt";projectfile.write_bytes(b"SYNTHETIC PROJECT DATA")
kept={p:sha(p) for p in (settings,userfile,projectfile)}
database=profile/"workbench.sqlite"
with sqlite3.connect(database) as db:
    db.execute("INSERT OR IGNORE INTO favorites VALUES(?)",("workbench.transcription",))
    installation=db.execute("SELECT * FROM installations").fetchall()
    assert installation
assert install(args.current.resolve(),installed)==0
assert all(sha(p)==digest for p,digest in kept.items())
for path in args.payload.resolve().rglob("*"):
    if path.is_file():assert sha(installed/path.relative_to(args.payload.resolve()))==sha(path),str(path)
result=run("new-self-check",["--self-check"])
expected_builtins=len(list((Path(__file__).resolve().parents[1]/'modules').glob('*/tool.json')))
assert result["schemaVersion"]==1 and result["registeredTools"]==expected_builtins+1 and result["volumeProbeComplete"]
with sqlite3.connect(database) as db:
    assert db.execute("SELECT * FROM installations").fetchall()==installation
    assert db.execute("SELECT count(*) FROM projects").fetchone()[0]==1
    assert db.execute("SELECT tool_id FROM favorites").fetchone()[0]=="workbench.transcription"
command=subprocess.list2cmdline([str(installed/"uninstall.exe"),"/S"])+" _?="+str(installed)
assert subprocess.run(command,timeout=60).returncode==0
assert all(sha(p)==digest for p,digest in kept.items())
assert database.is_file() and not exe.exists() and registry_snapshot()==before
report={"ok":True,"previousInstaller":args.previous.name,"currentInstaller":args.current.name,"previousSha256":sha(args.previous),"currentSha256":sha(args.current),"profileSchema":1,"settingsPreserved":True,"toolConfigurationPreserved":True,"favoritePreserved":True,"projectPreserved":True,"userFilesPreservedAfterUninstall":True,"registryUnchanged":True}
(root/"upgrade-validation.json").write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding="utf-8")
print(json.dumps(report))
