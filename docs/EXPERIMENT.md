# Experiment contract and fault model

## Question and competent comparison

For a synthetic configuration split into three groups, compare the observed
snapshot/retry property and persistent I/O cost of one atomic bundle with a
manifest selecting a complete two-bank group. Sequential independent updates
are an intentionally insufficient control. All three use identical littlefs
source, cache size, group content, geometry and preparation history per profile.

This is an application-layout comparison, not a filesystem contest. A single
bundle is an adequate existing solution. Separate files are a design option,
not a documented mission requirement asserted by this lab.

## Campaign

The sampled matrix is 3 policies x 2 geometries x 3 payload lengths x 2 histories:
36 reference traces. Geometries are 64 x 512-byte blocks / 16-byte program
alignment and 32 x 4,096-byte blocks / 64-byte program alignment. Every profile
uses a 64-byte cache, a 16-byte lookahead buffer and block_cycles=16. The latter
is a test setting to exercise metadata evolution, not a device endurance rating.

Each group contains 16, 128 or 768 synthetic payload bytes. The volume is already
initialized with a valid generation. Histories add 0 or 24 successful updates.
The initialization/format operation is not itself power-cut tested.

1. Prepare and snapshot the modeled durable bytes.
2. Execute one successful update and record every program/erase callback.
3. Restore that initial medium for each independent trial.
4. At each mutation, interrupt before, after 1 byte, halfway through, one byte
   before completion, and after completion. Deduplicate equal prefix lengths.
5. Abandon the C/littlefs runtime immediately with setjmp/longjmp; do not perform
   a clean close or unmount. No dynamic allocation is used in the abandoned layer.
6. Disable injection, reconstruct CPU state, mount, load and validate the result.
7. Retry the requested generation, reset again and verify the exact new bytes.

The follow-up fixes the 512-byte-block geometry, 128-byte group payloads and
24-update history and executes **every byte prefix of every mutation in that
one reference trace for each policy**. It overlaps sampled coordinates. It is
trace-complete byte-prefix coverage, not exhaustive software-state verification.

The source oracle compares the exact generation, payload length and all three
payload byte arrays. It does not ask the implementation to decide what the
expected result should be. The Python analyzer independently checks the CSV
classification, per-group generation diagnostics, counts and retry flags.

## Required observations

- After an interrupted competent-policy save: exact complete old or new snapshot.
- After a save returned success: exact new snapshot survives reset.
- A mixed sequential store is diagnosed as FC_EMIXED; its partial output is not
  a usable application snapshot.
- No unavailable/unexpected result, silent byte mismatch or failed retry is allowed.
- Loading/recovery in these traces must issue zero program/erase callbacks.
- The sequential negative control must actually expose mixed storage somewhere.

The harness exits nonzero if any required assertion fails. The raw `outcome`
column retains negative-control observations; they are not relabeled as success.

## Model assumptions and exclusions

The modeled medium changes requested program bytes only from 1 to 0 and erase
prefix bytes to 0xff. Completed writes are immediately persistent, so sync is
a counted no-op. Reads are exact and stable. A torn operation affects a byte
prefix of its requested range and does not corrupt neighboring blocks.

These are declared test assumptions, not a NOR-chip characterization. Arbitrary
bit subsets, disturb effects, cached controllers, lost sync, read faults, bad
blocks, wear limits, out-of-space operations and actual brownout behavior are not
validated. No elapsed C execution time is interpreted as a target deadline.

## Measurements

Successful-update counters exclude preparation and the initial mount. Recovery
reads include remount plus load, before retry. `program_bytes` counts bytes passed
to completed modeled programming; `erase_calls` and `erase_bytes` describe this
one trace. They do not predict energy, latency, write endurance or flight risk.

The ARM fixture executes the real implementation but only nominal save/reset/load
cycles. Its compiler section sizes include the whole linked test fixture. The
modeled medium occupies ordinary emulator RAM. No MCU board was connected.
