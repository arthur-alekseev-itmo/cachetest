#pragma once

#define ATTEMPT_COUNT 8
#define ITERATIONS 8192

#include <stdint.h>
#include <stddef.h>

typedef struct {
    size_t offset;
    size_t iterations;
    size_t stride;
    size_t spots;
} config_t;

double experiment_run(const config_t *config);

double experiment_run_with(size_t stride, size_t spots, size_t iterations);

double measure_min(size_t stride, size_t spots);
