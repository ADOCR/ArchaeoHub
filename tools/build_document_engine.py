"""Empaqueta pypdf instalado y lectores DOCX/Markdown de la biblioteca estándar, sin OCR ni suites ofimáticas."""
from pathlib import Path
import subprocess
import shutil
import sys
import importlib.metadata as metadata
import hashlib,json
root=Path(__file__).resolve().parents[1]
sources=sorted((root/"modules/documents").glob("*.py"))
source_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
subprocess.run([sys.executable,"-m","PyInstaller","--noconfirm","--onedir","--console","--name","DocumentWorker","--contents-directory","_internal","--distpath",str(root/"build/document-freeze"),"--workpath",str(root/".local/document-freeze/work"),"--specpath",str(root/".local/document-freeze"),"--exclude-module","numpy","--exclude-module","PIL","--exclude-module","pandas","--exclude-module","cryptography","--exclude-module","Crypto",str(root/"modules/documents/document_worker.py")],check=True,cwd=root)
shutil.copytree(root/"build/document-freeze/DocumentWorker",root/"build/modules/documents/engine",dirs_exist_ok=True)
assert source_hashes=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},"Sources changed during freezing"
(root/"build/modules/documents/engine/ORIGIN.json").write_text(json.dumps({"schemaVersion":1,"sources":source_hashes,"purpose":"Bundled local document reader and grounded analysis"},indent=2)+"\n",encoding="utf-8")
licenses=root/"resources/licenses/documents";licenses.mkdir(parents=True,exist_ok=True)
package=metadata.distribution("pypdf")
for file in package.files or []:
    if "LICENSE" in str(file).upper() or "NOTICE" in str(file).upper():
        source=Path(package.locate_file(file))
        if source.is_file():shutil.copy2(source,licenses/source.name)
(licenses/"pypdf-version.txt").write_text(f"pypdf {package.version}\nhttps://github.com/py-pdf/pypdf\n",encoding="utf-8")
