"""Make a source ZIP after read-only evidence checks. No upload or Git mutation."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys
import zipfile
from run_tests import ROOT
from make_manifest import paths

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT.parent/(ROOT.name+'-source.zip'))
    args=p.parse_args()
    subprocess.run([sys.executable,ROOT/'tools/verify.py'],cwd=ROOT,check=True)
    names=paths()
    manifest=json.loads((ROOT/'provenance/manifest.json').read_text())
    assert set(names)=={x['path'] for x in manifest['files']}|{'provenance/manifest.json'}, 'Refresh reviewed manifest before packaging'
    forbidden=('.git/','.tools/','.venv/','runs/','build')
    assert not any(x.startswith(forbidden) for x in names)
    dest=args.output.resolve();dest.parent.mkdir(parents=True,exist_ok=True)
    # A new name is required; never replace a user archive.
    with zipfile.ZipFile(dest,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name in names:z.write(ROOT/name,ROOT.name+'/'+name)
    with zipfile.ZipFile(dest) as z:
        assert z.testzip() is None
        for name in names:assert z.read(ROOT.name+'/'+name)==(ROOT/name).read_bytes()
    print(json.dumps(dict(archive=str(dest),files=len(names),bytes=dest.stat().st_size,
                         sha256=hashlib.sha256(dest.read_bytes()).hexdigest(),uploaded=False),indent=2))

if __name__=='__main__':main()
