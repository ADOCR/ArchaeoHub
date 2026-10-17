"""Controles de proyección, referencia con otra versión, ida y vuelta entre datums y contratos de error."""
from pathlib import Path
import argparse
import json
import math
import os
import subprocess
import pyproj

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--payload",type=Path,required=True)
parser.add_argument("--workdir",type=Path,required=True)
args=parser.parse_args();payload=args.payload.resolve();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError("Use an empty verification directory")
engine=payload/"modules/coordinates/engine"
env={k:os.environ[k] for k in ("SystemRoot","WINDIR","TEMP","TMP","USERPROFILE","LOCALAPPDATA","APPDATA") if k in os.environ}
env["PATH"]=str(payload)+";"+str(engine)+";"+str(Path(env["SystemRoot"])/"System32")
env["PROJ_NETWORK"]="OFF"
report={"oracle":{"pyproj":pyproj.__version__,"proj":pyproj.proj_version_str,"independentImplementation":False},"checks":[],"operations":{}}
counter=0
def request(op,**values):
    global counter
    counter+=1
    process=subprocess.run([str(payload/"CoordinateWorker.exe"),"--engine-dir",str(engine)],input=json.dumps({"schemaVersion":1,"op":op,**values}).encode(),capture_output=True,env=env,timeout=30)
    events=[json.loads(line) for line in process.stdout.splitlines()];assert events,(process.returncode,process.stderr)
    data=events[-1]
    (root/f"{counter:03d}-{op}.json").write_text(json.dumps(data,indent=2),encoding="utf-8")
    return data
def convert(source,target,points,**options):
    result=request("convert",source=f"EPSG:{source}",target=f"EPSG:{target}",rows=[{"id":str(i+1),"x":str(x),"y":str(y)} for i,(x,y) in enumerate(points)],**options)
    assert result["ok"],result
    return result
def row(result,index=0):
    value=result["rows"][index];assert value["status"]=="ok",value
    return value["resultX"],value["resultY"]
def check(name,condition,**details):
    assert condition,(name,details);report["checks"].append({"name":name,"ok":True,**details})
# Ejemplo publicado EPSG/IOGP, guía 7-2, método 9807. Los valores publicados están redondeados.
british=row(convert(4277,27700,[(0.5,50.5)]))
check("Published EPSG transverse Mercator control",abs(british[0]-577274.99)<0.02 and abs(british[1]-69740.50)<0.02,toleranceMetres=0.02,source="https://www.iogp.org/bookstore/wp-content/uploads/sites/2/woocommerce_uploads/2017/01/373-07-02.pdf")
for source,target,ellipsoid in ((5365,5367,"WGS84"),(8907,8908,"GRS80")):
    result=convert(source,target,[(-84,10)])
    x,y=row(result)
    # En el meridiano central, la distancia coincide con la longitud geodésica del meridiano multiplicada por la escala.
    expected=pyproj.Geod(ellps=ellipsoid).inv(0,0,0,10)[2]*0.9999
    check(f"CRTM05 meridian control {source}→{target}",abs(x-500000)<1e-7 and abs(y-expected)<1e-5,toleranceMetres=1e-5,expectedNorthing=expected)
    check(f"Axis convention {target}",result["target"]["axes"][0]["direction"]=="east" and result["target"]["axes"][1]["direction"]=="north")
    if target==5367:check("CR05 official North/East normalized",result["target"]["officialAxes"][0]["direction"]=="north")
    check(f"Units {target}",result["source"]["xUnit"]=="degree" and result["target"]["xUnit"]=="metre")
for target,longitude in ((32616,-87),(32617,-81)):
    x,y=row(convert(4326,target,[(longitude,0)]))
    check(f"UTM zone {target} central-meridian origin",abs(x-500000)<1e-6 and abs(y)<1e-7)
