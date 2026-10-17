"""Copia los avisos de los paquetes de desarrollo exactos usados para empaquetar el motor privado."""
from importlib.metadata import distribution
from pathlib import Path
import json
import shutil
import sys

root=Path(__file__).resolve().parents[1]
destination=root/"resources/licenses/catalog"
destination.mkdir(parents=True,exist_ok=True)
inventory={}
for name in ("Pillow","openpyxl","pillow-heif","et-xmlfile","PyInstaller"):
    package=distribution(name);copied=[]
    for relative in package.files or []:
        if any(token in str(relative).lower() for token in ("license","licence","copying")) and ".dist-info/" in relative.as_posix():
            source=Path(package.locate_file(relative))
            if source.is_file():
                target=destination/name/source.name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target);copied.append(target.relative_to(destination).as_posix())
    inventory[name]={"version":package.version,"notices":copied}
python_license=Path(sys.base_prefix)/"LICENSE.txt"
if python_license.is_file():shutil.copy2(python_license,destination/"Python-LICENSE.txt")
inventory["Python"]={"version":sys.version.split()[0]}
(destination/"inventory.json").write_text(json.dumps(inventory,indent=2)+"\n",encoding="utf-8")
print(json.dumps(inventory))
