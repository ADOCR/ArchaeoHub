"""Valida búsqueda ampliada de CRS, definiciones, paginación y controles geodésicos sintéticos."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import pyproj

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--payload',type=Path,required=True);p.add_argument('--workdir',type=Path,required=True)
args=p.parse_args();payload=args.payload.resolve();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError('Use an empty verification directory')
engine=payload/'modules/coordinates/engine';worker=payload/'CoordinateWorker.exe'
env={k:os.environ[k] for k in ('SystemRoot','WINDIR','TEMP','TMP','USERPROFILE','LOCALAPPDATA','APPDATA') if k in os.environ}
env['PATH']=str(payload)+';'+str(engine)+';'+env['SystemRoot']+';'+str(Path(env['SystemRoot'])/'System32');env['PROJ_NETWORK']='OFF'
requests=0;checks=[]
def rpc(op,**values):
    global requests
    requests+=1
    run=subprocess.run([str(worker),'--engine-dir',str(engine)],input=json.dumps({'schemaVersion':1,'op':op,**values}).encode(),capture_output=True,env=env,timeout=30)
    events=[json.loads(line) for line in run.stdout.splitlines()];assert events,(run.returncode,run.stderr)
    value=events[-1];(root/f'{requests:03d}-{op}.json').write_text(json.dumps(value,indent=2),encoding='utf-8')
    return value
def check(name,condition):
    assert condition,name
    checks.append(name)
def xy(reply):
    assert reply['ok'],reply
    row=reply['rows'][0];assert row['status']=='ok',row
    check('No ballpark operation',all(not op['ballpark'] for op in reply['operations']))
    return row['resultX'],row['resultY']
groups={};codes=set()
for group,minimum in (('recommended',25),('costa-rica',12),('central-america',21),('south-america',46),('geographic',15),('utm-north',60),('utm-south',60),('world',10)):
    reply=rpc('search',query='',group=group);assert reply['ok'],reply
    members=[x['code'] for x in reply['matches']];check('Group '+group,len(members)==minimum and reply['total']==minimum and len(set(members))==minimum)
    groups[group]=members;codes.update(members)
definitions={}
for code in sorted(codes):
    reply=rpc('describe',source=code,target=code);check('Definition '+code,reply['ok'])
    info=reply['source'];check('Metadata '+code,bool(info['datum'] and info['ellipsoid'] and info['area'] and info['wkt']) and len(info['axes'])==2)
    check('Axes '+code,[x['direction'] for x in info['axes']]==['east','north'])
    definitions[code]={k:info[k] for k in ('name','datum','ellipsoid','geographic','xUnit','yUnit','area','bbox','officialAxes','axes')}
check('Ocotepeque geographic visible','EPSG:5451' in groups['costa-rica'])
check('POSGAR is 2D','EPSG:5340' in groups['south-america'] and 'EPSG:5342' not in codes)
check('All northern WGS84 UTM zones',set(groups['utm-north'])=={f'EPSG:{32600+i}' for i in range(1,61)})
check('All southern WGS84 UTM zones',set(groups['utm-south'])=={f'EPSG:{32700+i}' for i in range(1,61)})
meridian=pyproj.Geod(ellps='WGS84').inv(0,0,0,10)[2]*0.9996
for south in (False,True):
    for zone in range(1,61):
        code=32700+zone if south else 32600+zone;lon=6*zone-183;lat=-10 if south else 10
        reply=rpc('convert',source='EPSG:4326',target=f'EPSG:{code}',rows=[{'id':'Synthetic','x':str(lon),'y':str(lat)}])
        x,y=xy(reply);expected=10000000-meridian if south else meridian
        check(f'UTM meridian {code}',abs(x-500000)<1e-6 and abs(y-expected)<1e-5)
        check(f'UTM units {code}',reply['target']['xUnit']=='metre' and reply['target']['yUnit']=='metre')
for source,target in ((5451,5460),(5451,5459),(5451,5461),(4674,31982),(4686,3116),(5340,5345),(4248,24878),(4326,3857),(4326,3395)):
    info=definitions[f'EPSG:{target}'];w,s,e,n=info['bbox'];point=((w+e)/2,(s+n)/2)
    actual=xy(rpc('convert',source=f'EPSG:{source}',target=f'EPSG:{target}',rows=[{'x':str(point[0]),'y':str(point[1])}]))
    oracle=pyproj.Transformer.from_crs(source,target,always_xy=True,allow_ballpark=False,only_best=True).transform(*point,errcheck=True)
    check(f'Oracle {source} to {target}',max(abs(a-b) for a,b in zip(actual,oracle))<0.02)
    back=xy(rpc('convert',source=f'EPSG:{target}',target=f'EPSG:{source}',rows=[{'x':str(actual[0]),'y':str(actual[1])}]))
    check(f'Round trip {target} to {source}',max(abs(a-b) for a,b in zip(back,point))<1e-7)
first=rpc('search',query='',group='all');second=rpc('search',query='',group='all',offset=100)
check('Browse all EPSG and page without overlap',first['ok'] and second['ok'] and first['total']>200 and second['offset']==100 and len(second['matches'])==100 and not ({x['code'] for x in first['matches']}&{x['code'] for x in second['matches']}))
query1=rpc('search',query='UTM',group='all');query2=rpc('search',query='UTM',group='all',offset=100)
check('Search paging',query1['total']==query2['total'] and len(query2['matches'])==100 and not ({x['code'] for x in query1['matches']}&{x['code'] for x in query2['matches']}))
exact=rpc('search',query='EPSG:32719');check('Legacy global search preserved',any(x['code']=='EPSG:32719' for x in exact['matches']))
empty=rpc('search',query='No-such-coordinate-system',group='all');check('Empty search safe',empty['ok'] and empty['matches']==[] and empty['total']==0)
for extra in ({'group':'unknown'},{'group':None},{'offset':-1},{'offset':1.5},{'offset':True},{'offset':100001}):
    check('Reject invalid search '+str(extra),not rpc('search',query='',**extra)['ok'])
for code in ('EPSG:5342','EPSG:4979','EPSG:7789'):
    check('3D/dynamic still rejected '+code,not rpc('describe',source=code,target='EPSG:4326')['ok'])
out=rpc('convert',source='EPSG:4326',target='EPSG:32719',rows=[{'x':'-69','y':'10'}]);check('Wrong UTM hemisphere rejected',out['rows'][0]['error']=='input.outside_target_area')
result={'ok':True,'groups':groups,'uniqueCuratedCrs':len(codes),'definitions':definitions,'checks':len(checks),'requests':requests,
        'utmZonesWithGeodeticControl':120,'regionalAndWorldOraclePairs':9,'network':False,
        'oracle':{'pyproj':pyproj.__version__,'proj':pyproj.proj_version_str,'independentImplementation':False},
        'hostSha256':hashlib.sha256((payload/'ArchaeoHub.exe').read_bytes()).hexdigest(),'workerSha256':hashlib.sha256(worker.read_bytes()).hexdigest()}
(root/'crs-catalog-validation.json').write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ('groups','definitions')},ensure_ascii=False))
