"""Maintainer utility: freeze Git-eligible files after reviewing fresh test evidence."""
from pathlib import Path
import hashlib
import json
import subprocess
from run_tests import ROOT

def paths():
    text=subprocess.check_output(['git','-C',str(ROOT),'ls-files','--cached','--others','--exclude-standard','-z']).decode('utf-8')
    return sorted(set(x for x in text.split('\0') if x))

def main():
    files=[]
    for name in paths():
        if name=='provenance/manifest.json':continue
        path=ROOT/name
        assert path.is_file() and path.stat().st_size<20_000_000,name
        files.append(dict(path=name,bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    (ROOT/'provenance/manifest.json').write_text(json.dumps(dict(format=1,kind='source and recorded evidence; self excluded',files=files),indent=2),encoding='utf-8')
    print('Manifest:',len(files),'files')

if __name__=='__main__':main()
