# Local validation — 1 November 2026

## Executed checks

| Check | Actual result | Evidence |
|---|---|---|
| Native C99 unit suite | CRC vector, format/buffer errors, NOR rules and 81 durable saves including unsigned generation wrap passed | `results/recorded/unit.log` |
| Sampled mutation campaign | 4,181 rows, required assertions passed | `results/recorded/campaign.csv` |
| Complete-prefix follow-up | 6,821 rows, required assertions passed | `results/recorded/full_prefix.csv` |
| Independent CSV checks | 10,963 interrupted trials and 39 acknowledged controls; every retry passed | `results/recorded/summary.json` |
| Debug CMake/CTest | Three tests passed from a fresh build directory; four CSVs match optimized-run outputs after LF normalization | `results/cmake/` |
| Cortex-M4 execution | All three policies, 36 nominal save/reset/load cycles, no dynamic allocator symbols | `results/arm/arm.json` |
| Single-case witness | Exact recorded mixed-generation case reproduced by C, including recovery I/O counters | `results/witness/witness.json` |
| Clean extracted source copy | Verify/build/campaign/analyze/replay passed; four CSVs exactly match recorded outputs | `provenance/package-check.json` |

Sequential updates deliberately expose mixed storage; the reader rejects those
snapshots. Bundle and manifest return complete old/new snapshots in the observed
trials. The two campaigns overlap. Their total is execution coverage, not an
independent failure count or an estimated reliability probability.

## Tool identity

Native: Python 3.12.10 on Windows, Zig 0.14.1 / Clang 19.1.7. CMake: installed
local CMake with Ninja 1.13.0 and the same public C driver, Debug configuration.
ARM: Arm GNU 15.2.1, Unicorn 2.1.4 and pyelftools 0.32. Exact commands, C-source
digests, upstream identity and the ARM ELF digest are included with raw results.

The source-copy test reused the already downloaded public Zig executable through
the explicit --zig option. The extracted implementation used its own relative
source paths and vendored dependency; it did not read code from the original
project. Fresh compiler download was separately exercised during initial setup.

## Interpretation and remaining work

Use the bundle where a single-file format is acceptable. The more involved
manifest is useful here as a separate-file comparison and has no claimed cost
superiority. All transaction mechanisms are established prior art.

No physical board, power supply, flash/controller, target timing, radiation,
flight data, NASA component integration or complete conference contribution was
validated. ARM power-cut testing, out-of-space/error cases, bad blocks, migration
and concurrent clients remain outside the executed scope. Remote GitHub Actions
and native Linux execution have not been run locally; the workflow is provided
as configuration. An internal skeptical review is in docs/REVIEW.md.

Source delivery excludes downloaded toolchains, environments, executables and
working caches. The default commands create fresh runs and preserve archived
evidence. The paused research automation was not resumed, and nothing was
published to GitHub or sent externally.
