"""Prueba motores de módulos instalados y vistas nativas con datos exclusivamente sintéticos."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys
from PIL import Image, ImageDraw

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--payload", type=Path, required=True)
parser.add_argument("--workdir", type=Path, required=True)
args = parser.parse_args()
payload, root = args.payload.resolve(), args.workdir.resolve()
root.mkdir(parents=True, exist_ok=True)
if any(root.iterdir()): raise ValueError("Use an empty isolated verification directory")
source, output = root / "Fotografías sintéticas", root / "Catálogo de prueba"
source.mkdir(); output.mkdir()
for i in range(1, 5):
    image = Image.new("RGB", (800, 600), (30 + 15*i, 85 + 12*i, 95))
    draw = ImageDraw.Draw(image);draw.rectangle((180,150,620,450),outline=(200,230,220),width=5)
    draw.text((240,290), f"SYNTHETIC QA PHOTO {i}", fill=(240,245,240))
    exif=Image.Exif();exif[274]=6 if i==2 else 1;exif[36867]="2026:01:02 03:04:05"
    image.save(source/f"Foto prueba {i}.jpg",exif=exif)
originals={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in source.iterdir()}
names=["SystemRoot","WINDIR","TEMP","TMP","USERPROFILE","LOCALAPPDATA","APPDATA","COMSPEC","PROGRAMFILES","PROGRAMFILES(X86)","USERNAME"]
env={k:os.environ[k] for k in names if k in os.environ}
env["PATH"]=str(Path(env["SystemRoot"])/"System32")+";"+env["SystemRoot"]
env["GAMBOA_CONFIG_DIR"]=str(root/"isolated-transcription-profile")
report={"environment":"Windows system PATH only; no external Python, Qt SDK or PROJ", "ui":{},"frozenCatalog":{}}
worker=payload/"modules/catalog/engine/CatalogWorker.exe"
process=subprocess.Popen([str(worker)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
def rpc(op, **values):
    request={"schemaVersion":1,"requestId":rpc.counter,"op":op,**values};rpc.counter+=1
    process.stdin.write((json.dumps(request,ensure_ascii=False)+"\n").encode());process.stdin.flush()
    # Quien invoca la prueba proporciona el tiempo límite del proceso completo.
    reply=json.loads(process.stdout.readline())
    assert reply["requestId"]==request["requestId"]
    assert reply["ok"],reply
    return reply["result"]
rpc.counter=1
try:
    data=rpc("open",source=str(source),output=str(output),site="QA-SYNTHETIC",operation="2",cache=str(root/"cache"))
    assert data["matched"]==4
    photo=data["items"][0]["id"]
    assert rpc("select",id=photo)["selection"]["preview"]["url"]
    form={"operation":"2","unitType":"Pozo","unit":"7","level":"1","view":"Foto sintética","notes":"Validación del Hub","subunitType":"Cuadro","subunit":"A"}
    filename=rpc("propose",id=photo,form=form)["proposedName"]
    saved=rpc("save",id=photo,form=form)
    assert saved["state"]["registered"]==1
    assert (output/"Entrega_Museo/Fotos"/filename).read_bytes()==next(source.iterdir()).read_bytes()
    report["frozenCatalog"]={"open":True,"preview":True,"save":True,"heif":data["state"]["heif"],"originalsUnchanged":True}
    rpc("close")
finally:
    process.stdin.close();assert process.wait(timeout=15)==0
    process.stdout.close();process.stderr.close()
exe=payload/"ArchaeoHub.exe"
def run(name, options):
    target=root/(name+".json")
    result=subprocess.run([str(exe),"--data-dir",str(root/(name+"-profile")),"--report",str(target),*options],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=40)
    data=json.loads(target.read_text(encoding="utf-8"))
    assert result.returncode==0 and data["ok"],(name,result.returncode,data)
    print(name,"OK",flush=True);return data
for theme,width,height in (("dark",1280,820),("light",1280,820),("dark",960,640),("light",960,640)):
    name=f"catalog-{theme}-{width}"
    report["ui"][name]=run(name,["--smoke-test","--route","tool/workbench.photo-catalog","--theme",theme,"--width",str(width),"--height",str(height),"--catalog-source",str(source),"--catalog-output",str(output),"--catalog-site","QA-SYNTHETIC","--catalog-operation","2","--catalog-first","--catalog-zoom-check","--screenshot",str(root/(name+".png"))])
for name,width,extra in (("catalog-grid",1280,["--catalog-grid"]),("catalog-wide",1600,["--catalog-first"])):
    report["ui"][name]=run(name,["--smoke-test","--route","tool/workbench.photo-catalog","--theme","dark","--width",str(width),"--height","900","--catalog-source",str(source),"--catalog-output",str(output),"--catalog-site","QA-SYNTHETIC","--catalog-operation","2",*extra,"--screenshot",str(root/(name+".png"))])
for theme,width,height in (("dark",1280,820),("light",1280,820),("dark",960,640)):
    name=f"transcription-{theme}-{width}"
    data=run(name,["--smoke-test","--route","tool/workbench.transcription","--theme",theme,"--width",str(width),"--height",str(height),"--screenshot",str(root/(name+".png"))])
    assert data["transcriptionReady"]
    report["ui"][name]=data
for theme,width,height in (("dark",1280,820),("light",960,640)):
    name=f"catalog-load-{theme}-{width}"
    data=run(name,["--smoke-test","--route","tool/workbench.photo-catalog","--theme",theme,"--width",str(width),"--height",str(height),"--catalog-source",str(source),"--catalog-project",str(output/"Entrega_Museo/Catalogo_fotografico_pozos.xlsx"),"--screenshot",str(root/(name+".png"))])
    assert data["catalogOpened"] and data["catalogSite"]=="QA-SYNTHETIC"
    report["ui"][name]=data
for theme,width,height in (("dark",1280,820),("light",1280,820),("dark",960,640)):
    name=f"coordinates-{theme}-{width}"
    report["ui"][name]=run(name,["--smoke-test","--route","tool/workbench.coordinates","--theme",theme,"--width",str(width),"--height",str(height),"--coordinate-demo","--screenshot",str(root/(name+".png"))])
for theme,width,height in (("dark",1280,820),("light",1280,820),("dark",960,640)):
    name=f"documents-{theme}-{width}"
    report["ui"][name]=run(name,["--smoke-test","--route","tool/workbench.document-summary","--theme",theme,"--width",str(width),"--height",str(height),"--screenshot",str(root/(name+".png"))])
library_source=root/"Documentos de prueba";library_source.mkdir()
(library_source/"Cerámica.md").write_text("# Contexto\nCerámica estratigrafía compartida. [[Estudio]]",encoding="utf-8")
(library_source/"Estudio.md").write_text("# Estudio\nCerámica estratigrafía compartida.",encoding="utf-8")
library_request=root/"library-request.json";library_request.write_text(json.dumps({"paths":[str(library_source)]}),encoding="utf-8")
for theme,width,height in (("dark",1280,820),("light",1280,820),("dark",960,640)):
    name=f"library-{theme}-{width}"
    data=run(name,["--smoke-test","--route","tool/workbench.document-summary","--theme",theme,"--width",str(width),"--height",str(height),"--document-library","--library-ui-request",str(library_request),"--screenshot",str(root/(name+".png"))])
    assert data["libraryReady"]==2 and data["libraryTopics"]>0
    report["ui"][name]=data
inventory_path=root/'Inventario sintético.ahinv'
def inventory_rpc(name,request):
    path=root/(name+'-request.json');path.write_text(json.dumps({'database':str(inventory_path),**request}),encoding='utf-8')
    return run(name,['--inventory-request',str(path)])
inventory_rpc('inventory-create',{'op':'create','metadata':{'project':'Proyecto sintético','site':'QA'}})
inventory_rpc('inventory-add',{'op':'add','record':{'operacion':'2','pozo':'7','nivel_std':'1-2','caja':'01','bolsa_grande':'A','bolsita':'01','cantidad_original':14,'q_cer':14}})
for theme,width,height in (("dark",1280,820),("light",1280,820),("dark",960,640)):
    name=f'inventory-{theme}-{width}'
    data=run(name,['--smoke-test','--route','tool/workbench.inventory','--theme',theme,'--width',str(width),'--height',str(height),'--inventory-open',str(inventory_path),'--screenshot',str(root/(name+'.png'))])
    assert data['inventoryOpened'] and data['inventoryRecords']==1
    report['ui'][name]=data
for theme,width,height in (("dark",1280,820),("light",960,640)):
    name=f'inventory-entry-{theme}-{width}'
    report['ui'][name]=run(name,['--smoke-test','--route','tool/workbench.inventory','--theme',theme,'--width',str(width),'--height',str(height),'--inventory-open',str(inventory_path),'--inventory-entry','--screenshot',str(root/(name+'.png'))])
assert originals=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in source.iterdir()}
report["hostSha256"]=hashlib.sha256(exe.read_bytes()).hexdigest()
report["ok"]=True
(root/"native-modules-validation.json").write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding="utf-8")
print("Native module validation passed.")
