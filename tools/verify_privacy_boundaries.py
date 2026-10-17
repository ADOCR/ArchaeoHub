"""Pruebas limitadas a conexión local: metadatos de interfaz, URLs multimedia, instancias duplicadas y recuperación."""
import argparse,base64,hashlib,json,os,sqlite3,subprocess,threading,time
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--payload',required=True,type=Path);parser.add_argument('--workdir',required=True,type=Path);parser.add_argument('--baseline',action='store_true')
args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
if any(root.iterdir()):raise ValueError('Use an empty QA directory')
payload=args.payload.resolve();exe=payload/'ArchaeoHub.exe';hits=[]
class Handler(BaseHTTPRequestHandler):
 def do_GET(self):
  hits.append(self.path);data=base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+j0GkAAAAASUVORK5CYII=')
  self.send_response(200);self.send_header('Content-Type','image/png');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
 def log_message(self,*args):pass
server=ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start();base=f'http://127.0.0.1:{server.server_port}'
env={k:v for k,v in os.environ.items() if k.upper() not in ('PYTHONPATH','PYTHONHOME','QT_PLUGIN_PATH','QML2_IMPORT_PATH')};env['PATH']=os.environ['SystemRoot']+';'+str(Path(os.environ['SystemRoot'])/'System32');env['GAMBOA_CONFIG_DIR']=str(root/'transcription')
report={'baseline':args.baseline,'hostSha256':hashlib.sha256(exe.read_bytes()).hexdigest()}
try:
 profile=root/'metadata-profile';manifest=profile/'tool-manifests';manifest.mkdir(parents=True)
 (manifest/'probe.json').write_text(json.dumps({'schemaVersion':1,'id':'qa.rich-text','version':'1.0.0','name':{'es':f'<img src="{base}/metadata.png">'},'description':{'es':'Synthetic metadata, never executable'},'runtime':'external','entrypoint':{'binding':'selected-installation'},'categoryIds':['utilities'],'capabilities':['launch'],'supportedFileTypes':[]}),encoding='utf-8')
 target=root/'metadata-report.json';command=[str(exe),'--data-dir',str(profile),'--smoke-test','--route','tools','--report',str(target)]
 if not args.baseline:command+=['--ui-network-probe',base+'/explicit-probe']
 result=subprocess.run(command,env=env,capture_output=True,timeout=40);assert result.returncode==0,(result.returncode,result.stderr)
 data=json.loads(target.read_text(encoding='utf-8'));report['metadataHttpRequests']=len(hits);report['uiNetworkRequestsBlocked']=data.get('uiNetworkRequestsBlocked',0)
 if not args.baseline:assert not hits and report['uiNetworkRequestsBlocked']>=1,report
 hits.clear();playlist=root/'Hostile.m3u8';playlist.write_text('#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:1\n#EXT-X-MEDIA-SEQUENCE:0\n#EXTINF:1,\n'+base+'/segment.ts\n#EXT-X-ENDLIST\n',encoding='utf-8')
 ffmpeg=payload/'modules/transcription/engine/tools/ffmpeg.exe';command=[str(ffmpeg),'-v','error','-nostdin']
 if not args.baseline:command+=['-protocol_whitelist','file,pipe']
 command+=['-i',str(playlist),'-vn','-ar','16000','-ac','1','-f','f32le','pipe:1']
 result=subprocess.run(command,env=env,capture_output=True,timeout=15);report['localMediaHttpRequests']=len(hits);report['mediaRejected']=result.returncode!=0
 (root/'media-diagnostics.txt').write_bytes(result.stderr)
 if not args.baseline:assert not hits and result.returncode!=0 and b'whitelist' in result.stderr,report
 if not args.baseline:
  profile=root/'instance-profile';first=subprocess.Popen([str(exe),'--data-dir',str(profile),'--route','home'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  try:
   log=profile/'logs/workbench.jsonl';deadline=time.monotonic()+20
   while time.monotonic()<deadline:
    if log.exists() and 'app.ready' in log.read_text(encoding='utf-8'):break
    assert first.poll() is None;time.sleep(.05)
   else:raise TimeoutError('First instance did not initialize')
   db=profile/'workbench.sqlite'
   with sqlite3.connect(db) as connection:connection.execute('INSERT INTO tasks VALUES(?,?,?,?,?,?,?,?)',('qa-live-task','qa.live','Synthetic running task','running',0,'','','2026-10-16T00:00:00Z'))
   target=root/'duplicate-command.json'
   second=subprocess.run([str(exe),'--data-dir',str(profile).upper(),'--self-check','--report',str(target)],env=env,capture_output=True,timeout=10)
   assert second.returncode==12 and json.loads(target.read_text())['code']=='profile.in_use'
   target=root/'duplicate-window.json';second=subprocess.run([str(exe),'--data-dir',str(profile),'--report',str(target)],env=env,capture_output=True,timeout=10)
   assert second.returncode==0 and json.loads(target.read_text())['alreadyRunning']
   with sqlite3.connect(db) as connection:assert connection.execute('SELECT state FROM tasks WHERE id=?',('qa-live-task',)).fetchone()[0]=='running'
   report['duplicateProfileGuardAndActivation']=True
  finally:
   first.terminate();first.wait(timeout=10)
  target=root/'recovered.json';third=subprocess.run([str(exe),'--data-dir',str(profile),'--self-check','--report',str(target)],env=env,capture_output=True,timeout=20)
  assert third.returncode==0,third.stderr
  with sqlite3.connect(db) as connection:
   assert connection.execute('PRAGMA integrity_check').fetchone()[0]=='ok'
   assert connection.execute('SELECT state FROM tasks WHERE id=?',('qa-live-task',)).fetchone()[0]=='interrupted'
  report['staleLockAndSqliteRecoveredAfterTestProcessTermination']=True
  isolated_env=dict(env);isolated_env.pop('GAMBOA_CONFIG_DIR',None)
  isolated=root/'native-profile';target=root/'native-profile.json'
  result=subprocess.run([str(exe),'--data-dir',str(isolated),'--smoke-test','--route','tool/workbench.transcription','--report',str(target)],env=isolated_env,capture_output=True,timeout=30)
  assert result.returncode==0 and json.loads(target.read_text())['transcriptionReady']
  assert (isolated/'transcription/configuration').is_dir()
  report['alternateProfileIsolatesLegacyTranscriptionConfiguration']=True
 report['ok']=not report['metadataHttpRequests'] and not report['localMediaHttpRequests']
 (root/'privacy-validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(report),flush=True)
finally:server.shutdown();server.server_close()
