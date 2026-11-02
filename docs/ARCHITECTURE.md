# Architecture and wire format

The reusable layer is `fc_format` plus `fc_store`. `ram_nor` is exclusively a test
double. The caller owns the mounted littlefs instance and supplies serialization
access; the layer contains no scheduler, flash driver or locking primitive.

## Layouts

- Sequential: `seq0`, `seq1`, `seq2` are individually closed after update. The
  loader rejects unequal generations, but no old complete group is retained.
- Bundle: `bundle` contains all three payloads. One successful littlefs close
  commits the new file data/metadata using littlefs's existing machinery.
- Manifest: `b0.0` through `b0.2` and `b1.0` through `b1.2` form two banks. Write
  only the inactive bank, close each file, close a `stage` manifest, then atomically
  rename `stage` over `active`. Boot follows `active` and verifies each referenced
  record's identity and generation. It never selects the highest staged generation.

The previous active bank is not overwritten before the manifest switch. On the
next successful update it becomes the inactive bank. No generation ordering is
needed; unsigned generation wrap is handled as identity, and is unit tested.

## Record format

All integers are 32-bit little-endian, encoded bytewise.

| Offset | Content |
|---:|---|
| 0 | Magic/version `FCG1` |
| 4 | Generation identity |
| 8 | Payload bytes per group, 1..768 |
| 12 | Group identity 0..2, or UINT32_MAX for a bundle |
| 16 | Group count, 1 or 3 |
| 20 | Payloads in group order |
| End - 4 | IEEE CRC32 of header and payload |

The 16-byte manifest contains magic/version `FCM1`, generation, bank index 0/1,
and CRC32. Record size, group identity, payload size, CRC and referenced generation
are checked before a usable snapshot is returned.

## Integration contract

`fc_store_init` requires a mounted littlefs instance and a 64-byte cache. The
store context owns fixed serialization/file-cache buffers. `fc_load` output is
valid only when the return value is zero. Negative errors must not be ignored.
`fc_save` returns success after all relevant littlefs closes/rename succeed.

The volume must have enough free space for the workload and copy-on-write process.
Initialization, migration, shared-reader lifetimes, arbitrary I/O errors and
resource exhaustion need additional design/testing before deployment. Never
automatically format a failed volume in a flight port. Preserve a recovery path.
