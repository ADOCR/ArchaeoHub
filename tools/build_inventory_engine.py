"""Empaqueta el dominio aislado de inventario SQLite/openpyxl con las dependencias existentes."""
from pathlib import Path
import subprocess,sys,shutil
root=Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,'-m','PyInstaller','--noconfirm','--onedir','--console','--name','InventoryWorker','--contents-directory','_internal','--distpath',str(root/'build/inventory-freeze'),'--workpath',str(root/'.local/inventory-freeze/work'),'--specpath',str(root/'.local/inventory-freeze'),'--exclude-module','numpy','--exclude-module','pandas','--exclude-module','PIL',str(root/'modules/inventory/inventory_worker.py')],check=True,cwd=root)
shutil.copytree(root/'build/inventory-freeze/InventoryWorker',root/'build/modules/inventory/engine',dirs_exist_ok=True)
