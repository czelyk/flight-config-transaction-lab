"""Read-only verification of shipped sources, raw results and dependency pins."""
from pathlib import Path
import hashlib
import json
from analyze import analyze
from run_tests import ROOT

def main():
    manifest=json.loads((ROOT/'provenance/manifest.json').read_text())
    for entry in manifest['files']:
        path=(ROOT/entry['path']).resolve()
        assert path.is_relative_to(ROOT.resolve()), entry['path']
        assert hashlib.sha256(path.read_bytes()).hexdigest()==entry['sha256'], entry['path']
    upstream=json.loads((ROOT/'provenance/upstream.json').read_text())
    for entry in upstream['files']:
        assert hashlib.sha256((ROOT/entry['path']).read_bytes()).hexdigest()==entry['sha256']
    summary=analyze(ROOT/'results/recorded',write=False)
    assert summary==json.loads((ROOT/'results/recorded/summary.json').read_text())
    for report in ('results/recorded/run.json','results/arm/arm.json'):
        recorded=json.loads((ROOT/report).read_text())
        assert recorded['pass_all'] and not recorded['physical_tests']
        for path,digest in recorded['source_sha256'].items():
            assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest, path
    witness=json.loads((ROOT/'results/witness/witness.json').read_text())
    assert witness['pass_replay'] and witness['expected']==witness['observed']
    assert json.loads((ROOT/'results/cmake/output-comparison.json').read_text())['matches_optimized']
    print(json.dumps(dict(pass_all=True,hashed_files=len(manifest['files']),trial_rows=summary['total_records'],
                          arm_nominal_cycles=36,physical_tests=False),indent=2))

if __name__=='__main__':main()
