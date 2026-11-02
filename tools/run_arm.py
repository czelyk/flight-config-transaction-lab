"""Execute the real C/littlefs nominal fixture as Cortex-M4 code in Unicorn."""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import shutil
import struct
import subprocess
import uuid
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_R0, UC_ARM_REG_SP
from run_tests import ROOT, CORE, DEFINES, execute

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--gcc', default=os.environ.get('ARM_GCC') or shutil.which('arm-none-eabi-gcc'))
    args = p.parse_args()
    if not args.gcc:
        raise SystemExit('Install Arm GNU Toolchain or set ARM_GCC.')
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    run = ROOT / 'runs' / (stamp + '_arm_' + uuid.uuid4().hex[:6]); run.mkdir(parents=True)
    elf = run / 'fixture.elf'
    gcc = Path(args.gcc).resolve()
    flags = ['-mcpu=cortex-m4', '-mthumb', '-O2', '-std=c99', '-ffreestanding', '-fno-builtin',
             '-ffunction-sections', '-fdata-sections', '-fstack-usage', '-nostartfiles', '-nostdlib',
             '-DNOR_MAX_BYTES=32768', '-DNOR_TRACE_CAP=64', '-DLFS_NO_ASSERT']
    command = [gcc, *flags, *DEFINES, '-I'+str(ROOT/'include'), '-I'+str(ROOT/'tests'),
               '-I'+str(ROOT/'third_party/littlefs'), *[ROOT/x for x in CORE], ROOT/'tests/arm_fixture.c',
               '-T', ROOT/'cmake/arm-emulation.ld', '-Wl,--gc-sections', '-Wl,-Map='+str(run/'fixture.map'),
               '-lc', '-lgcc', '-o', elf]
    execute(command, run, run/'build.log')
    nm = gcc.with_name('arm-none-eabi-nm'+gcc.suffix)
    undefined = execute([nm, '-u', elf], run, run/'undefined-symbols.txt')
    assert not undefined.strip(), undefined
    symbols_text = execute([nm, elf], run, run/'symbols.txt')
    assert not any(line.split()[-1] in ('malloc', 'calloc', 'realloc', '_sbrk') for line in symbols_text.splitlines())
    with elf.open('rb') as f:
        e = ELFFile(f)
        u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        u.mem_map(0x10000, 0x40000); u.mem_map(0x20000000, 0x20000)
        for segment in e.iter_segments():
            if segment['p_type'] == 'PT_LOAD' and segment['p_filesz']:
                u.mem_write(segment['p_vaddr'], segment.data())
        symbols = {s.name:s['st_value'] for s in e.get_section_by_name('.symtab').iter_symbols()}
    stop = 0x4f000
    u.reg_write(UC_ARM_REG_SP, 0x2001fff8); u.reg_write(UC_ARM_REG_LR, stop | 1)
    u.emu_start(symbols['arm_smoke'] | 1, stop, timeout=60_000_000, count=100_000_000)
    assert u.reg_read(UC_ARM_REG_PC) == stop, 'ARM fixture exceeded the execution bound'
    rc = u.reg_read(UC_ARM_REG_R0)
    values = struct.unpack('<8I', bytes(u.mem_read(symbols['arm_results'], 32)))
    assert rc == 0 and values[:2] == (3,36), (rc,values)
    size_tool = gcc.with_name('arm-none-eabi-size'+gcc.suffix)
    size = execute([size_tool, '-B', elf], run, run/'size.txt')
    version = execute([gcc,'--version'],run,run/'compiler-version.txt').splitlines()[0]
    source_hashes = {p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                     for folder in ('include','src','tests') for p in sorted((ROOT/folder).rglob('*'))
                     if p.suffix in ('.c','.h')}
    report = dict(pass_all=True, completed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  compiler=version, command=[str(x) for x in command], source_sha256=source_hashes,
                  elf_sha256=hashlib.sha256(elf.read_bytes()).hexdigest(),
                  policies=values[0], nominal_save_reset_load_cycles=values[1], store_context_bytes=values[2],
                  snapshot_bytes=values[3], littlefs_state_bytes=values[4], modeled_medium_bytes=values[5],
                  size_output=size, no_dynamic_allocator_symbols=True, physical_tests=False, target_timing=False,
                  scope='Real Cortex-M4 C instructions and littlefs; nominal functional smoke only. Host campaign owns power-cut coverage.')
    (run/'arm.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(dict(run=str(run),pass_all=True,policies=values[0],nominal_cycles=values[1],
                         store_context_bytes=values[2],size_output=size,physical_tests=False),indent=2))

if __name__ == '__main__':
    main()
