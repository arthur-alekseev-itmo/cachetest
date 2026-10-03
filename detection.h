#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "measurements.h"

typedef struct {
    int stride_idx;
    int spots;
} entity_location_t;
 
void print_entity_location(const entity_location_t *e, FILE *out);

entity_location_t *entity_detection_run(const table_t *table, size_t *count);
