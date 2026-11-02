/* SPDX-License-Identifier: MIT */
#ifndef FC_STORE_H
#define FC_STORE_H
#include "fc_format.h"
#include "lfs.h"
typedef enum { FC_SEQUENTIAL = 0, FC_BUNDLE = 1, FC_MANIFEST = 2 } fc_policy_t;
typedef struct {
    lfs_t *fs;
    uint32_t payload_size;
    uint8_t scratch[FC_RECORD_BYTES];
    uint8_t file_cache[64];
} fc_store_t;
/* One caller at a time. No malloc. Geometry must use a 64-byte littlefs cache.
 * The filesystem must already be mounted; this layer never formats a volume.
 * Load output is valid only on return 0. Diagnostic generations are optional.
 * Save success means all relevant littlefs close/rename operations succeeded.
 * The caller controls generation identity; unsigned wrap is supported as data. */
int fc_store_init(fc_store_t *s, lfs_t *fs, uint32_t payload_size);
int fc_save(fc_store_t *s, fc_policy_t policy, const fc_snapshot_t *value);
int fc_load(fc_store_t *s, fc_policy_t policy, fc_snapshot_t *value,
            uint32_t generations[FC_GROUPS]);
#endif
