/* SPDX-License-Identifier: MIT */
#ifndef RAM_NOR_H
#define RAM_NOR_H
#include "lfs.h"
#include <stddef.h>
#include <stdint.h>
#ifndef NOR_MAX_BYTES
#define NOR_MAX_BYTES 131072u
#endif
#ifndef NOR_TRACE_CAP
#define NOR_TRACE_CAP 1024u
#endif
typedef struct { uint32_t size; char kind; } nor_event_t;
typedef struct {
    uint64_t read_calls, read_bytes, program_calls, program_bytes, erase_calls, erase_bytes, sync_calls;
} nor_stats_t;
typedef struct {
    uint8_t data[NOR_MAX_BYTES];
    uint32_t block_size, block_count, prog_size;
    uint32_t mutations, cut_at, prefix;
    nor_event_t trace[NOR_TRACE_CAP];
    nor_stats_t stats;
    void (*powercut)(void *user);
    void *cut_user;
    uint8_t read_cache[64], prog_cache[64], lookahead[16];
} ram_nor_t;
int nor_configure(ram_nor_t *n, struct lfs_config *c, uint32_t block_size,
                   uint32_t block_count, uint32_t prog_size);
void nor_reset_stats(ram_nor_t *n);
#endif
