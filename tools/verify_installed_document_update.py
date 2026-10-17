"""Verifica las vistas documentales finales desde una instalación aislada con ruta Unicode."""
from pathlib import Path
import argparse,json,subprocess,sys
from verify_windows_installer import install,registry_snapshot,sha
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--installer',type=Path,required=True);parser.add_argument('--workdir',type=Path,required=True)
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError('Use an empty isolated directory')
installed=root/'Aplicación documental ñ';before=registry_snapshot()
assert install(args.installer.resolve(),installed)==0
try:
    subprocess.run([sys.executable,str(Path(__file__).with_name('verify_document_workspaces.py')),'--payload',str(installed),'--workdir',str(root/'document-checks')],check=True,timeout=300)
    checks=json.loads((root/'document-checks/document-workspaces-validation.json').read_text(encoding='utf-8'))
finally:
    command=subprocess.list2cmdline([str(installed/'uninstall.exe'),'/S'])+' _?='+str(installed)
    assert subprocess.run(command,timeout=60).returncode==0
    assert not (installed/'ArchaeoHub.exe').exists()
assert registry_snapshot()==before
report={'ok':True,'installerSha256':sha(args.installer),'unicodeInstallation':True,
        'documentConversionAndUiPassed':True,'uiScenarios':checks['uiScenarios'],
        'modelInferenceNotRepeatedFromInstalledCopy':True,'uninstalled':True,'registryUnchanged':True,
        'hostSha256':checks['hostSha256'],'workerSha256':checks['workerSha256']}
(root/'installed-document-update-validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report),flush=True)
