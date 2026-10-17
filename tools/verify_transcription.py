"""Valida el motor C++ de transcripción incluido con voz sintética generada localmente."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import queue
import subprocess
import threading
import time
import zipfile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload",type=Path,required=True);parser.add_argument("--models",type=Path,required=True)
    parser.add_argument("--audio",type=Path,required=True);parser.add_argument("--workdir",type=Path,required=True)
    parser.add_argument("--minutes-only",action="store_true",help="Verify one transcription and local grounded minutes")
    args=parser.parse_args();root=args.workdir.resolve();root.mkdir(parents=True,exist_ok=True)
    if any(root.iterdir()):raise ValueError("Use an empty isolated verification directory")
    before=hashlib.sha256(args.audio.read_bytes()).hexdigest()
    env={k:v for k,v in os.environ.items() if k not in ("PYTHONPATH","PYTHONHOME","QT_PLUGIN_PATH","QML2_IMPORT_PATH")}
    system=os.environ["SystemRoot"];env["PATH"]=system+";"+str(Path(system)/"System32")
    env["GAMBOA_CONFIG_DIR"]=str(root/"configuration");env["ARCHAEOHUB_MODELS_DIR"]=str(args.models.resolve())
    exe=args.payload.resolve()/"modules/transcription/engine/TranscriptionWorker.exe"
    events=queue.Queue();diagnostics=root/"worker.stderr.txt"
    with diagnostics.open("wb") as errors:
        process=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,env=env)
        def reader():
            for line in process.stdout:
                if line.startswith(b"@ARCHAEOHUB@"):
                    try:events.put(json.loads(line[len(b"@ARCHAEOHUB@"):]))
                    except Exception as exc:events.put(exc)
            events.put(RuntimeError("Worker exited"))
        thread=threading.Thread(target=reader,daemon=True);thread.start()
        def send(op,**values):process.stdin.write((json.dumps({"schemaVersion":1,"op":op,**values},ensure_ascii=False)+"\n").encode());process.stdin.flush()
        def wait(predicate,timeout=240):
            deadline=time.monotonic()+timeout
            while time.monotonic()<deadline:
                event=events.get(timeout=max(.1,deadline-time.monotonic()))
                if isinstance(event,Exception):raise event
                if predicate(event):return event
            raise TimeoutError("Transcription worker did not finish")
        report={"ok":False,"engineSha256":hashlib.sha256(exe.read_bytes()).hexdigest(),"audioSha256":before,"models":{},"syntheticAudio":True,"networkUsed":False}
        try:
            initial=wait(lambda e:e.get("type")=="state",30)["state"]
            assert len(initial["models"])==4
            for index in ((0,) if args.minutes_only else (0,1,2)):
                assert initial["models"][index]["ready"],initial["models"][index]
                target=root/f"model-{index}";target.mkdir()
                send("clear");send("settings",modelIndex=index,language="es",device="cpu",createMinutes=args.minutes_only,ollamaModel="qwen3:8b",outputFolder=str(target))
                send("add",paths=[str(args.audio.resolve())]);wait(lambda e:len(e.get("state",{}).get("files",[]))==1 and e["state"]["modelIndex"]==index,30)
                start=time.monotonic();send("start")
                finished=wait(lambda e:e.get("type")=="finished")
                assert finished["ok"],finished
                state=finished["state"];assert state["transcript"].strip() and state["segments"]
                folder=Path(state["resultFolder"]);assert folder.is_relative_to(target)
                for suffix in (".txt",".srt",".vtt",".docx"):
                    files=list(folder.glob("*"+suffix));assert files and all(p.stat().st_size>0 for p in files)
                with zipfile.ZipFile(next(folder.glob("*.docx"))) as archive:assert "word/document.xml" in archive.namelist()
                last=0
                for segment in state["segments"]:
                    assert 0<=segment["start"]<=segment["end"] and segment["start"]>=last;last=segment["start"]
                report["models"][str(index)]={"name":initial["models"][index]["name"],"seconds":round(time.monotonic()-start,2),"segments":len(state["segments"]),"text":state["transcript"],"exports":True}
                if args.minutes_only:
                    assert state["minutes"].strip()
                    quotes=[line[2:].strip() for line in state["minutes"].splitlines() if line.startswith("• ")]
                    assert quotes and all(quote in " ".join(state["transcript"].split()) for quote in quotes)
                    report["models"][str(index)]["minutesLiteralQuotesVerified"]=len(quotes)
                print(json.dumps(report["models"][str(index)],ensure_ascii=False),flush=True)
            send("settings",modelIndex=0);wait(lambda e:e.get("state",{}).get("modelIndex")==0,30)
            send("start");wait(lambda e:e.get("state",{}).get("busy"),30);send("cancel")
            cancelled=wait(lambda e:e.get("type")=="finished")
            assert not cancelled["ok"] and not cancelled["state"]["busy"]
            report["cancelledWithoutLosingPreviousResults"]=True
            assert hashlib.sha256(args.audio.read_bytes()).hexdigest()==before
            report["originalAudioUnchanged"]=True;report["ok"]=True
            (root/"transcription-validation.json").write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding="utf-8")
            send("close");assert process.wait(timeout=15)==0
        finally:
            if process.poll() is None:process.kill();process.wait(timeout=10)
            process.stdin.close();process.stdout.close()
    print(json.dumps({"ok":True,"report":str(root/"transcription-validation.json")}))

if __name__=="__main__":main()
