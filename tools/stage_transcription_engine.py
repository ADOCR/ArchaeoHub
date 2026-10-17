"""Prepara el runtime nativo completo de inferencia sin copiar pesos de modelos del usuario."""
from pathlib import Path
import argparse
import json
import shutil
import subprocess
import sys

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument("--runtime",type=Path,required=True)
args=parser.parse_args();root=Path(__file__).resolve().parents[1];runtime=args.runtime.resolve()
destination=root/"build/modules/transcription/engine";destination.mkdir(parents=True,exist_ok=True)
shutil.copy2(root/"build/TranscriptionWorker.exe",destination/"TranscriptionWorker.exe")
for name in ("ggml-base.dll","ggml.dll","ggml-cpu.dll","ggml-cuda.dll","whisper.dll","onnxruntime.dll","onnxruntime_providers_shared.dll","cublas64_13.dll","cublasLt64_13.dll","cudart64_13.dll"):
    shutil.copy2(runtime/name,destination/name)
tools=destination/"tools";tools.mkdir(exist_ok=True);shutil.copy2(runtime/"tools/ffmpeg.exe",tools/"ffmpeg.exe")
shutil.copytree(runtime/"platforms",destination/"platforms",dirs_exist_ok=True)
for folder in ("licenses","tls","networkinformation"):
    if (runtime/folder).is_dir():shutil.copytree(runtime/folder,destination/folder,dirs_exist_ok=True)
shutil.copy2(runtime/"THIRD_PARTY.md",destination/"THIRD_PARTY.md")
(destination/"qt.conf").write_text("[Paths]\nPrefix=.\nPlugins=.\n",encoding="ascii")
subprocess.run([sys.executable,str(root/"tools/copy_pe_dependencies.py"),str(destination),str(runtime)],check=True)
(destination/"engine.json").write_text(json.dumps({"protocol":1,"baseVersion":"3.0.0","modelWeightsIncluded":False,"localProcessing":True,"source":"modules/transcription/native/ORIGIN.json"},indent=2),encoding="utf-8")
print(destination)
