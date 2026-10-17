"""Prueba el selector real de CRS por grupos en ventanas compactas y amplias, claras y oscuras."""
from pathlib import Path
import argparse,json,os,subprocess,hashlib
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--payload',type=Path,required=True);p.add_argument('--workdir',type=Path,required=True)
a=p.parse_args();payload=a.payload.resolve();root=a.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError('Use an empty QA directory')
env={k:os.environ[k] for k in ('SystemRoot','WINDIR','TEMP','TMP','USERPROFILE','LOCALAPPDATA','APPDATA','COMSPEC') if k in os.environ}
env['PATH']=env['SystemRoot']+';'+str(Path(env['SystemRoot'])/'System32');env['GAMBOA_CONFIG_DIR']=str(root/'transcription-private')
results={}
for name,group,total,query,width,theme in (
    ('recommended',0,25,'',1280,'dark'),('south-america',3,46,'',1280,'light'),
    ('utm-south',6,60,'',960,'dark'),('all-epsg',8,None,'',960,'light'),('global-exact-search',0,1,'EPSG:32719',960,'dark')):
    options=root/(name+'-options.json');options.write_text(json.dumps({'crsPicker':True,'crsGroup':group,'crsQuery':query}),encoding='utf-8')
    report=root/(name+'.json')
    run=subprocess.run([str(payload/'ArchaeoHub.exe'),'--data-dir',str(root/(name+'-profile')),'--smoke-test','--route','tool/workbench.coordinates','--theme',theme,'--width',str(width),'--height','640' if width==960 else '820','--coordinate-ui-options',str(options),'--report',str(report),'--screenshot',str(root/(name+'.png'))],env=env,capture_output=True,timeout=40)
    assert run.returncode==0,(name,run.returncode,run.stderr)
    data=json.loads(report.read_text(encoding='utf-8'));assert data['ok'] and not data['qmlWarnings'],data
    picker=data['coordinateUi']['picker'];assert picker['visible'] and picker['offset']==0,(name,picker)
    assert picker['total']==total if total is not None else picker['total']>100,(name,picker)
    assert len(picker['results'])==min(picker['total'],100),(name,picker)
    assert len({x['code'] for x in picker['results']})==len(picker['results'])
    results[name]=picker;print(name+' PASS',flush=True)
result={'ok':True,'scenarios':results,'hostSha256':hashlib.sha256((payload/'ArchaeoHub.exe').read_bytes()).hexdigest()}
(root/'crs-picker-validation.json').write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print('CRS picker validation passed.')
