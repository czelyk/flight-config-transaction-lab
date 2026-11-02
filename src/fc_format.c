/* SPDX-License-Identifier: MIT */
#include "fc_format.h"
#include <string.h>
#define RECORD_MAGIC UINT32_C(0x31474346)
#define MANIFEST_MAGIC UINT32_C(0x314d4346)
void fc_put32(uint8_t *p, uint32_t v) {
    unsigned i;
    for (i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8u * i));
}
uint32_t fc_get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
uint32_t fc_crc32(const uint8_t *p, size_t n) {
    uint32_t c = UINT32_MAX;
    size_t i;
    unsigned b;
    for (i = 0; i < n; ++i) {
        c ^= p[i];
        for (b = 0; b < 8; ++b)
            c = (c >> 1) ^ (UINT32_C(0xedb88320) & (0u - (c & 1u)));
    }
    return ~c;
}
int fc_record_encode(uint8_t *out, size_t capacity, const fc_snapshot_t *s,
                     uint32_t kind) {
    uint32_t count = kind == FC_BUNDLE_KIND ? FC_GROUPS : 1u;
    size_t length;
    unsigned i;
    if (!out || !s || !s->payload_size || s->payload_size > FC_MAX_PAYLOAD ||
        (kind != FC_BUNDLE_KIND && kind >= FC_GROUPS)) return FC_EFORMAT;
    length = FC_HEADER_BYTES + count * s->payload_size + 4u;
    if (capacity < length) return FC_EFORMAT;
    fc_put32(out, RECORD_MAGIC);
    fc_put32(out + 4, s->generation);
    fc_put32(out + 8, s->payload_size);
    fc_put32(out + 12, kind);
    fc_put32(out + 16, count);
    for (i = 0; i < count; ++i)
        memcpy(out + FC_HEADER_BYTES + i * s->payload_size,
               s->values[kind == FC_BUNDLE_KIND ? i : kind], s->payload_size);
    fc_put32(out + length - 4, fc_crc32(out, length - 4));
    return (int)length;
}
int fc_record_decode(const uint8_t *p, size_t n, uint32_t kind, uint32_t size,
                     fc_record_view_t *view) {
    uint32_t count = kind == FC_BUNDLE_KIND ? FC_GROUPS : 1u;
    if (!p || !view || !size || size > FC_MAX_PAYLOAD || n < FC_HEADER_BYTES + 4u ||
        (kind != FC_BUNDLE_KIND && kind >= FC_GROUPS)) return FC_EFORMAT;
    if (fc_get32(p) != RECORD_MAGIC || fc_get32(p + 8) != size ||
        fc_get32(p + 12) != kind || fc_get32(p + 16) != count ||
        n != FC_HEADER_BYTES + count * size + 4u) return FC_EFORMAT;
    if (fc_get32(p + n - 4) != fc_crc32(p, n - 4)) return FC_ECRC;
    view->generation = fc_get32(p + 4);
    view->payload_size = size;
    view->kind = kind;
    view->count = count;
    view->payload = p + FC_HEADER_BYTES;
    return 0;
}
void fc_manifest_encode(uint8_t out[16], uint32_t gen, uint32_t bank) {
    fc_put32(out, MANIFEST_MAGIC);
    fc_put32(out + 4, gen);
    fc_put32(out + 8, bank);
    fc_put32(out + 12, fc_crc32(out, 12));
}
int fc_manifest_decode(const uint8_t in[16], uint32_t *gen, uint32_t *bank) {
    if (!in || !gen || !bank) return FC_EFORMAT;
    if (fc_get32(in) != MANIFEST_MAGIC || fc_get32(in + 8) > 1u) return FC_EFORMAT;
    if (fc_get32(in + 12) != fc_crc32(in, 12)) return FC_ECRC;
    *gen = fc_get32(in + 4);
    *bank = fc_get32(in + 8);
    return 0;
}
