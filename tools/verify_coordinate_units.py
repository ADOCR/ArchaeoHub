"""Comprueba unidades según el CRS, protocolo de formatos angulares y vistas compactas de coordenadas."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload", type=Path, required=True)
    parser.add_argument("--workdir", type=Path, required=True)
    parser.add_argument("--case", help="Run one named UI case while adjusting a layout")
    args = parser.parse_args()
    payload, root = args.payload.resolve(), args.workdir.resolve()
    root.mkdir(parents=True, exist_ok=True)
    if any(root.iterdir()):
        raise ValueError("Use an empty isolated verification directory")
    env = {k: os.environ[k] for k in ("SystemRoot", "WINDIR", "TEMP", "TMP", "USERPROFILE", "LOCALAPPDATA", "APPDATA") if k in os.environ}
    env["PATH"] = str(Path(env["SystemRoot"]) / "System32")
    exe = payload / "ArchaeoHub.exe"
    report = {"ui": {}, "conversions": {}}
    cases = [
        ("geographic-DD", 4326, 4326, 0, "light", 1280, False, False),
        ("geographic-DM", 4326, 4326, 1, "dark", 960, False, False),
        ("geographic-DMS", 4326, 4326, 2, "light", 960, False, False),
        ("metres-to-degrees", 8908, 4326, 2, "light", 1280, False, False),
        ("degrees-to-metres", 4326, 8908, 2, "dark", 960, False, False),
        ("metres-both", 8908, 8908, 2, "dark", 960, False, False),
        ("metres-batch", 8908, 8908, 2, "light", 960, True, False),
        ("metres-accessible", 8908, 8908, 2, "dark", 960, False, True),
        ("other-linear-unit", 2263, 2263, 2, "light", 1280, False, False),
    ]
    for name, source, target, angle, theme, width, batch, accessible in cases:
        if args.case and args.case != name:
            continue
        profile = root / (name + "-profile")
        profile.mkdir()
        (profile / "coordinate-preferences.ini").write_text(f"[General]\nsource=EPSG:{source}\ntarget=EPSG:{target}\n", encoding="utf-8")
        if accessible:
            (profile / "preferences.ini").write_text("[appearance]\ntextScale=1.4\nreduceTransparency=true\nreduceMotion=true\nhighContrast=true\n", encoding="utf-8")
        options = root / (name + "-options.json")
        options.write_text(json.dumps({"coordinateInputFormat": angle, "coordinateOutputFormat": angle, "batch": batch}), encoding="utf-8")
        target_report = root / (name + ".json")
        process = subprocess.run([str(exe), "--data-dir", str(profile), "--smoke-test", "--route", "tool/workbench.coordinates", "--theme", theme, "--width", str(width), "--height", "640" if width == 960 else "820", "--coordinate-ui-options", str(options), "--report", str(target_report), "--screenshot", str(root / (name + ".png"))], env=env, capture_output=True, timeout=40)
        assert process.returncode == 0, (name, process.returncode, process.stderr.decode(errors="replace"))
        data = json.loads(target_report.read_text(encoding="utf-8"))
        assert data["ok"] and not data["qmlWarnings"], (name, data)
        ui = data["coordinateUi"]
        assert ui["convertButtonWithinWindow"], (name, ui)
        for side, code, format_key, unit_key, option_key in (("source", source, "coordinateInputFormat", "coordinateInputUnit", "inputFormat"), ("target", target, "coordinateOutputFormat", "coordinateOutputUnit", "outputFormat")):
            assert ui[side]["code"] == f"EPSG:{code}", (name, ui)
            angular = ui[side]["geographic"]
            assert ui[format_key]["visible"] == angular
            assert ui[unit_key]["visible"] != angular
            assert ui["options"][option_key] == (["DD", "DM", "DMS"][angle] if angular else "DD"), (name, ui)
            if angular:
                assert "Grados" in ui[format_key]["currentText"]
            elif ui[side]["xUnit"] == "metre":
                assert ui[unit_key]["text"] == "Metros (m)"
            else:
                assert "Metros" not in ui[unit_key]["text"]
                assert ui[side]["xUnit"] in ui[unit_key]["text"]
        report["ui"][name] = ui
        print(name, "OK", flush=True)

    # Usa los códigos de protocolo emitidos por QML; las etiquetas no deben llegar al analizador.
    examples = [
        ("metres-identity", 8908, 8908, "500000", "1105744.247751049", "DD", ".", (500000, 1105744.247751049)),
        ("metres-comma-to-geographic", 8908, 8907, "500000,0", "1105744,247751049", "DD", ",", (-84, 10)),
        ("geographic-DM-identity", 4326, 4326, "84 30 W", "10 15 N", "DM", ".", (-84.5, 10.25)),
        ("geographic-DMS-identity", 4326, 4326, "84 30 0 W", "10 15 0 N", "DMS", ".", (-84.5, 10.25)),
    ]
    for name, source, target, x, y, fmt, decimal, expected in examples:
        request = root / (name + "-request.json")
        request.write_text(json.dumps({"op": "convert", "source": f"EPSG:{source}", "target": f"EPSG:{target}", "inputFormat": fmt, "outputFormat": "DD", "decimal": decimal, "precision": 7, "rows": [{"id": "QA", "x": x, "y": y}]}), encoding="utf-8")
        target_report = root / (name + ".json")
        process = subprocess.run([str(exe), "--data-dir", str(root / (name + "-profile")), "--coordinate-request", str(request), "--report", str(target_report)], env=env, capture_output=True, timeout=40)
        assert process.returncode == 0, (name, process.returncode)
        data = json.loads(target_report.read_text(encoding="utf-8"))
        assert data["ok"] and data["succeeded"] == 1 and data["failed"] == 0, (name, data)
        row = data["rows"][0]
        assert abs(row["resultX"] - expected[0]) < 1e-7 and abs(row["resultY"] - expected[1]) < 1e-7, (name, row)
        report["conversions"][name] = data
        print(name, "OK", flush=True)
    report.update(ok=True, hostSha256=hashlib.sha256(exe.read_bytes()).hexdigest())
    (root / "coordinate-units-validation.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
