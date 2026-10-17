"""Prueba la conversión independiente de PDF/Word y la vista QML real de fichas de lectura."""
from pathlib import Path
import argparse,json,os,subprocess,sys,time
root_source=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root_source/"tests"))
from document_worker_tests import create_pdf,create_docx
import hashlib
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--payload',type=Path,required=True);parser.add_argument('--workdir',type=Path,required=True)
parser.add_argument('--reading-preview',action='store_true')
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError('Use a new isolated QA directory')
source=root/'Fuentes ñ';source.mkdir();destination=root/'Markdown';destination.mkdir()
create_pdf(source/'Texto.pdf','There are 14 ceramic fragments in pit 7.')
create_pdf(source/'Escaneo.pdf','');create_docx(source/'Word.docx')
md=source/'Mi nota.md';md.write_text('USER MARKDOWN: keep unchanged')
before={p.name:sha(p) for p in source.iterdir()}
request=root/'conversion-request.json';request.write_text(json.dumps({'paths':[str(source)],'conversionOnly':True,'destination':str(destination)}),encoding='utf-8')
exe=args.payload.resolve()/'ArchaeoHub.exe'
env={k:v for k,v in os.environ.items() if k.upper() not in ('PYTHONPATH','PYTHONHOME','QT_PLUGIN_PATH','QML2_IMPORT_PATH')}
env['PATH']=os.environ['SystemRoot']+';'+str(Path(os.environ['SystemRoot'])/'System32')
env['GAMBOA_CONFIG_DIR']=str(root/'transcription-isolated')
def run(name,extra,timeout=180):
    reply=root/(name+'.json')
    result=subprocess.run([str(exe),'--data-dir',str(root/'profile'),'--report',str(reply),*extra],env=env,capture_output=True,timeout=timeout)
    value=json.loads(reply.read_text(encoding='utf-8'));assert result.returncode==0 and value['ok'],(name,result.returncode,value)
    return value
converted=run('conversion',['--library-request',str(request)])
assert converted['ready']==2 and converted['failed']==1 and not converted['research'] and not converted['connections'],converted
folder=Path(converted['folder']);assert len(list((folder/'Documentos').glob('*.md')))==2
assert not (folder/'Fichas').exists() and not list((folder/'Temas').iterdir())
assert before=={p.name:sha(p) for p in source.iterdir()}
ui={}
for name,theme,width,height in [('converter-dark','dark',1280,820),('converter-light','light',1280,820),('converter-compact','dark',960,640)]:
    ui[name]=run(name,['--smoke-test','--route','tool/workbench.document-summary','--library-ui-request',str(request),'--theme',theme,'--width',str(width),'--height',str(height),'--screenshot',str(root/(name+'.png'))])
    assert not ui[name]['qmlWarnings']
ui['library-compact']=run('library-compact',['--smoke-test','--route','tool/workbench.document-summary','--document-library','--theme','dark','--width','960','--height','640','--screenshot',str(root/'library-compact.png')])
assert not ui['library-compact']['qmlWarnings']
if args.reading_preview:
    theory=root/'Método.md';theory.write_text('El muestreo aleatorio simple asigna igual probabilidad a cada unidad. La selección aleatoria limita el sesgo de elegir únicamente depósitos visibles. No garantiza representar todos los depósitos ni elimina errores de medición.',encoding='utf-8')
    reading=root/'reading.json';reading.write_text(json.dumps({'paths':[str(theory)],'research':True,'model':'qwen3:8b'}),encoding='utf-8')
    ui['reading-preview']=run('reading-preview',['--smoke-test','--route','tool/workbench.document-summary','--library-ui-request',str(reading),'--theme','dark','--width','1280','--height','820','--screenshot',str(root/'reading-preview.png')],900)
    assert not ui['reading-preview']['qmlWarnings']
report={'ok':True,'syntheticOnly':True,'conversionFiles':2,'ocrPending':1,'markdownIgnoredAndPreserved':True,
        'researchNotGeneratedByConverter':True,'originalsUnchanged':True,'uiScenarios':len(ui),'ui':ui,
        'readingPreview':args.reading_preview,'hostSha256':sha(exe),'workerSha256':sha(args.payload/'modules/documents/engine/DocumentWorker.exe')}
(root/'document-workspaces-validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'ok':True,'uiScenarios':len(ui),'conversionFiles':2,'readingPreview':args.reading_preview}),flush=True)
