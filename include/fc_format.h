/* SPDX-License-Identifier: MIT */
#ifndef FC_FORMAT_H
#define FC_FORMAT_H
#include <stddef.h>
#include <stdint.h>
#define FC_GROUPS 3u
#define FC_MAX_PAYLOAD 768u
#define FC_HEADER_BYTES 20u
#define FC_RECORD_BYTES (FC_HEADER_BYTES + FC_GROUPS * FC_MAX_PAYLOAD + 4u)
#define FC_BUNDLE_KIND UINT32_MAX
enum { FC_EFORMAT = -1001, FC_ECRC = -1002, FC_EMIXED = -1003 };
typedef struct {
    uint32_t generation, payload_size;
    uint8_t values[FC_GROUPS][FC_MAX_PAYLOAD];
} fc_snapshot_t;
typedef struct {
    uint32_t generation, payload_size, kind, count;
    const uint8_t *payload;
} fc_record_view_t;
uint32_t fc_crc32(const uint8_t *data, size_t size);
void fc_put32(uint8_t *p, uint32_t value);
uint32_t fc_get32(const uint8_t *p);
int fc_record_encode(uint8_t *out, size_t capacity, const fc_snapshot_t *value,
                     uint32_t kind);
int fc_record_decode(const uint8_t *data, size_t size, uint32_t expected_kind,
                     uint32_t expected_payload_size, fc_record_view_t *view);
void fc_manifest_encode(uint8_t out[16], uint32_t generation, uint32_t bank);
int fc_manifest_decode(const uint8_t data[16], uint32_t *generation, uint32_t *bank);
#endif
