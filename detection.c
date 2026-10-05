#include "detection.h"
#include "experiment.h"
#include "utils.h"
#include "measurements.h"
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define INVALID_JUMP 65536
#define TOTAL_DETECTION_ATTEMPTS 4

void print_entity_location(const entity_location_t *e, FILE *out) {
    char b[32];
    fmt_bytes(16L << e->stride_idx, b, sizeof b);
    fprintf(out, "Entity: {stride: %s; spots: %zu}\n", b, e->spots);
}
 
bool different(int prev, int curr) {
    return !(prev == curr || prev - 1 == curr);
}

bool check_entity(entity_location_t entity) {
    print_entity_location(&entity, stdout);
    size_t spots = entity.spots;
    double small_time = get_small_time();
    size_t errors = 0;
    for (size_t stride_idx = entity.stride_idx; stride_idx > 1; stride_idx--) {
        size_t stride = 16 << stride_idx;
        double msmt_above = measure_min(stride, spots, 512) / small_time;
        double msmt_within = measure_min(stride, spots + 1, 512) / small_time;
        if (stride_idx != entity.stride_idx) {
            clear_lines(1);
        }
        printf("Moving to stride %zu and spots %zu, above: %f, within: %f\n", stride, spots, msmt_above, msmt_within);
        size_t entity_valid = msmt_above < msmt_within;
        errors += !entity_valid;
        if (errors > 100) {
            clear_lines(2);
            printf("Entity was fake\n\n");
            return false;
        }
        spots = spots << 1;
    }
    clear_lines(2);
    printf("Confirmed entity\n\n");
    return true;
}

size_t detect_all_entities(const table_t *table, entity_location_t *buffer) {
    printf("\nTrying to detect entities with data from table\n");
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
 
    size_t entity_id = 0;
    for (size_t stride_idx = 1; stride_idx <= table->strides_count - 2; stride_idx++) {
        size_t pj = jumps[stride_idx - 1], cj = jumps[stride_idx], nj = jumps[stride_idx + 1];
        if (pj != INVALID_JUMP && cj != INVALID_JUMP && nj != INVALID_JUMP && different(pj, cj) && cj == nj) {
            entity_location_t entity_to_check = {
                .spots = cj,
                .stride_idx = stride_idx,
            };
            if (!check_entity(entity_to_check)) {
                continue;
            }
            buffer[entity_id].spots = cj;
            buffer[entity_id].stride_idx = stride_idx;
            entity_id++;
        }
    }
    free(jumps);
    return entity_id;
}

entity_location_t entity_detection_run(const table_t* table) {
    entity_location_t entities[32];
    for (size_t attempt = 0; attempt < TOTAL_DETECTION_ATTEMPTS; attempt++) {
        size_t entity_count = detect_all_entities(table, entities);
        if (entity_count == 0) {
            printf("Warning, detected no entites, will retry again");
            continue;
        }
        return entities[0];
    }
    printf("We have not found a singular entity for total of %d attempts", TOTAL_DETECTION_ATTEMPTS);
    exit(1);
}
