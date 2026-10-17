"""Verifica una distribución Windows del Hub con perfil aislado y sin SDK en PATH."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import sqlite3
import subprocess
import time

def clean_environment():
    names=['SystemRoot','WINDIR','TEMP','TMP','USERPROFILE','LOCALAPPDATA','APPDATA','COMSPEC','PROGRAMFILES','PROGRAMFILES(X86)','USERNAME']
    env={k:os.environ[k] for k in names if k in os.environ}
    env['PATH']=str(Path(env['SystemRoot'])/'System32')+';'+env['SystemRoot']
    return env

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--payload',type=Path,required=True);parser.add_argument('--workdir',type=Path,required=True)
    parser.add_argument('--transcriber',type=Path);parser.add_argument('--launch-transcriber',action='store_true')
    parser.add_argument('--launch-fixture',type=Path)
    args=parser.parse_args();root=args.workdir.resolve();source=Path(__file__).resolve().parents[1]
    if root.exists() and any(root.iterdir()):raise ValueError('Use an empty, dedicated verification folder')
    root.mkdir(parents=True,exist_ok=True);profile=root/'profile';exe=args.payload.resolve()/'ArchaeoHub.exe';env=clean_environment()
    env['GAMBOA_CONFIG_DIR']=str(root/'isolated-transcription-profile')
    report={'environment':'Windows system PATH only; no Qt SDK or Python runtime','ui':{}}
    def run(name,options,expected=0,run_profile=None):
        target=root/(name+'.json')
        result=subprocess.run([str(exe),'--data-dir',str(run_profile or profile),'--report',str(target),*options],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=35)
        assert result.returncode==expected,(name,result.returncode)
        data=json.loads(target.read_text(encoding='utf-8'));assert data['ok']==(expected==0),(name,data)
        print(name, 'OK',flush=True);return data
    report['self_check']=run('self-check',['--self-check'])
    assert report['self_check']['volumeProbeComplete'] and report['self_check']['volumes']>=1
    with sqlite3.connect(profile/'workbench.sqlite') as db:
        db.execute('INSERT INTO tasks VALUES(?,?,?,?,?,?,?,?)',('qa-readable-document-error','documents.summarize','Resumen de documentos','failed',0,'','El modelo alcanzó su límite de respuesta. Prueba otro modelo.','2026-10-16T00:00:00Z'))
    if args.launch_fixture:
        import ctypes,msvcrt
        from ctypes import wintypes
        fixture_profile=root/'external-fixture-profile';manifests=fixture_profile/'tool-manifests';manifests.mkdir(parents=True)
        fixture_id='qa.launch-fixture'
        (manifests/'fixture.json').write_text(json.dumps({'schemaVersion':1,'id':fixture_id,'version':'1.0.0','name':{'es':'Fixture de prueba'},'description':{'es':'Proceso sintético de aceptación'},'runtime':'external','entrypoint':{'binding':'selected-installation'},'categoryIds':['utilities'],'capabilities':['launch'],'supportedFileTypes':[]}),encoding='utf-8')
        run('configure-fixture',['--configure-tool',fixture_id,'--program',str(args.launch_fixture.resolve()),'--self-check'],run_profile=fixture_profile)
        target=root/'launch-fixture.json';start=time.monotonic()
        child=subprocess.Popen([str(exe),'--data-dir',str(fixture_profile),'--launch-tool',fixture_id,'--report',str(target)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        try:
            assert child.wait(timeout=20)==0
            result=json.loads(target.read_text(encoding='utf-8'));assert result['ok']
            kernel=ctypes.WinDLL('kernel32',use_last_error=True)
            kernel.PeekNamedPipe.argtypes=[wintypes.HANDLE,wintypes.LPVOID,wintypes.DWORD,wintypes.LPVOID,ctypes.POINTER(wintypes.DWORD),wintypes.LPVOID]
            def closed(pipe):
                available=wintypes.DWORD()
                handle=msvcrt.get_osfhandle(pipe.fileno())
                while kernel.PeekNamedPipe(handle,None,0,None,ctypes.byref(available),None):
                    if not available.value:return False
                    os.read(pipe.fileno(),available.value)
                return ctypes.get_last_error()==109
            assert closed(child.stdout) and closed(child.stderr),'The detached child inherited the host pipes'
            report['process_detachment']={'ok':True,'host_exit_ms':round((time.monotonic()-start)*1000),'stdio_closed':True}
        finally:
            if child.poll() is None:child.kill();child.wait(timeout=5)
            child.stdout.close();child.stderr.close()
    if args.transcriber:
        report['configure_transcription']=run('configure',['--configure-tool','workbench.transcription','--program',str(args.transcriber.resolve()),'--self-check'])
    cases=[(name,route,theme,[]) for name,route,theme in [
        ('home-light','home','light'),('home-dark','home','dark'),('tools','tools','light'),
        ('transcription','tool/workbench.transcription','light'),('coordinates','tool/workbench.coordinates','light'),
        ('documents','tool/workbench.document-summary','light'),
        ('inventory','tool/workbench.inventory','light'),
        ('catalog','tool/workbench.photo-catalog','light'),('favorites','favorites','light'),('recent','recent','light'),
        ('projects','projects','light'),('activity','activity','light'),('settings','settings','dark'),('about','about','light')]]
    cases.extend([('palette','home','dark',['--palette']),('new-project','projects','light',['--new-project-dialog']),
        ('compact-new-project','projects','dark',['--new-project-dialog','--width','960','--height','640'])])
    for name,route,theme,extra in cases:
        report['ui'][name]=run(name,['--smoke-test','--route',route,'--theme',theme,'--screenshot',str(root/(name+'.png')),*extra])
        for button in report['ui'][name].get('buttonStyles',[]):
            if button['enabled'] and button['variant']=='primary':
                assert button['caption']==('#0b0c0e' if theme=='dark' else '#ffffff'),(name,button)
    (profile/'preferences.ini').write_text('[appearance]\ntheme=light\ntextScale=1.4\nreduceTransparency=true\nreduceMotion=true\nhighContrast=true\n',encoding='utf-8')
    report['ui']['accessible-project']=run('accessible-project',['--smoke-test','--route','projects','--new-project-dialog','--width','960','--height','640','--screenshot',str(root/'accessible-project.png')])
    parent=root/'project-parent';parent.mkdir();name='Proyecto prueba á'
    report['project_creation']=run('create-project',['--create-project',name,'--parent-folder',str(parent)])
    actual=parent/name;definition=json.loads((source/'resources/project-templates/archaeology.json').read_text(encoding='utf-8'))
    assert sorted(p.relative_to(actual).as_posix() for p in actual.rglob('*'))==sorted(definition['folders'])
    assert all(p.is_dir() for p in actual.rglob('*'))
    keep=actual/'keep.txt';keep.write_bytes(b'preserve this user file')
    report['project_existing_rejected']=run('existing-project',['--create-project',name,'--parent-folder',str(parent)],7)
    assert keep.read_bytes()==b'preserve this user file'
    with sqlite3.connect(profile/'workbench.sqlite') as db:
        assert db.execute('SELECT count(*) FROM projects').fetchone()[0]==1
    report['exact_tree_empty_and_registered']=True
    if args.launch_transcriber:
        assert args.transcriber,'Specify the known executable explicitly'
        report['launch_transcription']=run('launch-transcription',['--launch-tool','workbench.transcription'])
    report['hostSha256']=hashlib.sha256(exe.read_bytes()).hexdigest();report['ok']=True;(root/'release-validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'ok':True,'ui_scenarios':len(report['ui']),'report':str(root/'release-validation.json')},ensure_ascii=False))

if __name__=='__main__':main()
