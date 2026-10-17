"""Empaqueta openpyxl como un auxiliar XLSX independiente que se carga al utilizarlo."""
from pathlib import Path
import subprocess
import sys
import shutil
root = Path(__file__).resolve().parents[1]
subprocess.run([sys.executable, "-m", "PyInstaller", "--noconfirm", "--onedir", "--console", "--name", "SpreadsheetWorker", "--contents-directory", "_internal", "--distpath", str(root / "build/spreadsheet-freeze"), "--workpath", str(root / ".local/spreadsheet-freeze/work"), "--specpath", str(root / ".local/spreadsheet-freeze"), "--exclude-module", "numpy", "--exclude-module", "pandas", "--exclude-module", "PIL", str(root / "modules/coordinates/spreadsheet_worker.py")], check=True, cwd=root)
shutil.copytree(root / "build/spreadsheet-freeze/SpreadsheetWorker", root / "build/modules/coordinates/spreadsheets", dirs_exist_ok=True)
