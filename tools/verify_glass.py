"""Comprueba vidrio acelerado, accesibilidad opaca y alternativa por software en perfiles aislados."""
from pathlib import Path
import argparse,json,os,subprocess,hashlib
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--payload',type=Path,required=True);p.add_argument('--workdir',type=Path,required=True)
p.add_argument('--first-only',action='store_true')
a=p.parse_args();payload=a.payload.resolve();root=a.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError('Use an empty verification directory')
env={k:os.environ[k] for k in ('SystemRoot','WINDIR','TEMP','TMP','USERPROFILE','LOCALAPPDATA','APPDATA','COMSPEC') if k in os.environ}
env['PATH']=env['SystemRoot']+';'+str(Path(env['SystemRoot'])/'System32');env['GAMBOA_CONFIG_DIR']=str(root/'transcription-private')
cases=[
 ('palette-dark','home','dark',1280,['--palette'],{}),
 ('palette-light','home','light',960,['--palette'],{}),
 ('home-dark','home','dark',1280,[],{}),('home-light','home','light',1280,[],{}),
 ('coordinates','tool/workbench.coordinates','light',960,[],{}),
 ('new-project','home','dark',1280,['--new-project-dialog'],{}),
 ('reduced-transparency','home','light',1280,['--palette'],{'reduceTransparency':'true'}),
 ('high-contrast','home','dark',960,['--palette'],{'highContrast':'true'}),
 ('large-text','home','dark',1280,['--palette'],{'textScale':'1.4'}),
 ('reduced-motion','home','light',1280,['--palette'],{'reduceMotion':'true'}),
 ('software','home','dark',960,['--palette'],{}),
]
if a.first_only:cases=cases[:1]
results={}
for name,route,theme,width,flags,settings in cases:
 profile=root/(name+'-profile');profile.mkdir()
 if settings:(profile/'preferences.ini').write_text('[appearance]\n'+''.join(k+'='+v+'\n' for k,v in settings.items()),encoding='utf-8')
 current=env.copy()
 if name=='software':current['QT_QUICK_BACKEND']='software'
 report=root/(name+'.json');shot=root/(name+'.png')
 run=subprocess.run([str(payload/'ArchaeoHub.exe'),'--data-dir',str(profile),'--smoke-test','--route',route,'--theme',theme,'--width',str(width),'--height','640' if width==960 else '820','--report',str(report),'--screenshot',str(shot),*flags],env=current,capture_output=True,timeout=60)
 (root/(name+'-stdout.txt')).write_bytes(run.stdout);(root/(name+'-stderr.txt')).write_bytes(run.stderr)
 value=json.loads(report.read_text(encoding='utf-8')) if report.exists() else {}
 assert run.returncode==0 and value.get('ok'),(name,run.returncode,value,run.stderr.decode(errors='replace'))
 assert not value['qmlWarnings'] and shot.is_file(),(name,value)
 surfaces=value['glassSurfaces'];assert surfaces,(name,value)
 fallback=name in ('reduced-transparency','high-contrast','software')
 if fallback:assert all(s['opaqueFallback'] and not s['frosted'] and not s['effectReady'] for s in surfaces),(name,value)
 else:
  assert value['graphicsApi'] not in (0,1,2,7),(name,'Accelerated rendering unavailable',value)
  assert all(s['frosted'] and s['effectReady'] and not s['opaqueFallback'] for s in surfaces),(name,value)
 if name=='software':assert value['graphicsApi']==1,value
 if '--palette' in flags or '--new-project-dialog' in flags:assert any(s['popup'] for s in surfaces),(name,value)
 results[name]={'graphicsApi':value['graphicsApi'],'surfaces':surfaces,'warnings':value['qmlWarnings'],'screenshot':str(shot)}
 print(json.dumps({'case':name,'graphicsApi':value['graphicsApi'],'glassSurfaces':len(surfaces),'frosted':sum(s['frosted'] for s in surfaces),'ok':True}),flush=True)
result={'ok':True,'scenarios':results,'hostSha256':hashlib.sha256((payload/'ArchaeoHub.exe').read_bytes()).hexdigest(),
        'limits':'In-app backdrop diffusion; no OS desktop capture. Windows only. No GPU performance guarantee on other machines.'}
(root/'glass-validation.json').write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
