#include "detection.h"
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
 
    entity_location_t *entities = malloc(table->strides_count * sizeof *entities);
    size_t n = 0;
    for (size_t s = 1; s <= table->strides_count - 2; s++) {
        size_t pj = jumps[s - 1], cj = jumps[s], nj = jumps[s + 1];
        if (pj != INVALID_JUMP && cj != INVALID_JUMP && nj != INVALID_JUMP && different(pj, cj) && cj == nj) {
            entities[n].spots = cj;
            entities[n].stride_idx = s;
            n++;
        }
    }
    free(jumps);
    *count = n;
    return entities;
}

