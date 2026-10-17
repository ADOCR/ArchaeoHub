"""Empaqueta el dominio conservado del catálogo con su Python de desarrollo, fuera del arranque de la aplicación."""
from pathlib import Path
import json
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
source = root / "modules/catalog/python"
work = root / ".local/catalog-freeze"
distribution = root / "build/catalog-freeze"
work.mkdir(parents=True, exist_ok=True)
command = [sys.executable, "-m", "PyInstaller", "--noconfirm", "--onedir", "--console",
           "--name", "CatalogWorker", "--contents-directory", "_internal",
           "--paths", str(source), "--collect-all", "pillow_heif",
           "--distpath", str(distribution), "--workpath", str(work / "work"), "--specpath", str(work),
           "--exclude-module", "tkinter", "--exclude-module", "PIL.ImageTk",
           "--exclude-module", "numpy", "--exclude-module", "pandas", "--exclude-module", "matplotlib",
           str(source / "catalog_worker.py")]
subprocess.run(command, cwd=root, check=True)
destination = root / "build/modules/catalog/engine"
shutil.copytree(distribution / "CatalogWorker", destination, dirs_exist_ok=True)
shutil.copy2(source / "ORIGIN.json", destination / "ORIGIN.json")
(destination / "engine.json").write_text(json.dumps({"protocol": 1, "domainVersion": "2.4.5", "python": sys.version.split()[0], "network": False}, indent=2), encoding="utf-8")
print(f"Catalog engine staged: {destination}")
