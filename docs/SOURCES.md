# Primary sources and known prior work

Checked 1 November 2026 through public, anonymous access.

1. [NASA F Prime PrmDb design](https://github.com/nasa/fprime/blob/devel/Svc/PrmDb/docs/sdd.md).
   Parameters are loaded at startup, served to components and saved to a file.
   The document already describes saving the whole table. This is a spacecraft
   function/context source, not a missing-feature claim. The exact retrieved
   commit, source URL and document digest are in
   [space-context.json](../provenance/space-context.json). Browser access to an
   initially guessed implementation path failed; the correct public design
   document was acquired via the repository API/raw URL. No restricted source is
   required, and no security mechanism is studied here.
2. [littlefs README](https://github.com/littlefs-project/littlefs/blob/6cb4e86540eca0d9ba62500a298385c9d863c8be/README.md).
   It explicitly documents power-loss resilience, bounded configurable buffers,
   atomic POSIX operations and file updates committed by sync/close. These are
   predecessor capabilities, not discoveries by this project.
3. [littlefs design](https://github.com/littlefs-project/littlefs/blob/6cb4e86540eca0d9ba62500a298385c9d863c8be/DESIGN.md).
   Metadata logs, copy-on-write data structures and wear leveling already address
   the underlying filesystem problem. The unchanged source and documents are
   vendored with their BSD-3-Clause license and byte hashes.
4. [littlefs upstream tests](https://github.com/littlefs-project/littlefs/blob/6cb4e86540eca0d9ba62500a298385c9d863c8be/tests/test_files.toml)
   already exercise file updates and reentrant recovery. This lab is a separate
   application-group/retry/I/O comparison, not the first littlefs fault campaign.
5. [Zephyr NVS documentation](https://docs.zephyrproject.org/latest/services/storage/nvs/nvs.html).
   It documents data/metadata commit ordering, optional data CRC and space/wear
   handling for persistent elements. This credits mature embedded persistence
   alternatives; no Zephyr implementation is benchmarked in the checked-in results.

## Contribution boundary

Atomic rename, commit manifests, CRC32, double banking and bundling are established
mechanisms. The original material here is the compact C coordination prototype,
its explicit application contract, reusable interruption harness, matched cost
traces, and executable CSV witness. A literature claim of unique novelty has not
been established. No NASA flight data, spacecraft failure, deployed integration,
independent peer review or conference selection is claimed.
