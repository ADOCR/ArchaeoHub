"""Inventario de fuentes de proyectos seleccionados explícitamente, mediante lectura.

Es una utilidad de auditoría separada del futuro buscador del Hub. Requiere una
configuración local explícita; conserva rutas y volúmenes fuera del historial público.
"""
from __future__ import annotations

import argparse
import ast
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import stat

EXCLUDED = {
    ".git", ".venv", "venv", "env", "node_modules", "__pycache__",
    "build", "dist", "third_party", "external", "vendor", "vcpkg",
    "vcpkg_installed", "site-packages", "models", "modelos", "outputs",
    "training_data", ".ipynb_checkpoints", "instalador_generado",
}
SUFFIXES = {".py", ".cpp", ".h", ".hpp", ".qml", ".ps1", ".cmake", ".iss", ".nsi"}
MARKERS = {"cmakelists.txt", "requirements.txt", "pyproject.toml", "tool.json"}


def is_reparse(path: Path) -> bool:
    info = path.lstat()
    flag = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0)
    return path.is_symlink() or bool(getattr(info, "st_file_attributes", 0) & flag)


def inspect_source(path: Path, relative: str) -> dict:
    raw = path.read_bytes()
    item = {
        "path": relative,
        "bytes": len(raw),
        "sha256": hashlib.sha256(raw).hexdigest(),
        "lines": len(raw.splitlines()),
    }
    if path.suffix.lower() == ".py":
        try:
            tree = ast.parse(raw.decode("utf-8-sig"), filename=relative)
            imports, functions, tests = set(), 0, []
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    imports.update(alias.name.split(".")[0] for alias in node.names)
                elif isinstance(node, ast.ImportFrom) and node.module:
                    imports.add(node.module.split(".")[0])
                elif isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                    functions += 1
                    if node.name.startswith("test_"):
                        tests.append(node.name)
            item.update(imports=sorted(imports), functions=functions, test_names=tests)
        except (UnicodeError, SyntaxError) as error:
            item["parse_error"] = str(error)
    return item


def inspect_project(spec: dict, max_depth: int, max_files: int) -> dict:
    root = Path(spec["path"]).absolute()
    result = {"id": spec["id"], "root": str(root), "files": [], "errors": [], "omitted": []}
    if not root.is_dir() or is_reparse(root):
        result["errors"].append("Selected directory is unavailable or is a reparse point")
        return result

    def on_error(error: OSError) -> None:
        result["errors"].append(str(error))

    for current, directories, files in os.walk(root, followlinks=False, onerror=on_error):
        folder = Path(current)
        depth = len(folder.relative_to(root).parts)
        kept = []
        for name in sorted(directories):
            path = folder / name
            if spec.get("files") and not any(
                selected.startswith(path.relative_to(root).as_posix() + "/")
                for selected in spec["files"]
            ):
                continue
            try:
                excluded = name.lower() in EXCLUDED or is_reparse(path)
            except OSError as error:
                result["errors"].append(str(error))
                excluded = True
            if excluded or depth >= max_depth:
                result["omitted"].append(str(path.relative_to(root)))
            else:
                kept.append(name)
        directories[:] = kept
        for name in sorted(files):
            path = folder / name
            if spec.get("files") and path.relative_to(root).as_posix() not in spec["files"]:
                continue
            if path.suffix.lower() not in SUFFIXES and name.lower() not in MARKERS:
                continue
            if len(result["files"]) >= max_files:
                result["errors"].append(f"Source-file limit {max_files} reached; inventory incomplete")
                return result
            try:
                if is_reparse(path) or path.stat().st_size > 2_000_000:
                    result["omitted"].append(str(path.relative_to(root)))
                    continue
                result["files"].append(inspect_source(path, path.relative_to(root).as_posix()))
            except OSError as error:
                result["errors"].append(str(error))
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    config = json.loads(args.config.read_text(encoding="utf-8-sig"))
    max_depth = int(config.get("max_depth", 3))
    max_files = int(config.get("max_files_per_project", 1000))
    if not 0 <= max_depth <= 8 or not 1 <= max_files <= 10000:
        parser.error("Use depth 0..8 and 1..10000 source files per project")
    output = args.output.resolve()
    for spec in config["projects"]:
        root = Path(spec["path"]).resolve()
        if spec.get("files"):
            protected = [(root / relative).resolve() for relative in spec["files"]]
            overlaps = output in protected
        else:
            overlaps = output == root or root in output.parents
        if overlaps:
            parser.error("The report must not overwrite selected sources or enter a scanned tree")
    report = {
        "schema_version": 1,
        "created_at_utc": datetime.now(timezone.utc).isoformat(),
        "method": "static hashes and Python AST; no project code imported or executed",
        "limits": {"max_depth": max_depth, "max_files_per_project": max_files},
        "projects": [inspect_project(spec, max_depth, max_files) for spec in config["projects"]],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"projects": len(report["projects"]),
                      "source_files": sum(len(p["files"]) for p in report["projects"]),
                      "errors": sum(len(p["errors"]) for p in report["projects"]),
                      "python_parse_errors": sum(
                          "parse_error" in item for project in report["projects"]
                          for item in project["files"]),
                      "report": str(output)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
