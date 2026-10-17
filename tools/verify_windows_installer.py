"""Prueba el instalador real en un directorio aislado, sin escribir en el registro del usuario."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import sqlite3
import subprocess
import winreg
import ctypes
from ctypes import wintypes

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()

def clean_environment():
    names=['SystemRoot','WINDIR','TEMP','TMP','USERPROFILE','LOCALAPPDATA','APPDATA','COMSPEC','PROGRAMFILES','PROGRAMFILES(X86)','USERNAME']
    env={key:os.environ[key] for key in names if key in os.environ}
    env['PATH']=str(Path(env['SystemRoot'])/'System32')+';'+env['SystemRoot']
    return env

def registry_snapshot():
    result={}
    for name in [r'Software\ADOCR\ArchaeologyWorkbench',r'Software\Microsoft\Windows\CurrentVersion\Uninstall\ADOCR.ArchaeologyWorkbench']:
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER,name,access=winreg.KEY_READ|winreg.KEY_WOW64_64KEY) as key:
                values=[]
                for i in range(winreg.QueryInfoKey(key)[1]):values.append(winreg.EnumValue(key,i))
                result[name]=values
        except FileNotFoundError:result[name]=None
    return result

def install(exe,directory):
    # NSIS /D consume el resto de la línea de comandos; no interviene un intérprete de comandos.
    command=subprocess.list2cmdline([str(exe),'/TEST','/S','/ACCEPTTHIRDPARTY=1'])+' /D='+str(directory)
    return subprocess.run(command,timeout=90).returncode

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--installer',type=Path,required=True);parser.add_argument('--payload',type=Path,required=True)
    parser.add_argument('--workdir',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();root=args.workdir.resolve()
    if root.exists() and any(root.iterdir()):raise ValueError('Use an empty, dedicated installer test directory')
    root.mkdir(parents=True,exist_ok=True);installed=root/'instalación prueba';foreign=root/'other app';foreign.mkdir()
    assert installed.resolve().is_relative_to(root) and foreign.resolve().is_relative_to(root)
    note=foreign/'keep.txt';note.write_text('Another application owns this folder.\n',encoding='utf-8');note_hash=sha(note)
    before_registry=registry_snapshot();report={}
    rejected=root/'no-consent'
    command=subprocess.list2cmdline([str(args.installer),'/TEST','/S'])+' /D='+str(rejected)
    report['silent_without_third_party_acceptance_rejected']=subprocess.run(command,timeout=90).returncode==3 and not rejected.exists()
    assert report['silent_without_third_party_acceptance_rejected']
    report['foreign_directory_rejected']=install(args.installer,foreign)!=0 and sha(note)==note_hash and not (foreign/'ArchaeoHub.exe').exists()
    assert report['foreign_directory_rejected']
    assert install(args.installer,installed)==0
    marker=installed/'.workbench-install.ini';assert marker.is_file()
    expected={p.relative_to(args.payload):sha(p) for p in args.payload.rglob('*') if p.is_file()}
    assert all((installed/rel).is_file() and sha(installed/rel)==digest for rel,digest in expected.items())
    report['payload_files_verified']=len(expected)
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,wintypes.LPVOID,wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]
    kernel.CreateFileW.restype=wintypes.HANDLE
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    handle=kernel.CreateFileW(str(installed/'ArchaeoHub.exe'),0x80000000,1,None,3,0,None)
    assert handle!=ctypes.c_void_p(-1).value
    try:
        report['locked_executable_update_rejected']=install(args.installer,installed)==2
        command=subprocess.list2cmdline([str(installed/'uninstall.exe'),'/S'])+' _?='+str(installed)
        report['locked_executable_uninstall_rejected']=subprocess.run(command,timeout=30).returncode==2
        assert report['locked_executable_update_rejected'] and report['locked_executable_uninstall_rejected']
        assert all(sha(installed/rel)==digest for rel,digest in expected.items())
    finally:kernel.CloseHandle(handle)
    report['locked_workers_rejected']={}
    for relative in ('CoordinateWorker.exe','modules/catalog/engine/CatalogWorker.exe','modules/coordinates/spreadsheets/SpreadsheetWorker.exe','modules/transcription/engine/TranscriptionWorker.exe','modules/documents/engine/DocumentWorker.exe','modules/inventory/engine/InventoryWorker.exe'):
        if not (installed/relative).exists():continue
        handle=kernel.CreateFileW(str(installed/relative),0x80000000,1,None,3,0,None)
        assert handle!=ctypes.c_void_p(-1).value
        try:
            rejected=install(args.installer,installed)==2
            command=subprocess.list2cmdline([str(installed/'uninstall.exe'),'/S'])+' _?='+str(installed)
            rejected=rejected and subprocess.run(command,timeout=30).returncode==2
            assert rejected,relative
            report['locked_workers_rejected'][relative]=True
        finally:kernel.CloseHandle(handle)
    profile=root/'profile';check=root/'self-check.json'
    p=subprocess.run([str(installed/'ArchaeoHub.exe'),'--data-dir',str(profile),'--self-check','--report',str(check)],env=clean_environment(),capture_output=True,timeout=25)
    assert p.returncode==0,(p.returncode,p.stderr)
    report['installed_clean_environment']=json.loads(check.read_text(encoding='utf-8'))
    assert report['installed_clean_environment']['ok']
    assert report['installed_clean_environment']['volumeProbeComplete'] and report['installed_clean_environment']['volumes']>=1
    extra=installed/'my documents'/'keep.txt';extra.parent.mkdir();extra.write_text('User file; never remove during upgrade/uninstall.\n',encoding='utf-8');extra_hash=sha(extra)
    settings=profile/'preferences.ini';settings.write_text('[appearance]\ntheme=dark\nreduceTransparency=true\n',encoding='utf-8');settings_hash=sha(settings)
    model_cache=profile/'transcription/models/QA-model.bin';model_cache.parent.mkdir(parents=True);model_cache.write_bytes(b'SYNTHETIC USER MODEL CACHE');model_hash=sha(model_cache)
    with sqlite3.connect(profile/'workbench.sqlite') as connection:
        connection.execute('INSERT OR IGNORE INTO favorites VALUES(?)',('workbench.transcription',))
    report['same_version_update']=install(args.installer,installed)==0
    assert report['same_version_update'] and sha(extra)==extra_hash and sha(settings)==settings_hash and sha(model_cache)==model_hash
    with sqlite3.connect(profile/'workbench.sqlite') as connection:
        assert connection.execute('SELECT tool_id FROM favorites').fetchone()[0]=='workbench.transcription'
    assert all(sha(installed/rel)==digest for rel,digest in expected.items())
    report['settings_database_and_user_file_preserved_on_update']=True
    uninstaller=installed/'uninstall.exe'
    command=subprocess.list2cmdline([str(uninstaller),'/S'])+' _?='+str(installed)
    result=subprocess.run(command,timeout=60)
    assert result.returncode==0,result.returncode
    assert sha(extra)==extra_hash and sha(settings)==settings_hash and sha(model_cache)==model_hash and (profile/'workbench.sqlite').is_file()
    report['model_cache_preserved_on_update_and_uninstall']=True
    assert not (installed/'ArchaeoHub.exe').exists() and not marker.exists()
    report['user_files_preserved_on_uninstall']=True
    report['registry_unchanged']=registry_snapshot()==before_registry
    assert report['registry_unchanged']
    report['installer_sha256']=sha(args.installer);report['ok']=True
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
