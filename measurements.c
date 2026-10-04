#include "measurements.h"
#include "experiment.h"
#include "utils.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
 
void msmt_to_string(double ratio, char *buf, size_t n) {
    // For prettier printing results
    ratio = ratio > 9.99 ? 9.99 : ratio;

    if (ratio < LOW_RATIO) {
        snprintf(buf, n, "%4.2f", ratio);
        return;
    }
    
    if (ratio > HIGH_RATIO) {
        snprintf(buf, n, "\x1b[0;34m%4.2f\x1b[0;0m", ratio);
        return;
    }
    
    snprintf(buf, n, "\x1b[0;36m%4.2f\x1b[0;0m", ratio);
}


void print_data_table(const table_t *t, FILE *out) {
    fprintf(out, "          ");
    for (size_t i = 0; i < t->strides_count; i++) {
        char b[32];
        fmt_bytes(16L << i, b, sizeof b);
        fprintf(out, "%s%4s", i ? " . " : "", b);
    }
    fprintf(out, "\n");
    for (size_t spots = 0; spots < t->spots_count; spots++) {
        fprintf(out, "Spots: %03zu", spots + 1);
        for (size_t s = 0; s < t->strides_count; s++) {
            char b[64];
            msmt_to_string(VAL(t, s, spots), b, sizeof b);
            fprintf(out, " %s |", b);
        }
        fprintf(out, "\n");
    }
}

void refine_measurement_table(table_t* t, size_t spots_count, size_t strides_count, double small_time) {
    for (size_t stride_idx = 0; stride_idx < strides_count; stride_idx++) {
        for (size_t spots_idx = 0; spots_idx < spots_count; spots_idx++) {
            size_t stride = 16 << stride_idx;
            size_t spots = spots_idx + 1;
            double result = experiment_run_with(stride, spots, ITERATIONS / 2);
            double x = result / small_time;
            double current_val = VAL(t, stride_idx, spots_idx);
            if (current_val <= 0.01f) {
                VAL(t, stride_idx, spots_idx) = x;
                continue;
            }
            VAL(t, stride_idx, spots_idx) = min(x, current_val);
        }
    }
}

table_t create_measurement_table(int spots_count, int strides_count) {
    double small_time = get_small_time();
    bool print_table = true;

    table_t t = {
        .spots_count = spots_count, 
        .strides_count = strides_count,
        .values = calloc((size_t)spots_count * strides_count, sizeof(double))
    };
    
    if (!t.values) {
        perror("malloc");
        exit(1);
    }

    for (size_t i = 0; i < TABLE_REFINEMENT_ATTEMPTS; i++) {
        refine_measurement_table(&t, spots_count, strides_count, small_time);
        if (i != 0) {
            size_t line_clear_count = print_table ? spots_count + 2 : 1;
            clear_lines(line_clear_count);
        }
        printf("Refining measuring table %zu/%d\n", i + 1, TABLE_REFINEMENT_ATTEMPTS);
        if (print_table) {
            print_data_table(&t, stdout);
            fflush(stdout);
        }
    }
    
    return t;
}

