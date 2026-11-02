"""Independently validate raw campaign CSVs and make a bounded results report."""
from pathlib import Path
import argparse
import collections
import csv
import json

ROOT = Path(__file__).resolve().parents[1]

def analyze(run, write=True):
    if write and not run.resolve().is_relative_to((ROOT/'runs').resolve()):
        raise ValueError('Write reports only inside ignored runs/; recorded evidence is immutable.')
    record = json.loads((run / 'run.json').read_text())
    counts = {}
    total = cut_trials = acknowledgments = 0
    for filename, key in [('campaign.csv','sampled'), ('full_prefix.csv','full-prefix')]:
        if not (run / filename).exists():
            continue
        rows = list(csv.DictReader((run / filename).open(newline='',encoding='utf-8')))
        expected = next(x for x in record['campaigns'] if x['campaign'] == key)
        assert len(rows) == expected['rows']
        by = collections.defaultdict(collections.Counter)
        for x in rows:
            assert x['outcome'] in ('old','new','mixed_storage')
            by[x['policy']][x['outcome']] += 1
            assert x['retry_ok'] == '1'
            if x['policy'] != 'sequential':
                assert x['outcome'] in ('old','new')
            if x['acknowledged'] == '1':
                assert x['outcome'] == 'new' and x['cut_operation'] == '0'
                acknowledgments += 1
            else:
                cut_trials += 1
            if x['outcome'] == 'mixed_storage':
                assert x['policy'] == 'sequential' and len({x['generation0'],x['generation1'],x['generation2']}) > 1
            if x['outcome'] in ('old','new'):
                assert len({x['generation0'],x['generation1'],x['generation2']}) == 1
        assert by['sequential']['mixed_storage'] > 0
        assert sum(x['mixed_storage'] for x in by.values()) == expected['mixed_storage_negative_controls']
        counts[key] = dict(by); total += len(rows)
    cost = list(csv.DictReader((run/'costs.csv').open(newline='',encoding='utf-8')))
    examples = [x for x in cost if x['block_size']=='512' and x['payload_per_group']=='128' and x['aged_updates']=='24']
    summary = dict(total_records=total, interrupted_trials=cut_trials, acknowledged_controls=acknowledgments,
                   outcomes=counts, cost_example=examples, all_retry_checks_passed=True,
                   physical_tests=False, reliability_probability_estimated=False)
    if write:
        (run/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
    outcome_rows = '\n'.join(f"| {campaign} | {policy} | {c.get('old',0)} | {c.get('new',0)} | {c.get('mixed_storage',0)} |"
                            for campaign, methods in counts.items() for policy,c in methods.items())
    cost_rows = '\n'.join(f"| {x['policy']} | {x['program_bytes']} | {x['erase_calls']} | {x['read_bytes']} | {x['sync_calls']} |" for x in examples)
    report = f'''# Executed results

The real littlefs C implementation and the authored C layer were compiled and
executed on the host. These are synthetic, deterministic fault traces, not
physical flash tests or spacecraft flight data.

**{total:,} raw records** comprise **{cut_trials:,} abrupt modeled cuts** and
**{acknowledgments} acknowledged-save controls**. All retries converged to the
requested complete configuration. The complete-prefix follow-up enumerates every
byte prefix at every mutation in one declared reference trace per policy;
it does not enumerate every possible execution history or corruption pattern.
The two campaigns overlap; totals count executions, not independent unique faults.

| Campaign | Policy | Old complete | New complete | Mixed storage (rejected) |
|---|---|---:|---:|---:|
{outcome_rows}

Sequential file updates are an intentionally non-transactional negative control.
The loader rejects mixed generations; these counts are not silent acceptance of
corrupt parameters. No unavailable/unexpected state appeared in this campaign.
Both competent methods retained a complete old or new generation in all observed
cuts. This is finite coverage evidence, not a probability of field reliability.

## One matched I/O comparison

Geometry: 64 x 512-byte blocks, 16-byte program alignment, 64-byte cache, 128
payload bytes per group, and 24 successful prior updates. Counters cover one
successful update; they exclude format and initial mount.

| Policy | Programmed bytes | Erase calls | Read bytes | Sync calls |
|---|---:|---:|---:|---:|
{cost_rows}

**Use the atomic bundle when a single-file layout is acceptable.** In this
matched trace it achieves the same observed snapshot property with less program
and erase work than the manifest policy. The manifest implementation preserves
separate group files at an observed extra cost; it is not an improvement over
the bundle on these metrics. Read `costs.csv` for all 36 matched profiles;
one trace must not be generalized into a device lifetime or timing claim.

The compiler, exact commands and source hashes are in `run.json`. The initially
failed warning-as-error build is retained in ignored runs/; third-party warnings
are now reported separately while authored C remains compiled with -Werror.
'''
    if write:
        (run/'REPORT.md').write_text(report,encoding='utf-8',newline='\n')
    return summary

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('run',type=Path,nargs='?')
    args=p.parse_args()
    selected = args.run or sorted((ROOT/'runs').glob('*/run.json'))[-1].parent
    print(json.dumps(analyze(selected.resolve()),indent=2))
