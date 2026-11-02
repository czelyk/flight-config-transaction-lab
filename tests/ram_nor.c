/* SPDX-License-Identifier: MIT */
#include "ram_nor.h"
#include <string.h>
static int bounds(const ram_nor_t *n, lfs_block_t b, lfs_off_t off, lfs_size_t size) {
    return b < n->block_count && off <= n->block_size && size <= n->block_size - off;
}
static int rd(const struct lfs_config *c, lfs_block_t b, lfs_off_t off, void *p, lfs_size_t size) {
    ram_nor_t *n = c->context;
    if (!bounds(n, b, off, size) || off % c->read_size || size % c->read_size) return LFS_ERR_INVAL;
    ++n->stats.read_calls; n->stats.read_bytes += size;
    memcpy(p, n->data + b * n->block_size + off, size); return 0;
}
static uint32_t mutation(ram_nor_t *n, char kind, uint32_t size) {
    ++n->mutations;
    if (n->mutations <= NOR_TRACE_CAP) {
        n->trace[n->mutations - 1].kind = kind;
        n->trace[n->mutations - 1].size = size;
    }
    return n->cut_at == n->mutations && n->powercut ?
            (n->prefix < size ? n->prefix : size) : size;
}
static void maybe_cut(ram_nor_t *n) {
    if (n->cut_at == n->mutations && n->powercut) n->powercut(n->cut_user);
}
static int pg(const struct lfs_config *c, lfs_block_t b, lfs_off_t off,
               const void *p, lfs_size_t size) {
    ram_nor_t *n = c->context;
    const uint8_t *source = p;
    uint8_t *target;
    uint32_t i, amount;
    if (!bounds(n, b, off, size) || off % c->prog_size || size % c->prog_size) return LFS_ERR_INVAL;
    target = n->data + b * n->block_size + off;
    for (i = 0; i < size; ++i) if ((target[i] & source[i]) != source[i]) return LFS_ERR_CORRUPT;
    amount = mutation(n, 'P', size);
    ++n->stats.program_calls; n->stats.program_bytes += amount;
    for (i = 0; i < amount; ++i) target[i] &= source[i];
    maybe_cut(n); return 0;
}
static int er(const struct lfs_config *c, lfs_block_t b) {
    ram_nor_t *n = c->context;
    uint32_t amount;
    if (b >= n->block_count) return LFS_ERR_INVAL;
    amount = mutation(n, 'E', n->block_size);
    ++n->stats.erase_calls; n->stats.erase_bytes += amount;
    memset(n->data + b * n->block_size, 0xff, amount);
    maybe_cut(n); return 0;
}
static int sy(const struct lfs_config *c) {
    ram_nor_t *n = c->context;
    ++n->stats.sync_calls;
    /* Writes hit persistent modeled bytes immediately; no volatile write cache. */
    return 0;
}
void nor_reset_stats(ram_nor_t *n) {
    n->mutations = 0; memset(&n->stats, 0, sizeof(n->stats));
}
int nor_configure(ram_nor_t *n, struct lfs_config *c, uint32_t block,
                   uint32_t count, uint32_t prog) {
    if (!block || !count || block > NOR_MAX_BYTES / count ||
        !prog || prog > 64 || 64 % prog || block % 64) return LFS_ERR_INVAL;
    n->block_size = block; n->block_count = count; n->prog_size = prog;
    n->cut_at = 0; n->powercut = 0; n->cut_user = 0; nor_reset_stats(n);
    memset(c, 0, sizeof(*c));
    c->context = n; c->read = rd; c->prog = pg; c->erase = er; c->sync = sy;
    c->read_size = 16; c->prog_size = prog; c->block_size = block; c->block_count = count;
    c->block_cycles = 16; c->cache_size = 64; c->lookahead_size = 16;
    c->read_buffer = n->read_cache; c->prog_buffer = n->prog_cache; c->lookahead_buffer = n->lookahead;
    return 0;
}
