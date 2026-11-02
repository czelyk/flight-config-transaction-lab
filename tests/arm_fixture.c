/* SPDX-License-Identifier: MIT */
#include "fc_store.h"
#include "ram_nor.h"
#include "test_support.h"
#include <string.h>
static ram_nor_t nor;
static struct lfs_config config;
static lfs_t fs;
static fc_store_t store;
uint32_t arm_results[8];
int arm_smoke(void) {
    fc_snapshot_t desired, observed;
    unsigned policy, i;
    memset(arm_results, 0, sizeof(arm_results));
    for (policy = 0; policy < 3; ++policy) {
        memset(&nor, 0, sizeof(nor)); memset(nor.data, 0xff, sizeof(nor.data));
        if (nor_configure(&nor, &config, 512, 64, 16) < 0) return 1;
        memset(&fs, 0, sizeof(fs));
        if (lfs_format(&fs, &config) < 0 || lfs_mount(&fs, &config) < 0) return 2;
        if (fc_store_init(&store, &fs, 128) < 0) return 3;
        for (i = 0; i < 12; ++i) {
            uint32_t generation = i == 10 ? UINT32_MAX : i == 11 ? 0u : i + 1u;
            make_snapshot(&desired, generation, 128);
            if (fc_save(&store, (fc_policy_t)policy, &desired) < 0) return 4;
            /* Drop all littlefs/application state without unmounting. */
            memset(&fs, 0, sizeof(fs));
            if (lfs_mount(&fs, &config) < 0 || fc_store_init(&store, &fs, 128) < 0) return 5;
            if (fc_load(&store, (fc_policy_t)policy, &observed, 0) < 0 ||
                !snapshot_equal(&desired, &observed)) return 6;
            ++arm_results[1];
        }
        ++arm_results[0];
    }
    arm_results[2] = (uint32_t)sizeof(fc_store_t);
    arm_results[3] = (uint32_t)sizeof(fc_snapshot_t);
    arm_results[4] = (uint32_t)sizeof(lfs_t);
    arm_results[5] = NOR_MAX_BYTES;
    arm_results[6] = 1; /* no injected cuts in this target smoke test */
    return 0;
}
