"""Adaptador JSONL local, sin interfaz Tk, red, intérprete de comandos ni instalación automática de dependencias."""
from __future__ import annotations

import dataclasses
import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageOps
from catalog_workbook import load_workbook
from catalog_core import (
    Catalog, CatalogLock, FIELDS, LEGACY_FIELDS_V3, FIELDS_V4,
    PHOTO_FILTERS, UNIT_TYPES, SUBUNIT_TYPES, HEIC_ENABLED,
    SaveCommittedWarning, CorrectionCommittedWarning, WithdrawalCommittedWarning,
    DiscardCommittedWarning, normalized,
)
from georreferenciacion import read_gps, export_gps_report
from catalog_project import inspect_project

COMMITTED = (SaveCommittedWarning, CorrectionCommittedWarning,
             WithdrawalCommittedWarning, DiscardCommittedWarning)


class Session:
    def __init__(self):
        self.catalog = None
        self.lock = None
        self.paths = {}
        self.cache = None
        self.recovery_required = False
        self.initial_operation = ""

    def close(self):
        if self.catalog is not None:
            self.catalog.wb.close()
        if self.lock is not None:
            self.lock.close()
        self.catalog, self.lock, self.paths = None, None, {}

    def image(self, path: Path, size: int) -> dict:
        try:
            stat = path.stat()
            key = hashlib.sha256(f"{normalized(path)}|{stat.st_size}|{stat.st_mtime_ns}|{size}|v1".encode()).hexdigest()
            target = self.cache / f"{key}.jpg"
            if not target.exists():
                with Image.open(path) as image:
                    oriented = ImageOps.exif_transpose(image)
                    oriented.thumbnail((size, size), Image.Resampling.LANCZOS)
                    # Sólo vista previa RGB; se conservan los bytes y EXIF del original y de la copia.
                    rgb = oriented.convert("RGB")
                    fd, temporary = tempfile.mkstemp(prefix="preview-", suffix=".jpg", dir=self.cache)
                    try:
                        with os.fdopen(fd, "wb") as stream:
                            rgb.save(stream, format="JPEG", quality=88)
                        os.replace(temporary, target)
                    finally:
                        Path(temporary).unlink(missing_ok=True)
            with Image.open(target) as cached:
                width, height = cached.size
            return {"url": target.as_uri(), "imageError": "", "width": width, "height": height}
        except Exception as exc:
            return {"url": "", "imageError": f"No se pudo leer la imagen: {exc}"}

    def item(self, path: Path, thumbnail=False):
        c = self.catalog
        stable_id = hashlib.sha256(normalized(path).encode()).hexdigest()
        located = c.row_for_path(path)
        row = dict(located[1]) if located else {}
        item = {"id": stable_id, "name": path.name, "path": str(path),
                "status": c.display_status(path), "registered": c.status_of(path) == "registered",
                "skipped": c.status_of(path) == "skipped", "record": row}
        if thumbnail:
            item["thumbnail"] = self.image(path, 220)["url"]
        return item

    def state(self):
        c = self.catalog
        return {"source": str(c.source), "output": str(c.output), "site": c.site,
                "total": len(c.files), "registered": len(c.completed), "pending": len(c.remaining()),
                "skipped": len(c.skipped), "gpsReview": len(c.gps_review), "xlsx": str(c.xlsx_path),
                "filters": PHOTO_FILTERS, "unitTypes": UNIT_TYPES, "subunitTypes": SUBUNIT_TYPES,
                "heif": HEIC_ENABLED, "recoveryRequired": self.recovery_required, "initialOperation": self.initial_operation}

    def select_item(self, path, remember=True):
        if remember:
            try:
                self.catalog.remember_current(path)
            except OSError:
                pass  # Un marcador de navegación no debe convertir un guardado confirmado en un fallo.
        item = self.item(path)
        point, gps_status = read_gps(path)
        item.update({"preview": self.image(path, 2000), "gps": dataclasses.asdict(point) if point else {}, "gpsStatus": gps_status})
        if item["registered"]:
            copy = self.catalog.photos / item["record"]["Nombre_archivo"]
            point, status = read_gps(copy)
            item.update({"copyGps": dataclasses.asdict(point) if point else {}, "copyGpsStatus": status})
        return item

    def next_pending(self, after):
        c = self.catalog
        start = c.files.index(after) if after in c.files else -1
        for offset in range(1, len(c.files) + 1):
            candidate = c.files[(start + offset) % len(c.files)]
            if c.status_of(candidate) == "pending": return candidate
        return None

    def load(self, request):
        project = inspect_project(request["location"], request.get("source", ""), request.get("site", ""), request.get("operation", ""))
        if any(project[key] for key in ("needsSource", "needsSite", "needsOperation")):
            return {"project": project, "needsInput": True, "message": "El catálogo está identificado. Confirma la carpeta de originales o completa únicamente la información que no contiene el catálogo anterior."}
        result = self.open(request | project | {"op": "open"})
        result["project"] = project
        return result

    def matching_files(self, request):
        condition = request.get("filter", "Todas")
        query = str(request.get("query", "")).casefold().strip()
        files = self.catalog.filtered_files(condition)
        if query:
            files = [p for p in files if query in (str(p) + " " + " ".join(self.item(p)["record"].values())).casefold()]
        return files

    def page(self, request):
        files = self.matching_files(request)
        offset = max(0, int(request.get("offset", 0)))
        selected = files[offset:offset + 60]
        return {"items": [self.item(p, True) for p in selected], "offset": offset,
                "matched": len(files), "state": self.state()}

    def open(self, request):
        source, output = Path(request["source"]).resolve(), Path(request["output"]).resolve()
        if not source.is_dir() or not output.is_dir():
            raise ValueError("Selecciona dos carpetas existentes.")
        if source == output or source.is_relative_to(output):
            raise ValueError("La salida debe ser distinta del origen y no puede contenerlo.")
        if request.get("create") and any(output.iterdir()):
            raise ValueError("Para crear un catálogo elige una carpeta de salida vacía. Para reanudar utiliza Cargar catálogo.")
        site_text, operation_text = str(request.get("site", "")).strip(), str(request.get("operation", "")).strip()
        if not site_text or not operation_text:
            existing = inspect_project(output, source, site_text, operation_text)
            site_text = site_text or existing["site"]
            operation_text = operation_text or existing["operation"]
            if existing["needsOperation"]:
                raise ValueError("Este catálogo antiguo no guarda la operación. Indícala una vez antes de confirmar su migración.")
        site = Catalog.validate_identifier(site_text, "Código del sitio")
        operation = Catalog.normalize_operation(operation_text) if operation_text else None
        same = self.catalog is not None and self.catalog.output == output
        state_file = output / "estado_catalogador.json"
        if state_file.is_file() and state_file.stat().st_size > 64 * 1024 * 1024:
            raise ValueError("El estado del catálogo supera 64 MB.")
        next_lock = self.lock if same else CatalogLock(output)
        try:
            xlsx = output / "Entrega_Museo" / "Catalogo_fotografico_pozos.xlsx"
            if xlsx.exists():
                workbook = load_workbook(xlsx, read_only=True, data_only=True)
                try:
                    headers = [cell.value for cell in workbook.active[1]]
                    sites = {str(values[0]) for values in workbook.active.iter_rows(min_row=2, values_only=True) if values[0]}
                finally:
                    workbook.close()
                if sites and sites != {site}:
                    raise ValueError("El código del sitio no coincide con el catálogo existente.")
                if headers not in (FIELDS, FIELDS_V4, LEGACY_FIELDS_V3):
                    raise ValueError("El Excel existente tiene columnas diferentes.")
                if headers in (FIELDS_V4, LEGACY_FIELDS_V3) and not request.get("confirmMigration", False):
                    return {"migrationRequired": True, "message": "El catálogo usa un esquema anterior. Se crearán respaldos fuera de Entrega_Museo. Las filas antiguas sin operación recibirán la operación indicada. Revisa el código y la operación antes de continuar."}
            cache = Path(request["cache"]).resolve()
            # La caché debe quedar fuera de la entrada científica y de la entrega.
            if cache.is_relative_to(source) or cache.is_relative_to(output):
                raise ValueError("La caché del Hub debe estar fuera del origen y de la entrega.")
            cache.mkdir(parents=True, exist_ok=True)
            candidate = Catalog(source, output, site, legacy_operation=operation)
            previous, previous_lock = self.catalog, self.lock
            self.catalog, self.lock, self.cache = candidate, next_lock, cache
            self.recovery_required = False
            self.initial_operation = operation or ""
            self.paths = {hashlib.sha256(normalized(p).encode()).hexdigest(): p for p in candidate.files}
            if previous is not None:
                previous.wb.close()
            if previous_lock is not None and previous_lock is not next_lock:
                previous_lock.close()
            resumed = next((p for p in candidate.files if normalized(p) == candidate.last_path and candidate.status_of(p) != "registered"), None)
            resumed = resumed or next(iter(candidate.remaining()), None) or next(iter(candidate.omitted()), None) or (candidate.files[-1] if candidate.files else None)
            offset = (candidate.files.index(resumed) // 60) * 60 if resumed else 0
            result = self.page({"offset": offset})
            if resumed: result["selection"] = self.select_item(resumed, remember=False)
            return result
        finally:
            if next_lock is not self.lock:
                next_lock.close()

    def path(self, request):
        path = self.paths.get(request.get("id"))
        if path is None or path not in self.catalog.files:
            raise ValueError("La fotografía no pertenece al catálogo abierto.")
        if not path.is_file():
            raise FileNotFoundError("La fotografía no está disponible. Revisa la conexión de su unidad.")
        return path

    def dispatch(self, request):
        if request.get("schemaVersion") != 1:
            raise ValueError("Versión del protocolo no compatible.")
        op = request.get("op")
        if op == "open":
            return self.open(request)
        if op == "load":
            return self.load(request)
        if op == "close":
            self.close()
            return {"closed": True}
        if self.catalog is None:
            raise ValueError("Abre primero un catálogo.")
        c = self.catalog
        if op == "page":
            return self.page(request)
        if op == "exportGps":
            result = export_gps_report(c.xlsx_path, c.photos, c.state_path, Path(request["destination"]))
            return {"report": dataclasses.asdict(result) | {"path": str(result.path)}}
        path = self.path(request)
        if op == "select":
            return {"selection": self.select_item(path)}
        if op == "navigate":
            files = self.matching_files(request)
            if not files: return self.page(request)
            index = files.index(path) if path in files else 0
            following = files[max(0, min(len(files)-1, index + (1 if int(request.get("delta", 1)) > 0 else -1)))]
            result = self.page(request | {"offset": (files.index(following) // 60) * 60})
            result["selection"] = self.select_item(following)
            return result
        if op == "nextGps":
            candidates = [p for p in c.files if c.needs_gps_review(p)]
            following = next((p for p in candidates if c.files.index(p) > c.files.index(path)), None) or next(iter(candidates), None)
            if following:
                result = self.page({"offset": (c.files.index(following) // 60) * 60})
                result["selection"] = self.select_item(following)
                return result | {"filter": "Todas"}
            return {"message": "No quedan fotografías con GPS por revisar."}
        if self.recovery_required:
            raise ValueError("Hay una transacción que requiere recuperación. Cierra y reabre el catálogo antes de guardar más cambios; no repitas el registro ya confirmado.")
        if op == "propose":
            form = request["form"]
            method = c.proposed_correction_name if c.status_of(path) == "registered" else c.proposed_name
            return {"proposedName": method(path, form["operation"], form["unitType"], form["unit"], form["level"], form.get("subunitType", ""), form.get("subunit", ""))}
        warning = ""
        try:
            if op in ("save", "correct"):
                form = request["form"]
                args = [path, form["operation"], form["unitType"], form["unit"], form["level"], form.get("view", ""), form.get("notes", ""), form.get("subunitType", ""), form.get("subunit", "")]
                if op == "save":
                    gps = request.get("manualGps")
                    c.save(*args, manual_gps=tuple(gps) if gps else None)
                else:
                    c.correct(*args)
            elif op == "skip":
                c.set_skipped(path, bool(request.get("skipped", True)))
            elif op == "withdraw":
                if request.get("confirmed") is not True:
                    raise ValueError("Confirma el retiro recuperable antes de continuar.")
                c.withdraw_for_review(path)
            elif op == "discard":
                if request.get("confirmed") is not True:
                    raise ValueError("Confirma el resguardo del original antes de continuar.")
                withdrawn = False
                if c.status_of(path) == "registered":
                    c.withdraw_for_review(path)
                    withdrawn = True
                try:
                    c.discard_unregistered(path)
                except COMMITTED:
                    raise
                except Exception as exc:
                    if withdrawn:
                        raise WithdrawalCommittedWarning(f"El registro quedó retirado de la entrega, pero el original se conserva en su carpeta: {exc}. Reabre el catálogo antes de continuar con su resguardo.") from exc
                    raise
            elif op == "manualGps":
                c.update_manual_gps(path, *request["manualGps"])
            else:
                raise ValueError("Acción del catálogo no reconocida.")
        except COMMITTED as exc:
            warning = str(exc)
            self.recovery_required = True
        result = self.page(request)
        result["warning"] = warning
        result["committed"] = True
        following = self.next_pending(path) if request.get("advance") and not warning and op in ("save", "skip", "discard") else None
        if following:
            result = self.page({"offset": (c.files.index(following) // 60) * 60}) | {"committed": True, "filter": "Todas"}
            result["selection"] = self.select_item(following)
            return result
        if path in c.files and path.exists():
            result["selection"] = self.select_item(path, remember=False)
        return result


def main():
    # Protocolo UTF-8 explícito, independiente de la página ANSI de Windows.
    sys.stdin.reconfigure(encoding="utf-8", errors="strict")
    sys.stdout.reconfigure(encoding="utf-8", errors="strict", newline="\n")
    session = Session()
    try:
        while True:
            line = sys.stdin.buffer.readline(2 * 1024 * 1024 + 1)
            if not line:
                break
            request_id = None
            try:
                if len(line) > 2 * 1024 * 1024 or not line.endswith(b"\n"):
                    raise ValueError("Solicitud demasiado grande o incompleta.")
                request = json.loads(line)
                request_id = request.get("requestId")
                result = session.dispatch(request)
                reply = {"schemaVersion": 1, "requestId": request_id, "ok": True, "result": result}
            except Exception as exc:
                reply = {"schemaVersion": 1, "requestId": request_id, "ok": False, "error": str(exc), "errorType": type(exc).__name__}
            print(json.dumps(reply, ensure_ascii=False, allow_nan=False), flush=True)
    finally:
        session.close()


if __name__ == "__main__":
    main()
