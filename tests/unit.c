/* SPDX-License-Identifier: MIT */
#include "fc_store.h"
#include "ram_nor.h"
#include "test_support.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while (0)
static ram_nor_t nor;
static lfs_t fs;
static fc_store_t store;
static struct lfs_config cfg;
int main(void) {
    fc_snapshot_t s, out;
    fc_record_view_t view;
    uint8_t wire[FC_RECORD_BYTES], manifest[16];
    uint32_t gen, bank, gens[FC_GROUPS];
    unsigned policy, i, g;
    int n;
    CHECK(fc_crc32((const uint8_t *)"123456789", 9) == UINT32_C(0xcbf43926));
    for (g = 0; g < FC_GROUPS + 1; ++g) {
        uint32_t kind = g < FC_GROUPS ? g : FC_BUNDLE_KIND;
        make_snapshot(&s, UINT32_MAX, FC_MAX_PAYLOAD);
        n = fc_record_encode(wire, sizeof(wire), &s, kind); CHECK(n > 0);
        CHECK(fc_record_decode(wire, (size_t)n, kind, FC_MAX_PAYLOAD, &view) == 0);
        CHECK(view.generation == UINT32_MAX && view.payload_size == FC_MAX_PAYLOAD);
        CHECK(fc_record_decode(wire, (size_t)n - 1, kind, FC_MAX_PAYLOAD, &view) < 0);
        wire[FC_HEADER_BYTES + 7] ^= 1;
        CHECK(fc_record_decode(wire, (size_t)n, kind, FC_MAX_PAYLOAD, &view) == FC_ECRC);
        wire[FC_HEADER_BYTES + 7] ^= 1;
        CHECK(fc_record_decode(wire, (size_t)n, kind, FC_MAX_PAYLOAD - 1, &view) < 0);
    }
    s.payload_size = 0; CHECK(fc_record_encode(wire, sizeof(wire), &s, 0) < 0);
    s.payload_size = FC_MAX_PAYLOAD + 1; CHECK(fc_record_encode(wire, sizeof(wire), &s, 0) < 0);
    make_snapshot(&s, 3, 16); CHECK(fc_record_encode(wire, 3, &s, 0) < 0);
    CHECK(fc_record_encode(wire, sizeof(wire), &s, 3) < 0);
    memset(&fs, 0, sizeof(fs)); memset(&cfg, 0, sizeof(cfg)); fs.cfg = &cfg;
    cfg.cache_size = 128; CHECK(fc_store_init(&store, &fs, 16) == FC_EFORMAT);
    cfg.cache_size = 64; CHECK(fc_store_init(&store, &fs, 16) == 0);
    CHECK(fc_store_init(&store, &fs, 0) == FC_EFORMAT);
    CHECK(fc_manifest_decode(NULL, &gen, &bank) == FC_EFORMAT);
    fc_manifest_encode(manifest, UINT32_MAX, 1);
    CHECK(fc_manifest_decode(manifest, &gen, &bank) == 0 && gen == UINT32_MAX && bank == 1);
    manifest[4] ^= 1; CHECK(fc_manifest_decode(manifest, &gen, &bank) == FC_ECRC);
    fc_manifest_encode(manifest, 0, 2); CHECK(fc_manifest_decode(manifest, &gen, &bank) == FC_EFORMAT);
    for (policy = 0; policy < 3; ++policy) {
        memset(&nor, 0, sizeof(nor)); memset(nor.data, 0xff, sizeof(nor.data));
        CHECK(nor_configure(&nor, &cfg, 512, 64, 16) == 0);
        CHECK(lfs_format(&fs, &cfg) == 0); CHECK(lfs_mount(&fs, &cfg) == 0);
        CHECK(fc_store_init(&store, &fs, 128) == 0);
        CHECK(fc_load(&store, (fc_policy_t)policy, &out, gens) == LFS_ERR_NOENT);
        for (i = 0; i < 27; ++i) {
            uint32_t generation = i == 25 ? UINT32_MAX : i == 26 ? 0u : i + 1u;
            make_snapshot(&s, generation, 128);
            CHECK(fc_save(&store, (fc_policy_t)policy, &s) == 0);
            CHECK(lfs_unmount(&fs) == 0); memset(&fs, 0, sizeof(fs));
            CHECK(lfs_mount(&fs, &cfg) == 0);
            CHECK(fc_load(&store, (fc_policy_t)policy, &out, gens) == 0);
            CHECK(snapshot_equal(&s, &out));
            CHECK(gens[0] == generation && gens[1] == generation && gens[2] == generation);
        }
        CHECK(lfs_unmount(&fs) == 0);
    }
    memset(&nor, 0, sizeof(nor)); memset(nor.data, 0xff, sizeof(nor.data));
    CHECK(nor_configure(&nor, &cfg, 512, 64, 16) == 0);
    memset(wire, 0, 16); CHECK(cfg.prog(&cfg, 0, 0, wire, 16) == 0);
    memset(wire, 0xff, 16); CHECK(cfg.prog(&cfg, 0, 0, wire, 16) == LFS_ERR_CORRUPT);
    CHECK(cfg.prog(&cfg, 0, 1, wire, 16) == LFS_ERR_INVAL);
    CHECK(cfg.read(&cfg, 64, 0, wire, 16) == LFS_ERR_INVAL);
    CHECK(cfg.erase(&cfg, 64) == LFS_ERR_INVAL);
    puts("PASS: CRC vector, wire bounds, malformed/corrupt records, bank validation, generation wrap, 81 durable saves, NOR rules");
    return 0;
}
