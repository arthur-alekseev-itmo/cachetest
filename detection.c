#include "detection.h"
#include "experiment.h"
#include "utils.h"
#include "measurements.h"
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define INVALID_JUMP 65536

void print_entity_location(const entity_location_t *e, FILE *out) {
    char b[32];
    fmt_bytes(16L << e->stride_idx, b, sizeof b);
    fprintf(out, "Entity: {stride: %s; spots: %zu}\n", b, e->spots);
}
 
bool different(int prev, int curr) {
    return !(prev == curr || prev - 1 == curr);
}

bool check_entity(size_t entity_spots, size_t entity_stride_idx) {
    printf("Checking entity is real\n");
    size_t spots = entity_spots;
    for (size_t stride_idx = entity_stride_idx; stride_idx > 1; stride_idx--) {
        size_t stride = 16 << stride_idx;
        double msmt_above = measure_min(stride, spots + 1);
        double msmt_within = measure_min(stride, spots);
        printf("Moving to stride %zu and spots %zu, above: %f, within: %f\n", stride, spots, msmt_above, msmt_within);
        if (msmt_within / msmt_above > LOW_RATIO) {
            return false;
        }
        spots = spots << 1;
    }
    return true;
}

entity_location_t *entity_detection_run(const table_t *table, size_t *count) {
    size_t *jumps = malloc(table->strides_count * sizeof(size_t));
    for (size_t s = 0; s < table->strides_count; s++) {
        jumps[s] = INVALID_JUMP;
        for (size_t sp = 0; sp < table->spots_count; sp++) {
            if (VAL(table, s, sp) > HIGH_RATIO) {
                jumps[s] = sp;
                break;
            }
        }
    }
 
    entity_location_t *entities = malloc(table->strides_count * sizeof(entity_location_t));
    size_t entity_id = 0;
    for (size_t stride_idx = 1; stride_idx <= table->strides_count - 2; stride_idx++) {
        size_t pj = jumps[stride_idx - 1], cj = jumps[stride_idx], nj = jumps[stride_idx + 1];
        if (pj != INVALID_JUMP && cj != INVALID_JUMP && nj != INVALID_JUMP && different(pj, cj) && cj == nj) {
            if (!check_entity(cj, stride_idx)) {
                continue;
            }
            entities[entity_id].spots = cj;
            entities[entity_id].stride_idx = stride_idx;
            entity_id++;
        }
    }
    free(jumps);
    *count = entity_id;
    return entities;
}

