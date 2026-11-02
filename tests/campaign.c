/* SPDX-License-Identifier: MIT */
#include "fc_store.h"
#include "ram_nor.h"
#include "test_support.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ram_nor_t nor;
static uint8_t baseline[NOR_MAX_BYTES];
static lfs_t fs;
static fc_store_t store;
static struct lfs_config cfg;
static fc_snapshot_t old_value, new_value, recovered;
static jmp_buf reset_boundary;
static const char *const names[] = {"sequential", "bundle", "manifest"};
static FILE *csv, *costs;
static unsigned failures, mixed, rows;
static uint32_t current_size;
static int replay_mode;
static uint32_t replay_cut, replay_prefix;

static void fatal(const char *message, int rc) {
    fprintf(stderr, "Harness prerequisite failed: %s (%d)\n", message, rc); exit(2);
}
static void require(int rc, const char *message) { if (rc < 0) fatal(message, rc); }
static void reset_cpu(void) {
    memset(&fs, 0, sizeof(fs));
    require(lfs_mount(&fs, &cfg), "mount");
    require(fc_store_init(&store, &fs, current_size), "store init");
}
static void powercut(void *unused) { (void)unused; longjmp(reset_boundary, 1); }
static int attempt(fc_policy_t policy) {
    if (setjmp(reset_boundary)) return 1;
    require(fc_save(&store, policy, &new_value), "save without modeled power cut");
    return 0;
}
static int classify(fc_policy_t policy, uint32_t gens[FC_GROUPS]) {
    int rc;
    nor.cut_at = 0; nor.powercut = NULL; nor_reset_stats(&nor);
    reset_cpu();
    rc = fc_load(&store, policy, &recovered, gens);
    if (rc == FC_EMIXED) return 2;
    if (rc < 0) return 3;
    if (snapshot_equal(&old_value, &recovered)) return 0;
    if (snapshot_equal(&new_value, &recovered)) return 1;
    return 4;
}
static void trial(fc_policy_t policy, unsigned age, uint32_t cut,
                   uint32_t prefix, char kind, uint32_t operation_size) {
    int interrupted, state, retry_ok;
    uint32_t generations[FC_GROUPS];
    nor_stats_t recovery_stats;
    static const char *const states[] = {"old", "new", "mixed_storage", "unavailable", "unexpected"};
    memcpy(nor.data, baseline, cfg.block_size * cfg.block_count);
    nor.cut_at = 0; nor.powercut = NULL; reset_cpu(); nor_reset_stats(&nor);
    nor.cut_at = cut; nor.prefix = prefix; nor.powercut = powercut;
    interrupted = attempt(policy);
    if (cut && !interrupted) fatal("cut point not reached", (int)cut);
    state = classify(policy, generations); recovery_stats = nor.stats;
    if (state == 2) ++mixed;
    if ((policy != FC_SEQUENTIAL && state == 2) || state >= 3 || (!interrupted && state != 1)) ++failures;
    /* A rejected mixed sequential snapshot is never passed to an application. */
    require(fc_save(&store, policy, &new_value), "retry after reset");
    reset_cpu(); retry_ok = fc_load(&store, policy, &recovered, NULL) == 0 && snapshot_equal(&new_value, &recovered);
    if (!retry_ok || recovery_stats.program_calls || recovery_stats.erase_calls) ++failures;
    fprintf(csv, "%s,%u,%u,%u,%u,%u,%u,%c,%u,%d,%s,%u,%u,%u,%llu,%llu,%d\n",
            names[policy], (unsigned)cfg.block_size, (unsigned)cfg.block_count, (unsigned)current_size,
            age, cut, prefix, kind, operation_size, !interrupted, states[state],
            generations[0], generations[1], generations[2],
            (unsigned long long)recovery_stats.read_calls, (unsigned long long)recovery_stats.read_bytes, retry_ok);
    ++rows;
}
static void scenario(fc_policy_t policy, uint32_t block, uint32_t count,
                      uint32_t prog, uint32_t size, unsigned age, int all_prefixes) {
    unsigned i;
    uint32_t op, total;
    static nor_event_t events[NOR_TRACE_CAP];
    nor_stats_t stats;
    current_size = size;
    memset(&nor, 0, sizeof(nor)); memset(nor.data, 0xff, sizeof(nor.data));
    require(nor_configure(&nor, &cfg, block, count, prog), "geometry");
    memset(&fs, 0, sizeof(fs)); require(lfs_format(&fs, &cfg), "format"); reset_cpu();
    for (i = 0; i <= age; ++i) {
        make_snapshot(&old_value, i + 1u, size);
        require(fc_save(&store, policy, &old_value), "prepare history");
    }
    make_snapshot(&new_value, old_value.generation + 1u, size);
    require(lfs_unmount(&fs), "prepare unmount");
    memcpy(baseline, nor.data, block * count);
    reset_cpu(); nor_reset_stats(&nor);
    require(fc_save(&store, policy, &new_value), "reference update");
    total = nor.mutations; stats = nor.stats;
    if (!total || total > NOR_TRACE_CAP) fatal("mutation trace capacity", (int)total);
    memcpy(events, nor.trace, total * sizeof(events[0]));
    fprintf(costs, "%s,%u,%u,%u,%u,%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
            names[policy], block, count, prog, size, age,
            (unsigned long long)stats.read_calls, (unsigned long long)stats.read_bytes,
            (unsigned long long)stats.program_calls, (unsigned long long)stats.program_bytes,
            (unsigned long long)stats.erase_calls, (unsigned long long)stats.erase_bytes,
            (unsigned long long)stats.sync_calls);
    if (replay_mode) {
        if (replay_cut > total || (replay_cut && replay_prefix > events[replay_cut - 1].size))
            fatal("replay coordinates outside reference trace", -1);
        trial(policy, age, replay_cut, replay_prefix,
              replay_cut ? events[replay_cut - 1].kind : '-',
              replay_cut ? events[replay_cut - 1].size : 0);
        return;
    }
    trial(policy, age, 0, 0, '-', 0); /* acknowledgment durability control */
    for (op = 1; op <= total; ++op) {
        uint32_t n = events[op - 1].size;
        if (all_prefixes) {
            uint32_t prefix;
            for (prefix = 0; prefix <= n; ++prefix) trial(policy, age, op, prefix, events[op - 1].kind, n);
        } else {
            uint32_t prefixes[5] = {0, 1, n / 2, n - 1, n};
            unsigned j;
            for (j = 0; j < 5; ++j) {
                unsigned k; int duplicate = 0;
                for (k = 0; k < j; ++k) if (prefixes[k] == prefixes[j]) duplicate = 1;
                if (!duplicate) trial(policy, age, op, prefixes[j], events[op - 1].kind, n);
            }
        }
    }
}
int main(int argc, char **argv) {
    unsigned policy, geometry, payload, age;
    const uint32_t sizes[] = {16, 128, 768};
    const unsigned ages[] = {0, 24};
    int full = argc > 1 && strcmp(argv[1], "--full-prefix") == 0;
    const char *out = full ? "full_prefix.csv" : "campaign.csv";
    const char *cost = full ? "full_prefix_costs.csv" : "costs.csv";
    csv = fopen(out, "w"); costs = fopen(cost, "w");
    if (!csv || !costs) fatal("output file", -1);
    fputs("policy,block_size,block_count,payload_per_group,aged_updates,cut_operation,prefix_bytes,operation_kind,operation_size,acknowledged,outcome,generation0,generation1,generation2,recovery_read_calls,recovery_read_bytes,retry_ok\n", csv);
    fputs("policy,block_size,block_count,program_size,payload_per_group,aged_updates,read_calls,read_bytes,program_calls,program_bytes,erase_calls,erase_bytes,sync_calls\n", costs);
    replay_mode = argc > 1 && strcmp(argv[1], "--replay") == 0;
    if (replay_mode) {
        uint32_t fields[8];
        if (argc != 10) fatal("--replay policy block count prog payload age cut prefix", -1);
        for (policy = 0; policy < 8; ++policy) {
            char *end;
            unsigned long x = strtoul(argv[policy + 2], &end, 10);
            if (*end || !argv[policy + 2][0] || x > UINT32_MAX) fatal("invalid replay number", -1);
            fields[policy] = (uint32_t)x;
        }
        if (fields[0] > 2 || fields[5] > 1024) fatal("invalid replay policy/history", -1);
        replay_cut = fields[6]; replay_prefix = fields[7];
        scenario((fc_policy_t)fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], 0);
    } else for (policy = 0; policy < 3; ++policy) {
        if (full) scenario((fc_policy_t)policy, 512, 64, 16, 128, 24, 1);
        else for (geometry = 0; geometry < 2; ++geometry)
            for (payload = 0; payload < 3; ++payload)
                for (age = 0; age < 2; ++age)
                    scenario((fc_policy_t)policy, geometry ? 4096 : 512,
                             geometry ? 32 : 64, geometry ? 64 : 16, sizes[payload], ages[age], 0);
    }
    fclose(csv); fclose(costs);
    if (!replay_mode && !mixed) { fprintf(stderr, "Negative control did not expose mixed generations\n"); ++failures; }
    printf("{\"campaign\":\"%s\",\"rows\":%u,\"mixed_storage_negative_controls\":%u,\"failures\":%u}\n", full ? "full-prefix" : "sampled", rows, mixed, failures);
    return failures ? 1 : 0;
}
