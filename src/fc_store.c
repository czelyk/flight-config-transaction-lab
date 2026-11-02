/* SPDX-License-Identifier: MIT */
#include "fc_store.h"
#include <string.h>
static const char *const seq_names[FC_GROUPS] = {"seq0", "seq1", "seq2"};
static const char *const banks[2][FC_GROUPS] = {
    {"b0.0", "b0.1", "b0.2"}, {"b1.0", "b1.1", "b1.2"}
};
int fc_store_init(fc_store_t *s, lfs_t *fs, uint32_t size) {
    if (!s || !fs || !fs->cfg || fs->cfg->cache_size != sizeof(s->file_cache) ||
        !size || size > FC_MAX_PAYLOAD) return FC_EFORMAT;
    memset(s, 0, sizeof(*s)); s->fs = fs; s->payload_size = size;
    return 0;
}
static int put_file(fc_store_t *s, const char *name, const uint8_t *p, size_t n) {
    lfs_file_t f;
    struct lfs_file_config c;
    lfs_ssize_t count;
    int rc, closed;
    memset(&f, 0, sizeof(f)); memset(&c, 0, sizeof(c)); c.buffer = s->file_cache;
    rc = lfs_file_opencfg(s->fs, &f, name, LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY, &c);
    if (rc < 0) return rc;
    count = lfs_file_write(s->fs, &f, p, (lfs_size_t)n);
    closed = lfs_file_close(s->fs, &f);
    if (count < 0) return (int)count;
    if ((size_t)count != n) return LFS_ERR_IO;
    return closed;
}
static int get_file(fc_store_t *s, const char *name, uint8_t *p, size_t capacity) {
    lfs_file_t f;
    struct lfs_file_config c;
    lfs_soff_t n;
    lfs_ssize_t count;
    int rc, closed;
    memset(&f, 0, sizeof(f)); memset(&c, 0, sizeof(c)); c.buffer = s->file_cache;
    rc = lfs_file_opencfg(s->fs, &f, name, LFS_O_RDONLY, &c);
    if (rc < 0) return rc;
    n = lfs_file_size(s->fs, &f);
    if (n < 0 || (size_t)n > capacity) { lfs_file_close(s->fs, &f); return FC_EFORMAT; }
    count = lfs_file_read(s->fs, &f, p, (lfs_size_t)n);
    closed = lfs_file_close(s->fs, &f);
    if (closed < 0) return closed;
    if (count != n) return count < 0 ? (int)count : LFS_ERR_IO;
    return (int)count;
}
static int get_manifest(fc_store_t *s, uint32_t *generation, uint32_t *bank) {
    uint8_t wire[16];
    int rc = get_file(s, "active", wire, sizeof(wire));
    if (rc < 0) return rc;
    if (rc != 16) return FC_EFORMAT;
    return fc_manifest_decode(wire, generation, bank);
}
int fc_save(fc_store_t *s, fc_policy_t policy, const fc_snapshot_t *v) {
    uint32_t bank = 0, old_gen = 0;
    unsigned g;
    int rc, n;
    uint8_t manifest[16];
    if (!s || !s->fs || !v || s->payload_size != v->payload_size ||
        policy < FC_SEQUENTIAL || policy > FC_MANIFEST) return FC_EFORMAT;
    if (policy == FC_BUNDLE) {
        n = fc_record_encode(s->scratch, sizeof(s->scratch), v, FC_BUNDLE_KIND);
        return n < 0 ? n : put_file(s, "bundle", s->scratch, (size_t)n);
    }
    if (policy == FC_MANIFEST) {
        rc = get_manifest(s, &old_gen, &bank);
        if (rc != 0 && rc != LFS_ERR_NOENT) return rc;
        if (rc == 0) bank ^= 1u;
    }
    for (g = 0; g < FC_GROUPS; ++g) {
        n = fc_record_encode(s->scratch, sizeof(s->scratch), v, g);
        if (n < 0) return n;
        rc = put_file(s, policy == FC_SEQUENTIAL ? seq_names[g] : banks[bank][g],
                      s->scratch, (size_t)n);
        if (rc < 0) return rc;
    }
    if (policy == FC_MANIFEST) {
        fc_manifest_encode(manifest, v->generation, bank);
        rc = put_file(s, "stage", manifest, sizeof(manifest));
        if (rc < 0) return rc;
        return lfs_rename(s->fs, "stage", "active");
    }
    return 0;
}
int fc_load(fc_store_t *s, fc_policy_t policy, fc_snapshot_t *v, uint32_t gens[FC_GROUPS]) {
    unsigned g;
    uint32_t bank = 0, manifest_gen = 0, local[FC_GROUPS] = {0};
    int n, rc;
    fc_record_view_t view;
    if (!s || !s->fs || !v || policy < FC_SEQUENTIAL || policy > FC_MANIFEST) return FC_EFORMAT;
    memset(v, 0, sizeof(*v)); v->payload_size = s->payload_size;
    if (gens) memset(gens, 0, FC_GROUPS * sizeof(*gens));
    if (policy == FC_BUNDLE) {
        n = get_file(s, "bundle", s->scratch, sizeof(s->scratch));
        if (n < 0) return n;
        rc = fc_record_decode(s->scratch, (size_t)n, FC_BUNDLE_KIND, s->payload_size, &view);
        if (rc < 0) return rc;
        v->generation = view.generation;
        for (g = 0; g < FC_GROUPS; ++g) {
            memcpy(v->values[g], view.payload + g * s->payload_size, s->payload_size);
            if (gens) gens[g] = view.generation;
        }
        return 0;
    }
    if (policy == FC_MANIFEST) {
        rc = get_manifest(s, &manifest_gen, &bank);
        if (rc < 0) return rc;
    }
    for (g = 0; g < FC_GROUPS; ++g) {
        n = get_file(s, policy == FC_SEQUENTIAL ? seq_names[g] : banks[bank][g],
                      s->scratch, sizeof(s->scratch));
        if (n < 0) return n;
        rc = fc_record_decode(s->scratch, (size_t)n, g, s->payload_size, &view);
        if (rc < 0) return rc;
        local[g] = view.generation;
        if (gens) gens[g] = local[g];
        memcpy(v->values[g], view.payload, s->payload_size);
    }
    if (local[0] != local[1] || local[1] != local[2]) return FC_EMIXED;
    if (policy == FC_MANIFEST && local[0] != manifest_gen) return FC_EMIXED;
    v->generation = local[0];
    return 0;
}
