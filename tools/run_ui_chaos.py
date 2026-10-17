"""Ejecuta estrés de interacción sintético dentro del proceso, con tiempo límite y estado de salida registrado."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--payload',type=Path,required=True)
parser.add_argument('--workdir',type=Path,required=True)
parser.add_argument('--iterations',type=int,default=1500)
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError('Use an empty QA directory')
exe=args.payload.resolve()/'ArchaeoHub.exe';report=root/'chaos.json'
env={k:v for k,v in os.environ.items() if k.upper() not in ('QT_PLUGIN_PATH','QML2_IMPORT_PATH','PYTHONPATH','PYTHONHOME')}
env['PATH']=os.environ['SystemRoot']+';'+str(Path(os.environ['SystemRoot'])/'System32')
env['GAMBOA_CONFIG_DIR']=str(root/'isolated-transcription')
start=time.monotonic()
with (root/'stdout.txt').open('wb') as out,(root/'stderr.txt').open('wb') as err:
 child=subprocess.Popen([str(exe),'--data-dir',str(root/'profile'),'--chaos-test','--chaos-iterations',str(args.iterations),'--report',str(report)],env=env,stdout=out,stderr=err)
 try:code=child.wait(timeout=300);timed_out=False
 except subprocess.TimeoutExpired:
  subprocess.run(['taskkill','/PID',str(child.pid),'/T','/F'],capture_output=True);code=child.wait(timeout=15);timed_out=True
result={'ok':code==0 and report.exists(),'exitCode':code,'timeout':timed_out,'seconds':round(time.monotonic()-start,2),'hostSha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'pid':child.pid,'iterationsRequested':args.iterations}
if report.exists():result['session']=json.loads(report.read_text(encoding='utf-8'));result['ok']&=result['session']['ok']
(root/'launcher-validation.json').write_text(json.dumps(result,indent=2,ensure_ascii=False),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False),flush=True)
raise SystemExit(0 if result['ok'] else 1)
