"""Procesa localmente el libro que fallaba y conserva los resultados privados en QA."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,time,sys,re
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--payload',type=Path,required=True);parser.add_argument('--workdir',type=Path,required=True);parser.add_argument('--source',type=Path,required=True)
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError('Use an empty QA directory')
source=args.source.resolve();sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();before=sha(source);stat=source.stat()
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'modules/documents'))
from document_worker import extract
extracted=extract(source);assert extracted['characters']>200000
request=root/'request.json';request.write_text(json.dumps({'model':'qwen3:8b','paths':[str(source)],'detailed':True}),encoding='utf-8')
env={k:v for k,v in os.environ.items() if k.upper() not in ('PYTHONHOME','PYTHONPATH','QT_PLUGIN_PATH','QML2_IMPORT_PATH')}
env['PATH']=os.environ['SystemRoot']+';'+str(Path(os.environ['SystemRoot'])/'System32');env['GAMBOA_CONFIG_DIR']=str(root/'transcription-profile')
exe=args.payload.resolve()/'ArchaeoHub.exe';reply=root/'response.json';output=root/'Resumen privado.md';start=time.monotonic()
print(f"Starting local full-book summary: {extracted['characters']} characters, {len(extracted['blocks'])} page blocks.",flush=True)
process=subprocess.run([str(exe),'--data-dir',str(root/'profile'),'--document-request',str(request),'--document-export',str(output),'--report',str(reply)],env=env,capture_output=True,timeout=3650)
data=json.loads(reply.read_text(encoding='utf-8'));assert process.returncode==0 and data['ok'],(process.returncode,data)
summary=data['summary'];assert len(summary)>500 and 'D1' in summary and source.name in summary and '<think>' not in summary
assert output.read_text(encoding='utf-8')==summary and sha(source)==before and source.stat().st_mtime_ns==stat.st_mtime_ns
report={'ok':True,'sourceSha256':before,'sourceCharacters':extracted['characters'],'pageBlocks':len(extracted['blocks']),'model':data['model'],'seconds':round(time.monotonic()-start,2),'summaryCharacters':len(summary),'originalUnchanged':True,'exportMatches':True,'sourceListPresent':True,'pageCitationsPresent':bool(re.search(r'\[D\d+:',summary)),'generatedFactsValidated':False,'localOnly':True,'hostSha256':sha(exe),'workerSha256':sha(args.payload.resolve()/'modules/documents/engine/DocumentWorker.exe'),'limits':'Full read and completion verified; generated factual assertions need human review. Source list does not prove claim-by-claim page citations. Private summary not distributed.'}
(root/'long-document-validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report),flush=True)
