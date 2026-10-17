"""Reúne los textos locales exactos de licencias para el instalador; este paso no registra su aceptación."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
text=(root/"packaging/windows/THIRD_PARTY_NOTICE.txt").read_text(encoding="utf-8")
for path in sorted((root/"resources/licenses").rglob("*")):
    if path.is_file():
        text+="\n\n"+"="*70+"\n"+path.relative_to(root/"resources").as_posix()+"\n"+"="*70+"\n"
        text+=path.read_text(encoding="utf-8-sig",errors="replace")
target=root/".local/installer-build/third-party-notice.txt";target.parent.mkdir(parents=True,exist_ok=True)
target.write_text(text,encoding="utf-8-sig")
print(target)
