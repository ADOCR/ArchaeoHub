"""Inyecta GPS EXIF manual en JPEG sin recomprimir la fotografía.

Se conserva la estructura TIFF existente y se agrega un IFD GPS nuevo. Los
datos de imagen y todos los bloques JPEG ajenos a EXIF quedan byte a byte
iguales. No se usa para HEIC u otros formatos.
"""

from __future__ import annotations

import io
import struct
from decimal import Decimal, InvalidOperation, ROUND_HALF_UP

from PIL import Image


def coordinate(text: str, kind: str) -> Decimal:
    value = text.strip()
    if not value or "," in value:
        raise ValueError(f"{kind}: usa grados decimales con punto, por ejemplo 9.966968.")
    try:
        number = Decimal(value)
    except InvalidOperation as exc:
        raise ValueError(f"{kind}: escribe un número válido.") from exc
    limit = 90 if kind == "Latitud" else 180
    if not number.is_finite() or abs(number) > limit:
        raise ValueError(f"{kind}: debe estar entre -{limit} y {limit} grados.")
    return number


def optional_number(text: str, name: str, minimum: Decimal, maximum: Decimal) -> Decimal | None:
    value = text.strip()
    if not value:
        return None
    if "," in value:
        raise ValueError(f"{name}: usa punto decimal, no coma.")
    try:
        number = Decimal(value)
    except InvalidOperation as exc:
        raise ValueError(f"{name}: escribe un número válido.") from exc
    if not number.is_finite() or not minimum <= number <= maximum:
        raise ValueError(f"{name}: valor fuera de rango.")
    return number


def _rational(value: Decimal, scale: int = 1_000_000) -> tuple[int, int]:
    numerator = int((value * scale).to_integral_value(rounding=ROUND_HALF_UP))
    if not 0 <= numerator <= 0xFFFFFFFF:
        raise ValueError("La coordenada no cabe en un racional EXIF.")
    return numerator, scale


def _dms(value: Decimal) -> tuple[tuple[int, int], ...]:
    absolute = abs(value)
    degrees = int(absolute)
    minutes_decimal = (absolute - degrees) * 60
    minutes = int(minutes_decimal)
    seconds = (minutes_decimal - minutes) * 60
    return (degrees, 1), (minutes, 1), _rational(seconds)


def _jpeg_segments(data: bytes):
    if not data.startswith(b"\xff\xd8"):
        raise ValueError("Solo se puede modificar el GPS de una copia JPEG.")
    position = 2
    while position < len(data) - 4:
        if data[position] != 0xFF:
            raise ValueError("Cabecera JPEG inesperada.")
        start = position
        while data[position] == 0xFF:
            position += 1
        marker = data[position]
        position += 1
        if marker == 0xDA:
            return
        if marker in (0x01, 0xD8, 0xD9) or 0xD0 <= marker <= 0xD7:
            continue
        if position + 2 > len(data):
            raise ValueError("Segmento JPEG incompleto.")
        length = int.from_bytes(data[position:position + 2], "big")
        if length < 2 or position + length > len(data):
            raise ValueError("Segmento JPEG inválido.")
        end = position + length
        yield marker, start, end, data[position + 2:end]
        position = end


def _gps_ifd(offset: int, byte_order: str, lat: Decimal, lon: Decimal,
             altitude: Decimal | None, accuracy: Decimal | None) -> bytes:
    endian = "<" if byte_order == "II" else ">"
    pack = lambda fmt, *values: struct.pack(endian + fmt, *values)
    fields: list[tuple[int, int, int, bytes]] = [
        (0, 1, 4, bytes((2, 3, 0, 0))),
        (1, 2, 2, (b"S" if lat < 0 else b"N") + b"\x00"),
        (2, 5, 3, b"".join(pack("II", *pair) for pair in _dms(lat))),
        (3, 2, 2, (b"W" if lon < 0 else b"E") + b"\x00"),
        (4, 5, 3, b"".join(pack("II", *pair) for pair in _dms(lon))),
        (18, 2, 7, b"WGS-84\x00"),
        (27, 7, 14, b"ASCII\x00\x00\x00MANUAL"),
    ]
    if altitude is not None:
        fields.extend([
            (5, 1, 1, bytes((1 if altitude < 0 else 0,))),
            (6, 5, 1, pack("II", *_rational(abs(altitude), 1000))),
        ])
    if accuracy is not None:
        fields.append((31, 5, 1, pack("II", *_rational(accuracy, 1000))))
    fields.sort(key=lambda item: item[0])
    header_size = 2 + len(fields) * 12 + 4
    entries = bytearray(pack("H", len(fields)))
    extra = bytearray()
    for tag, field_type, count, value in fields:
        if len(value) <= 4:
            pointer = value.ljust(4, b"\x00")
        else:
            if len(extra) % 2:
                extra += b"\x00"
            pointer = pack("I", offset + header_size + len(extra))
            extra += value
        entries += pack("HHI", tag, field_type, count) + pointer
    entries += pack("I", 0)
    return bytes(entries + extra)


