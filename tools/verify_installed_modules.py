"""Verifica motores y vistas nativos desde una instalación Unicode real y después desinstala."""
from pathlib import Path
import argparse
import json
import subprocess
import sys
from verify_windows_installer import install, registry_snapshot, sha

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--installer",type=Path,required=True)
parser.add_argument("--workdir",type=Path,required=True)
parser.add_argument("--inventory-source",type=Path)
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError("Use an empty verification directory")
installed=root/"Aplicación arqueológica ñ";assert installed.is_relative_to(root)
before=registry_snapshot();assert install(args.installer.resolve(),installed)==0
try:
    subprocess.run([sys.executable,str(Path(__file__).with_name("verify_native_modules.py")),"--payload",str(installed),"--workdir",str(root/"module-checks")],check=True,timeout=300)
    subprocess.run([sys.executable,str(Path(__file__).with_name("verify_documents.py")),"--payload",str(installed),"--workdir",str(root/"document-checks")],check=True,timeout=620)
    subprocess.run([sys.executable,str(Path(__file__).with_name("verify_document_library.py")),"--payload",str(installed),"--workdir",str(root/"library-checks")],check=True,timeout=270)
    if args.inventory_source:
        subprocess.run([sys.executable,str(Path(__file__).with_name("verify_inventory.py")),"--payload",str(installed),"--workdir",str(root/"inventory-checks"),"--source",str(args.inventory_source.resolve())],check=True,timeout=220)
finally:
    command=subprocess.list2cmdline([str(installed/"uninstall.exe"),"/S"])+" _?="+str(installed)
    removed=subprocess.run(command,timeout=60).returncode==0
    assert removed and not (installed/"ArchaeoHub.exe").exists()
assert registry_snapshot()==before
result={"ok":True,"installerSha256":sha(args.installer),"unicodeInstallation":True,"nativeModules":True,"realLocalSummaryFromInstalledApp":True,"documentLibraryFromInstalledApp":True,"inventoryFromInstalledApp":bool(args.inventory_source),"uninstalled":True,"registryUnchanged":True}
(root/"installed-modules-validation.json").write_text(json.dumps(result,indent=2),encoding="utf-8")
print(json.dumps(result))
