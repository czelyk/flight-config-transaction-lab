/* SPDX-License-Identifier: MIT */
#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H
#include "fc_store.h"
#include <string.h>
static void make_snapshot(fc_snapshot_t *s, uint32_t generation, uint32_t size) {
    unsigned g, i;
    memset(s, 0, sizeof(*s)); s->generation = generation; s->payload_size = size;
    for (g = 0; g < FC_GROUPS; ++g)
        for (i = 0; i < size; ++i)
            s->values[g][i] = (uint8_t)(generation * 29u + i * 17u + g * 61u + (i >> 3));
}
static int snapshot_equal(const fc_snapshot_t *a, const fc_snapshot_t *b) {
    unsigned g;
    if (a->generation != b->generation || a->payload_size != b->payload_size) return 0;
    for (g = 0; g < FC_GROUPS; ++g)
        if (memcmp(a->values[g], b->values[g], a->payload_size)) return 0;
    return 1;
}
#endif
