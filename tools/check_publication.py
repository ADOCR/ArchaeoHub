"""Rechaza datos privados, credenciales habituales y dependencias de recursos ausentes.

No realiza solicitudes de red. Informa reglas y rutas relativas sin revelar valores.
La revisión está acotada; no garantiza detectar todas las credenciales posibles.
"""
from pathlib import Path
import argparse, hashlib, json, re, subprocess, sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
IGNORED = {'.git','.local','.venv','venv','__pycache__','dist','out','third_party','models','modelos','data','datasets','outputs','.cache'}
TOP_LEVEL = {'.github','app','core','services','shared','ui','modules','tests','tools','packaging','resources','docs','config'}
ROOT_FILES = {'AGENTS.md','LICENSE','README.md','BUILDING.md','ARCHITECTURE.md','MODULES.md','DESIGN.md','THIRD_PARTY.md',
              'CONTRIBUTING.md','SECURITY.md','CMakeLists.txt','resources.qrc','.gitignore','.gitattributes',
              'requirements.txt','requirements-build.txt','requirements-geodesy.txt'}
ALLOWED_MODELS = {'modules/transcription/native/resources/nemo128.onnx','modules/transcription/native/resources/silero_vad.onnx'}
BANNED_SUFFIXES = {'.exe','.dll','.so','.dylib','.pdb','.zip','.7z','.dmg','.appimage','.gguf','.ggml','.bin',
                   '.sqlite','.sqlite3','.db','.ahinv','.xlsx','.xls','.csv','.pdf','.docx','.wav','.mp3','.ogg','.mp4',
                   '.pem','.key','.pfx','.p12','.pyc','.log'}
BINARY_ASSETS = {'.onnx','.png','.ico'}
ALLOWED_ARTWORK = {'docs/preview.png','resources/app.ico'}
PATTERNS = {
    'private-user-path': re.compile(r'[A-Za-z]:[/\\]+Users[/\\]+[^/\\\s]+',re.I),
    'private-workspace-path': re.compile(r'[A-Za-z]:[/\\]+(?:codex|Trabajo|Descargas|OneDrive|Conda|Qgis)(?:[/\\]+|\b)',re.I),
    'private-posix-profile': re.compile(r'/(?:home|Users)/[A-Za-z0-9_.-]+/'),
    'private-key': re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----'),
    'github-token': re.compile(r'(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,})'),
    'aws-access-key': re.compile(r'AKIA[0-9A-Z]{16}'),
    'api-key': re.compile(r'\bsk-(?:proj-)?[A-Za-z0-9_-]{30,}'),
}

def findings(root, relative):
    """Devuelve sólo nombres de reglas, sin revelar credenciales ni contenido documental."""
    path=root/relative;parts=Path(relative).parts;issues=[]
    if not parts or (len(parts)>1 and parts[0] not in TOP_LEVEL) or (len(parts)==1 and relative not in ROOT_FILES):
        issues.append('unexpected-path')
    if any(p in IGNORED or p.startswith('build') for p in parts[:-1]) or relative.startswith('docs/audit/') or relative=='docs/RESUME.md':
        issues.append('private-or-generated-path')
    if path.is_symlink():return issues+['symlink']
    if not path.is_file():return issues+['missing-file']
    if path.stat().st_size>10*1024*1024:issues.append('oversized-source-file')
    if path.suffix.lower() in BANNED_SUFFIXES or path.name.startswith('.env'):
        issues.append('data-binary-or-credential-file')
    if path.suffix.lower()=='.onnx' and relative not in ALLOWED_MODELS:issues.append('unapproved-model')
    if path.suffix.lower() in ('.png','.ico') and relative not in ALLOWED_ARTWORK:issues.append('unreviewed-image')
    if path.suffix.lower() not in BINARY_ASSETS:
        try:text=path.read_text(encoding='utf-8-sig')
        except (UnicodeError,OSError):return issues+['unexpected-binary']
        issues.extend(name for name,pattern in PATTERNS.items() if pattern.search(text))
    return issues

def files(root, tracked):
    if tracked:
        command=subprocess.run(['git','ls-files','-z','--cached'],cwd=root,capture_output=True,check=True)
        return sorted(set(command.stdout.decode('utf-8').split('\0'))-{''})
    return sorted(p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()
                  and not any(part in IGNORED or part.startswith('build') for part in p.relative_to(root).parts[:-1]))

def verify_resources(root):
    issues=[]
    for manifest in [root/'resources.qrc',root/'modules/transcription/native/resources.qrc']:
        for element in ET.parse(manifest).iter('file'):
            target=manifest.parent/(element.text or '')
            if not target.is_file():issues.append({'path':manifest.relative_to(root).as_posix(),'rules':['qrc-resource-missing']})
    models=json.loads((root/'resources/auxiliary-models.json').read_text(encoding='utf-8'))
    assert {item['path'] for item in models['resources']}==ALLOWED_MODELS
    for item in models['resources']:
        path=root/item['path'];notice=root/item['notice']
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest()!=item['sha256'] or not notice.is_file() or item['license']!='MIT':
            issues.append({'path':item['path'],'rules':['auxiliary-model-integrity-or-attribution']})
    return issues

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--tracked',action='store_true')
    args=parser.parse_args();selected=files(ROOT,args.tracked)
    if not selected:raise ValueError('No source files selected')
    issues=[{'path':name,'rules':result} for name in selected if (result:=findings(ROOT,name))]
    issues.extend(verify_resources(ROOT))
    if args.tracked:
        unstaged=subprocess.run(['git','diff','--name-only'],cwd=ROOT,capture_output=True,check=True)
        if unstaged.stdout:issues.append({'path':'.','rules':['unstaged-changes-not-audited']})
    print(json.dumps({'ok':not issues,'filesChecked':len(selected),'findings':issues,
                      'limits':'Pattern-based source audit; no claim of exhaustive secret detection.'},indent=2))
    return 1 if issues else 0
if __name__=='__main__':sys.exit(main())
