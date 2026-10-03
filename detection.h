#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "measurements.h"

typedef struct {
    size_t stride_idx;
    size_t spots;
} entity_location_t;
 
void print_entity_location(const entity_location_t *e, FILE *out);

entity_location_t *entity_detection_run(const table_t *table, size_t *count);
