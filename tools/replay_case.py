"""Replay one raw CSV witness through the compiled C implementation."""
from pathlib import Path
import argparse
import csv
import datetime
import hashlib
import json
import os
import subprocess
import uuid
from run_tests import ROOT

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--run',type=Path,help='Existing successful native run; default newest')
    p.add_argument('--policy',choices=['sequential','bundle','manifest'],default='sequential')
    args=p.parse_args()
    run=args.run or sorted((ROOT/'runs').glob('*/run.json'))[-1].parent
    record=json.loads((run/'run.json').read_text())
    for path, expected in record['source_sha256'].items():
        if hashlib.sha256((ROOT/path).read_bytes()).hexdigest() != expected:
            raise SystemExit('C sources changed: run tools/run_tests.py first.')
    rows=list(csv.DictReader((run/'campaign.csv').open(newline='',encoding='utf-8')))
    expected=next(x for x in rows if x['policy']==args.policy and x['cut_operation']!='0'
                  and (x['outcome']=='mixed_storage' if args.policy=='sequential' else True))
    prog='16' if expected['block_size']=='512' else '64'
    coordinates=[str(['sequential','bundle','manifest'].index(args.policy)),expected['block_size'],
                 expected['block_count'],prog,expected['payload_per_group'],expected['aged_updates'],
                 expected['cut_operation'],expected['prefix_bytes']]
    target=ROOT/'runs'/('witness_'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'_'+uuid.uuid4().hex[:6])
    target.mkdir()
    binary=run/('campaign.exe' if os.name=='nt' else 'campaign')
    r=subprocess.run([str(binary.resolve()),'--replay',*coordinates],cwd=target,capture_output=True,text=True,check=True)
    (target/'execution.log').write_text(r.stdout+r.stderr,encoding='utf-8')
    actual=list(csv.DictReader((target/'campaign.csv').open(newline='',encoding='utf-8')))
    assert len(actual)==1 and actual[0]==expected, (expected,actual)
    witness=dict(pass_replay=True, reference_run=str(run), coordinates=coordinates, expected=expected,
                 observed=actual[0], physical_test=False)
    (target/'witness.json').write_text(json.dumps(witness,indent=2),encoding='utf-8')
    print(json.dumps(dict(output=str(target),**witness),indent=2))

if __name__=='__main__':
    main()
