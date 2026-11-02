# MCU porting checklist

The authored layer builds as C99 and has no dynamic allocator. A port supplies
littlefs and the device callbacks; `tests/ram_nor.c` is not a device driver.

1. Derive read/program/erase sizes, alignment, address bounds and busy/error
   handling from the actual device and controller documentation.
2. Implement read, program and erase and verify their physical guarantees.
   The modeled byte-prefix failures are not a device specification.
3. Make sync wait for durable completion and flush any controller/cache state.
   A no-op sync is valid here only because modeled writes are immediate.
4. Mount an explicitly provisioned volume. Do not interpret mount failure as
   permission to erase flight configuration automatically.
5. Use a 64-byte littlefs cache for this prototype, or adapt the context and
   revalidate its buffer contract. Serialize callers and respect file lifetimes.
6. Treat nonzero load/save results as failures. Do not use partial output after
   FC_EMIXED, CRC or format errors. Define a mission-appropriate fallback separately.
7. Re-run host tests, ARM smoke and device tests; then test real power interruption,
   exhaustion, bad blocks and any required migrations/concurrent readers.

The recorded ARM fixture has 20,028 bytes in its linked text section and 36,160
bytes BSS, including a 32,768-byte synthetic medium in ordinary RAM. The target
store context is 2,400 bytes; a snapshot is 2,312 bytes and littlefs state 128 bytes.
These are build-specific object/section sizes, not peak RAM, stack high-water,
device flash capacity, WCET or a board qualification result. Compiler flags and
the ELF digest are recorded in `results/arm/arm.json`.
