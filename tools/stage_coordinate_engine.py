"""Prepara el runtime PROJ revisado como dependencia privada del motor, cargada al utilizarlo."""
from pathlib import Path
import argparse,json,shutil,subprocess,sys
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--destination',type=Path,required=True);p.add_argument('--bin',type=Path,required=True);p.add_argument('--data',type=Path,required=True);a=p.parse_args()
    a.destination.mkdir(parents=True,exist_ok=True);data=a.destination/'data';data.mkdir(exist_ok=True)
    shutil.copy2(a.bin/'proj_9.dll',a.destination/'proj_9.dll')
    for name in ['proj.db','proj.ini']:
        if (a.data/name).is_file():shutil.copy2(a.data/name,data/name)
    (a.destination/'engine.json').write_text(json.dumps({'library':'proj_9.dll','network':False,'version':'9.8.1'},indent=2)+'\n',encoding='utf-8')
    subprocess.run([sys.executable,str(Path(__file__).with_name('copy_pe_dependencies.py')),str(a.destination),str(a.bin)],check=True)
if __name__=='__main__':main()
