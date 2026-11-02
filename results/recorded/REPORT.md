# Executed results

The real littlefs C implementation and the authored C layer were compiled and
executed on the host. These are synthetic, deterministic fault traces, not
physical flash tests or spacecraft flight data.

**11,002 raw records** comprise **10,963 abrupt modeled cuts** and
**39 acknowledged-save controls**. All retries converged to the
requested complete configuration. The complete-prefix follow-up enumerates every
byte prefix at every mutation in one declared reference trace per policy;
it does not enumerate every possible execution history or corruption pattern.
The two campaigns overlap; totals count executions, not independent unique faults.

| Campaign | Policy | Old complete | New complete | Mixed storage (rejected) |
|---|---|---:|---:|---:|
| sampled | sequential | 410 | 32 | 880 |
| sampled | bundle | 1053 | 39 | 0 |
| sampled | manifest | 1721 | 46 | 0 |
| full-prefix | sequential | 708 | 2 | 1418 |
| full-prefix | bundle | 956 | 14 | 0 |
| full-prefix | manifest | 3719 | 4 | 0 |

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
| sequential | 576 | 3 | 1840 | 6 |
| bundle | 448 | 1 | 1984 | 2 |
| manifest | 1136 | 5 | 28640 | 9 |

**Use the atomic bundle when a single-file layout is acceptable.** In this
matched trace it achieves the same observed snapshot property with less program
and erase work than the manifest policy. The manifest implementation preserves
separate group files at an observed extra cost; it is not an improvement over
the bundle on these metrics. Read `costs.csv` for all 36 matched profiles;
one trace must not be generalized into a device lifetime or timing claim.

The compiler, exact commands and source hashes are in `run.json`. The initially
failed warning-as-error build is retained in ignored runs/; third-party warnings
are now reported separately while authored C remains compiled with -Werror.
