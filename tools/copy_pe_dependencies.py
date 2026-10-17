"""Resuelve dependencias de la distribución Windows sin copiar DLL del sistema operativo."""
from pathlib import Path
import argparse
import os
import shutil
import pefile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination",type=Path)
    parser.add_argument("search",type=Path,nargs="+")
    parser.add_argument("--exclude-tree",action="append",default=[],help="Separate engine bundles resolved in their own DLL context")
    args=parser.parse_args()
    destination=args.destination.resolve();system=Path(os.environ.get("SystemRoot",r"C:\Windows"))/"System32"
    excluded=[(destination/p).resolve() for p in args.exclude_tree]
    queue=[p for p in destination.rglob("*.dll") if not any(p.resolve().is_relative_to(root) for root in excluded)]+list(destination.glob("*.exe"));visited=set();missing=set()
    while queue:
        path=queue.pop()
        if path in visited:continue
        visited.add(path)
        try:
            with pefile.PE(str(path),fast_load=True) as pe:
                pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"],pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT"]])
                imports={entry.dll.decode("ascii") for field in ("DIRECTORY_ENTRY_IMPORT","DIRECTORY_ENTRY_DELAY_IMPORT") for entry in getattr(pe,field,[])}
        except pefile.PEFormatError:continue
        for name in imports:
            if name.lower().startswith(("api-ms-","ext-ms-")):continue
            target=destination/name
            if target.exists():continue
            candidate=next((folder/name for folder in args.search if (folder/name).is_file()),None)
            if candidate is None and (system/name).exists():
                if name.lower().startswith(("vcruntime","msvcp","vcomp","concrt")):candidate=system/name
                else:continue
            if candidate is None:missing.add(name);continue
            shutil.copy2(candidate,target);queue.append(target)
    if missing:raise RuntimeError("Unresolved deployment imports: "+", ".join(sorted(missing)))
    print(f"Checked {len(visited)} PE files; all imports resolved")

if __name__=="__main__":main()
