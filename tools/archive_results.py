"""Copy one completed set of results to a NEW archival destination; never overwrite."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
from run_tests import ROOT

def main():
    p=argparse.ArgumentParser()
    p.add_argument('destination',type=Path,help='New directory, e.g. results or results-next; must not exist')
    args=p.parse_args()
    native=sorted((ROOT/'runs').glob('*/run.json'))[-1].parent
    arm=sorted((ROOT/'runs').glob('*/arm.json'))[-1].parent
    witness=sorted((ROOT/'runs').glob('*/witness.json'))[-1].parent
    assert json.loads((native/'run.json').read_text())['pass_all']
    assert json.loads((arm/'arm.json').read_text())['pass_all']
    assert json.loads((witness/'witness.json').read_text())['pass_replay']
    assert (native/'summary.json').exists(), 'Run tools/analyze.py first'
    assert (ROOT/'runs/cmake-fresh.json').exists()
    files=[]
    for source in native.iterdir():
        if source.suffix in ('.csv','.json','.log','.txt','.md'):
            files.append((source,Path('recorded')/source.name))
    for source in arm.iterdir():
        if source.suffix in ('.json','.log','.txt','.map'):
            files.append((source,Path('arm')/source.name))
    for source in witness.iterdir():
        if source.suffix in ('.json','.csv','.log'):
            files.append((source,Path('witness')/source.name))
    files += [(ROOT/'runs/cmake-fresh.json',Path('cmake/run.json')),
              (ROOT/'runs/cmake-fresh.log',Path('cmake/execution.log')),
              (ROOT/'build-cmake-fresh/Testing/Temporary/LastTest.log',Path('cmake/LastTest.log'))]
    comparison={}
    for name in ('campaign.csv','costs.csv','full_prefix.csv','full_prefix_costs.csv'):
        a=(native/name).read_bytes().replace(b'\r\n',b'\n')
        b=(ROOT/'build-cmake-fresh'/name).read_bytes().replace(b'\r\n',b'\n')
        assert a==b, 'Debug/optimized output differs: '+name
        comparison[name]=hashlib.sha256(a).hexdigest()
    dest=args.destination.resolve()
    dest.mkdir(parents=True,exist_ok=False)
    for source,relative in files:
        target=dest/relative;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,target)
    (dest/'cmake/output-comparison.json').write_text(json.dumps(dict(matches_optimized=True,lf_normalized_sha256=comparison),indent=2),encoding='utf-8')
    stacks='\n'.join(p.name+'\n'+p.read_text() for p in sorted(arm.glob('*.su')))
    (dest/'arm/stack-usage.txt').write_text(stacks,encoding='utf-8')
    print('Archived',len(files),'files without copying any executable or overwriting evidence:',dest)

if __name__=='__main__':main()
