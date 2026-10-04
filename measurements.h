#pragma once

#define LOW_RATIO 1.06
#define HIGH_RATIO 1.15
#define TABLE_REFINEMENT_ATTEMPTS 8

#include <stdlib.h>
#include <stdio.h>

typedef struct {
    size_t spots_count;
    size_t strides_count;
    double *values;
} table_t;

table_t create_measurement_table(int spots_count, int strides_count);

void print_data_table(const table_t *t, FILE *out);

#define VAL(t, stride_idx, spot) ((t)->values[(size_t)(stride_idx) * (t)->spots_count + (spot)])