# Sistemas costarricenses y ambas zonas WGS84 UTM. La referencia utiliza otra versión y otro adaptador de PROJ.
pairs=[(4326,5367,[(-85.1,10.2),(-84.2,9.9),(-83.1,9.0)]),(4326,8908,[(-85.1,10.2),(-84.2,9.9),(-83.1,9.0)]),(5365,5367,[(-85.1,10.2),(-84.2,9.9),(-83.1,9.0)]),(8907,8908,[(-85.1,10.2),(-84.2,9.9),(-83.1,9.0)]),(4326,32616,[(-85.4,10.2),(-84.2,9.9)]),(4326,32617,[(-83.8,10.2),(-83.1,9.0)]),(4326,5456,[(-84.5,10.5),(-84.1,10.1)]),(4326,5457,[(-83.5,9.2),(-83.0,8.7)]),(5367,8908,[(500000,1105744.247751049)])]
for source,target,points in pairs:
    result=convert(source,target,points);oracle=pyproj.Transformer.from_crs(source,target,always_xy=True,allow_ballpark=False,only_best=True)
    projected=[]
    for index,point in enumerate(points):
        actual=row(result,index);expected=oracle.transform(*point,errcheck=True)
        check(f"Oracle {source}→{target} row {index+1}",max(abs(a-b) for a,b in zip(actual,expected))<0.02,toleranceMetres=0.02)
        projected.append(actual)
    inverse=convert(target,source,projected)
    tolerance=1e-7 if source not in (5367,) else 0.02
    for index,point in enumerate(points):
        actual=row(inverse,index);check(f"Round trip {source}→{target}→{source} row {index+1}",max(abs(a-b) for a,b in zip(actual,point))<tolerance,tolerance=tolerance)
    report["operations"][f"{source}-{target}"]=[{"name":o["name"],"accuracyMetres":o["accuracyMetres"],"ballpark":o["ballpark"]} for o in result["operations"]]
    check(f"No ballpark {source}→{target}",all(not o["ballpark"] for o in result["operations"]))
batch=request("convert",source="EPSG:4326",target="EPSG:8908",text='id;x;y;nota\nA;-84,1;10,2;"texto; ñ"\nB;NaN;10,2;error\nC;10,2;-84,1;ejes invertidos\n',delimiter=";",decimal=",",header=True,xColumn=1,yColumn=2,idColumn=0)
check("Batch preserves fields and per-row failures",batch["ok"] and batch["succeeded"]==1 and batch["failed"]==2 and batch["rows"][0]["fields"][3]=="texto; ñ")
single=row(convert(4326,8908,[(-84.1,10.2)]));check("Batch equals single",max(abs(a-b) for a,b in zip(row(batch),single))<1e-8)
for source,target,point,error in ((4326,8908,(0,0),"input.outside_target_area"),(4326,32616,(-83,10),"input.outside_target_area"),(4326,8908,(10,-84),"input.outside_target_area"),(4326,8908,(-84,91),"input.angular_range")):
    result=convert(source,target,[point]);check(f"Reject {error} {point}",result["rows"][0]["error"]==error)
for code in (5364,8906,99999999,7789):
    result=request("describe",source=f"EPSG:{code}",target="EPSG:4326");check(f"Reject unsupported CRS {code}",not result["ok"])
query=request("search",query="Costa Rica");check("Search includes historical and modern CRS",query["ok"] and any(m["code"]=="EPSG:5456" for m in query["matches"]))
# Formato DMS/GD sin cambiar el datum y con coma decimal regional.
dms=convert(4326,4326,[("84 30 0 W","10 15 0 N")],inputFormat="DMS",outputFormat="DMS",precision=3)
check("DMS identity",row(dms)==(-84.5,10.25))
invalid=request("convert",source="EPSG:4326",target="EPSG:8908",text="a,b,c\n1,2,3",delimiter=",",decimal=",")
check("Ambiguous decimal/column separator rejected",not invalid["ok"] and invalid["code"]=="input.decimal_delimiter")
report["engine"]=batch["engine"];report["requests"]=counter;report["ok"]=True
(root/"coordinate-validation.json").write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding="utf-8")
print(f"{len(report['checks'])} geodetic checks passed; {counter} worker requests")
