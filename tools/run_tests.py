"""Build the real C code and run deterministic power-cut campaigns locally."""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[1]
CORE = ['src/fc_format.c', 'src/fc_store.c', 'tests/ram_nor.c',
        'third_party/littlefs/lfs.c', 'third_party/littlefs/lfs_util.c']
DEFINES = ['-DLFS_NO_MALLOC', '-DLFS_NO_DEBUG', '-DLFS_NO_WARN', '-DLFS_NO_ERROR']

def compiler(selected=None, zig=None):
    if zig:
        return [str(Path(zig).resolve()), 'cc']
    if selected:
        return [selected]
    for name in ('gcc', 'clang', 'cc'):
        if shutil.which(name):
            return [shutil.which(name)]
    if os.name == 'nt':
        from bootstrap_zig import bootstrap
        return [str(bootstrap()), 'cc']
    raise RuntimeError('Install a C99 compiler (GCC or Clang), or pass --cc.')

def execute(command, cwd, log, timeout=600):
    result = subprocess.run([str(x) for x in command], cwd=cwd, capture_output=True, text=True, timeout=timeout)
    log.write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'Command failed ({result.returncode}); see {log}\n{result.stderr[-2000:]}')
    return result.stdout

def main():
    p = argparse.ArgumentParser()
    group=p.add_mutually_exclusive_group()
    group.add_argument('--cc', help='GCC/Clang executable; Windows defaults to pinned local Zig if absent')
    group.add_argument('--zig', help='An existing public Zig executable; useful for a clean source-copy check')
    p.add_argument('--quick', action='store_true', help='unit tests and sampled matrix only')
    args = p.parse_args()
    cc = compiler(args.cc,args.zig)
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    run = ROOT / 'runs' / (stamp + '_' + uuid.uuid4().hex[:6])
    run.mkdir(parents=True)
    version = execute(cc + ['--version'], ROOT, run / 'compiler-version.txt')
    upstream = json.loads((ROOT / 'provenance/upstream.json').read_text())
    for item in upstream['files']:
        assert hashlib.sha256((ROOT / item['path']).read_bytes()).hexdigest() == item['sha256']
    commands = []
    flags = ['-std=c99', '-O2', '-g', '-Wall', '-Wextra'] + DEFINES + ['-Iinclude', '-Itests', '-Ithird_party/littlefs']
    objects = []
    for index, source in enumerate(CORE):
        obj = run / (str(index) + '.o')
        command = cc + flags + ([] if source.startswith('third_party/') else ['-Werror']) + ['-c', source, '-o', str(obj)]
        execute(command, ROOT, run / ('compile-' + str(index) + '.log'))
        commands.append(command)
        objects.append(str(obj))
    for target, test in [('unit', 'tests/unit.c'), ('campaign', 'tests/campaign.c')]:
        binary = run / (target + ('.exe' if os.name == 'nt' else ''))
        command = cc + flags + ['-Werror'] + objects + [test, '-o', str(binary)]
        execute(command, ROOT, run / (target + '-build.log'))
        commands.append(command)
    execute([run / ('unit.exe' if os.name == 'nt' else 'unit')], run, run / 'unit.log')
    campaign = run / ('campaign.exe' if os.name == 'nt' else 'campaign')
    outputs = [json.loads(execute([campaign], run, run / 'campaign.log'))]
    if not args.quick:
        outputs.append(json.loads(execute([campaign, '--full-prefix'], run, run / 'full-prefix.log')))
    source_hashes = {p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                     for folder in ('include','src','tests') for p in sorted((ROOT/folder).rglob('*'))
                     if p.suffix in ('.c','.h')}
    record = dict(completed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  host=platform.platform(), python=platform.python_version(), compiler=version.splitlines()[0],
                  commands=commands, source_sha256=source_hashes, upstream_commit=upstream['commit'], campaigns=outputs,
                  physical_tests=False, target_timing=False, pass_all=all(x['failures'] == 0 for x in outputs))
    (run / 'run.json').write_text(json.dumps(record, indent=2), encoding='utf-8')
    print(json.dumps(dict(run=str(run), compiler=record['compiler'], campaigns=outputs,
                         pass_all=record['pass_all'], physical_tests=False), indent=2))

if __name__ == '__main__':
    main()
