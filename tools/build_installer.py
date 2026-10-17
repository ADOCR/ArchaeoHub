"""Construye el instalador del Hub desde una distribución verificada mediante NSIS 3."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import re

def escaped(value):
    text=str(value)
    if '\n' in text or '\r' in text:raise ValueError('Newlines are not allowed in NSIS definitions')
    return text.replace('$','$$').replace('"','$\\"')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--payload',type=Path,required=True);parser.add_argument('--nsis',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--icon',type=Path,required=True)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1];payload=args.payload.resolve();output=args.output.resolve()
    if not (payload/'ArchaeoHub.exe').is_file():raise FileNotFoundError('Build/deploy the Hub before packaging')
    files=sorted(p for p in payload.rglob('*') if p.is_file())
    for p in files:
        if p.is_symlink() or not p.resolve().is_relative_to(payload):raise ValueError('Reparse/symlink path in payload')
        if p.suffix.lower() in ('.onnx','.gguf','.ggml','.pyc','.sqlite','.pdb'):raise ValueError('Data/model/development file in payload: '+str(p))
    output.parent.mkdir(parents=True,exist_ok=True);work=root/'.local/installer-build';work.mkdir(parents=True,exist_ok=True)
    install=[];remove=[];hashes={};folders=set()
    for p in files:
        relative=p.relative_to(payload);name=escaped(str(relative).replace('/','\\'))
        install.extend([f'SetOutPath "$INSTDIR\\{escaped(relative.parent)}"',f'File "{escaped(p)}"'])
        remove.extend(['ClearErrors',f'Delete "$INSTDIR\\{name}"','IfErrors 0 +2','StrCpy $DeleteFailed 1'])
        folders.update(relative.parents);hashes[relative.as_posix()]=hashlib.sha256(p.read_bytes()).hexdigest()
    for folder in sorted(folders,key=lambda p:(len(p.parts),str(p)),reverse=True):
        if str(folder)!='.':remove.append(f'RMDir "$INSTDIR\\{escaped(folder)}"')
    installing=work/'install-files.nsh';removing=work/'uninstall-files.nsh'
    installing.write_text('\n'.join(install)+'\n',encoding='utf-8-sig');removing.write_text('\n'.join(remove)+'\n',encoding='utf-8-sig')
    (work/'payload-hashes.json').write_text(json.dumps(hashes,indent=2)+'\n',encoding='utf-8')
    brand=json.loads((root/'resources/branding.json').read_text(encoding='utf-8'))
    import sys
    subprocess.run([sys.executable,str(root/'tools/build_third_party_notice.py')],check=True)
    version=re.search(r'project\(ArchaeologyWorkbench VERSION (\d+\.\d+\.\d+)',(root/'CMakeLists.txt').read_text(encoding='utf-8')).group(1)
    definitions={'PRODUCT_NAME':brand['displayName'],'PRODUCT_VERSION':version+'.0','OUTPUT_FILE':output,
        'ICON_FILE':args.icon.resolve(),'INSTALL_MANIFEST':installing,'UNINSTALL_MANIFEST':removing,
        'THIRD_PARTY_NOTICE':work/'third-party-notice.txt'}
    subprocess.run([str(args.nsis),'/V2',*[f'/D{k}={escaped(v)}' for k,v in definitions.items()],str(root/'packaging/windows/workbench.nsi')],check=True)
    digest=hashlib.sha256(output.read_bytes()).hexdigest();output.with_suffix('.exe.sha256').write_text(f'{digest}  {output.name}\n',encoding='utf-8')
    print(json.dumps({'installer':str(output),'files':len(files),'bytes':output.stat().st_size,'sha256':digest}))

if __name__=='__main__':main()