def _append_gps_to_tiff(raw: bytes, lat: Decimal, lon: Decimal,
                        altitude: Decimal | None, accuracy: Decimal | None) -> bytes:
    tiff = bytearray(raw)
    if len(tiff) < 14 or tiff[:2] not in (b"II", b"MM"):
        raise ValueError("El bloque EXIF no contiene TIFF válido.")
    order = tiff[:2].decode("ascii")
    endian = "<" if order == "II" else ">"
    unpack = lambda fmt, position: struct.unpack_from(endian + fmt, tiff, position)[0]
    pack = lambda fmt, *values: struct.pack(endian + fmt, *values)
    if unpack("H", 2) != 42:
        raise ValueError("La cabecera TIFF del EXIF no es válida.")
    ifd0 = unpack("I", 4)
    if ifd0 + 2 > len(tiff):
        raise ValueError("El directorio EXIF principal está incompleto.")
    count = unpack("H", ifd0)
    end = ifd0 + 2 + count * 12 + 4
    if end > len(tiff):
        raise ValueError("El directorio EXIF principal está truncado.")
    entries = [bytes(tiff[ifd0 + 2 + i * 12:ifd0 + 14 + i * 12]) for i in range(count)]
    gps_entry = next((ifd0 + 2 + i * 12 for i in range(count)
                      if unpack("H", ifd0 + 2 + i * 12) == 0x8825), None)
    if len(tiff) % 2:
        tiff += b"\x00"
    if gps_entry is None:
        if count >= 0xFFFF:
            raise ValueError("El directorio EXIF no admite otra etiqueta.")
        new_ifd0 = len(tiff)
        new_gps = new_ifd0 + 2 + (count + 1) * 12 + 4
        gps_tag = pack("HHII", 0x8825, 4, 1, new_gps)
        entries.append(gps_tag)
        entries.sort(key=lambda entry: struct.unpack(endian + "H", entry[:2])[0])
        tiff += pack("H", count + 1) + b"".join(entries) + bytes(tiff[end - 4:end])
        tiff[4:8] = pack("I", new_ifd0)
    else:
        new_gps = len(tiff)
        tiff[gps_entry:gps_entry + 12] = pack("HHII", 0x8825, 4, 1, new_gps)
    tiff += _gps_ifd(new_gps, order, lat, lon, altitude, accuracy)
    return bytes(tiff)


def add_manual_gps(data: bytes, latitude: str, longitude: str,
                   altitude: str = "", accuracy: str = "") -> bytes:
    """Devuelve un JPEG con GPS manual; no cambia los bytes de imagen."""
    lat = coordinate(latitude, "Latitud")
    lon = coordinate(longitude, "Longitud")
    alt = optional_number(altitude, "Altitud", Decimal("-10000"), Decimal("100000"))
    error = optional_number(accuracy, "Precisión", Decimal("0"), Decimal("100000"))
    found = [(start, end, payload) for marker, start, end, payload in _jpeg_segments(data)
             if marker == 0xE1 and payload.startswith(b"Exif\x00\x00")]
    if len(found) > 1:
        raise ValueError("El JPEG tiene más de un bloque EXIF; requiere revisión manual.")
    if found:
        start, end, payload = found[0]
        tiff = _append_gps_to_tiff(payload[6:], lat, lon, alt, error)
    else:
        start = end = 2
        tiff = _append_gps_to_tiff(b"II*\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00", lat, lon, alt, error)
    new_payload = b"Exif\x00\x00" + tiff
    if len(new_payload) + 2 > 0xFFFF:
        raise ValueError("No cabe más EXIF en este JPEG; no se modificó la copia.")
    segment = b"\xff\xe1" + (len(new_payload) + 2).to_bytes(2, "big") + new_payload
    result = data[:start] + segment + data[end:]
    with Image.open(io.BytesIO(result)) as image:
        image.verify()
    return result
