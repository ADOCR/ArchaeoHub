"""Lee un catálogo existente sin crear archivos, adquirir bloqueos ni migrarlo."""
from pathlib import Path
import json
import os
from catalog_workbook import load_workbook
from catalog_core import FIELDS, FIELDS_V4, LEGACY_FIELDS_V3


def inspect_project(location, source_hint="", site_hint="", operation_hint=""):
    selected = Path(location).resolve()
    if selected.is_file():
        if selected.name == "Catalogo_fotografico_pozos.xlsx" and selected.parent.name == "Entrega_Museo":
            output = selected.parent.parent
        elif selected.name == "estado_catalogador.json":
            output = selected.parent
        else:
            raise ValueError("Selecciona Catalogo_fotografico_pozos.xlsx o estado_catalogador.json del catálogo.")
    elif selected.is_dir():
        output = selected.parent if selected.name == "Entrega_Museo" else selected
    else:
        raise ValueError("El catálogo seleccionado no está disponible. Revisa su unidad.")
    xlsx = output / "Entrega_Museo" / "Catalogo_fotografico_pozos.xlsx"
    if not xlsx.is_file():
        raise ValueError("No se encontró el Excel del catálogo en Entrega_Museo. Cargar no crea un catálogo nuevo.")
    workbook = load_workbook(xlsx, read_only=True, data_only=True)
    try:
        headers = [c.value for c in workbook.active[1]]
        if headers not in (FIELDS, FIELDS_V4, LEGACY_FIELDS_V3):
            raise ValueError("El Excel existente tiene columnas diferentes.")
        sites, operations, rows = set(), set(), 0
        for values in workbook.active.iter_rows(min_row=2, values_only=True):
            if not any(v is not None for v in values):
                continue
            rows += 1
            if values[0]: sites.add(str(values[0]).strip())
            if "Operacion" in headers and values[1]: operations.add(str(values[1]).strip())
    finally:
        workbook.close()
    if len(sites) > 1:
        raise ValueError("El Excel contiene varios códigos de sitio; revísalo antes de abrirlo.")
    state_file = output / "estado_catalogador.json"
    paths = []
    if state_file.is_file():
        if state_file.stat().st_size > 64 * 1024 * 1024: raise ValueError("El estado del catálogo supera 64 MB.")
        state = json.loads(state_file.read_text(encoding="utf-8-sig"))
        if not isinstance(state, dict): raise ValueError("El estado del catálogo no es válido.")
        completed, skipped = state.get("processed_paths", []), state.get("skipped_paths", [])
        if not isinstance(completed, list) or not isinstance(skipped, list):
            raise ValueError("El estado contiene rutas no válidas.")
        paths = completed + skipped
        if any(not isinstance(p, str) for p in paths):
            raise ValueError("El estado contiene rutas no válidas.")
    elif rows:
        raise ValueError("Falta estado_catalogador.json; conserva el Excel junto a la carpeta de trabajo original para reanudar sin repetir fotos.")
    source = Path(source_hint).resolve() if str(source_hint).strip() else None
    # Un directorio padre común sólo es una sugerencia; no demuestra la raíz de búsqueda original.
    valid_source = source is not None and source.is_dir() and source != output and not source.is_relative_to(output)
    if valid_source and paths:
        valid_source = all(Path(p).is_absolute() and Path(p).resolve().is_relative_to(source) for p in paths)
    suggestion = ""
    if paths and all(Path(p).is_absolute() for p in paths):
        try:
            parent = Path(os.path.commonpath([str(Path(p).parent) for p in paths]))
            if parent.is_dir() and parent != output and not parent.is_relative_to(output): suggestion = str(parent)
        except ValueError:
            pass
    site = next(iter(sites)) if sites else str(site_hint).strip()
    operation = next(iter(operations)) if len(operations) == 1 else ""
    if headers == LEGACY_FIELDS_V3: operation = str(operation_hint).strip()
    return {"location": str(output), "output": str(output), "source": str(source) if valid_source else "",
            "suggestedSource": suggestion, "site": site, "operation": operation,
            "rows": rows, "operations": sorted(operations), "needsSource": not valid_source,
            "needsSite": not bool(site), "needsOperation": headers == LEGACY_FIELDS_V3 and not bool(operation),
            "legacy": headers != FIELDS}
