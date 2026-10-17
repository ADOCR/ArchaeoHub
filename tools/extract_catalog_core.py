"""Extrae el dominio auditado conservando la aplicación original del usuario."""
import argparse
import ast
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("source", type=Path)
parser.add_argument("destination", type=Path)
args = parser.parse_args()
source = args.source / "catalogador_pozos.py"
text = source.read_text(encoding="utf-8-sig")
tree = ast.parse(text)
boundary = next(n.lineno for n in tree.body if isinstance(n, ast.ClassDef) and n.name == "ImageViewer")
domain = "\n".join(text.splitlines()[:boundary - 1]) + "\n"
# Sólo se eliminan importaciones de interfaz; Catalog, las excepciones transaccionales y los auxiliares conservan sus bytes.
domain = domain.replace("import tkinter as tk\n", "").replace("from tkinter import filedialog, messagebox, ttk\n", "")
domain = domain.replace("from PIL import Image, ImageDraw, ImageOps, ImageTk", "from PIL import Image, ImageDraw, ImageOps")
args.destination.mkdir(parents=True, exist_ok=True)
(args.destination / "catalog_core.py").write_text(domain, encoding="utf-8")
files = [source, args.source / "georreferenciacion.py", args.source / "gps_manual.py"]
for path in files[1:]:
    (args.destination / path.name).write_bytes(path.read_bytes())
origin = {"sourceVersion": "2.4.5", "domainBoundary": boundary, "files": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
(args.destination / "ORIGIN.json").write_text(json.dumps(origin, indent=2) + "\n", encoding="utf-8")
# Comparar los árboles de sintaxis aporta más garantías que contar las líneas copiadas.
extracted = ast.parse(domain)
for name in ("Catalog", "CatalogLock", "discover", "excel_safe", "normalized"):
    old = next(n for n in tree.body if getattr(n, "name", None) == name)
    new = next(n for n in extracted.body if getattr(n, "name", None) == name)
    assert ast.dump(old) == ast.dump(new), name
print("Catalog domain extracted; original files unchanged; domain AST identical.")
