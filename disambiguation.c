#include "experiment.h"
#include "measurements.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>

#define ATTEMPT_COUNT 4

typedef enum {
    Decrease,
    Increase,
    Flat
} trend_t;

double measure_min(size_t stride, size_t spots) {
    double min_measure = 100000;
    size_t iterations = ITERATIONS / 10;
    for (size_t i = 0; i < ATTEMPT_COUNT; i++) {
        min_measure = min(min_measure, experiment_run_with(stride, spots, iterations));
    }
    return min_measure;
}

size_t jump_for_stride(size_t stride, size_t hi) {
    double time = measure_min(stride, 2);
    double limit = time * HIGH_RATIO;
    
    if (measure_min(stride, hi) <= limit)
        return 0;
    
    size_t lo = 2;
    while (hi - lo > 1) {
        size_t mid = lo + (hi - lo) / 2;
        if (measure_min(stride, mid) > limit) hi = mid;
        else lo = mid;
    }
    return hi;
}

trend_t trend_for(size_t stride, size_t capacity) {
    size_t hi = capacity / 8; 
    size_t jump_no_addition = jump_for_stride(stride, hi);

    if (jump_no_addition == 0)
        return Flat;

    int decrease_count = 0, increase_count = 0;

    size_t addition = stride / 2;
    if (addition < 8 || addition % 8 != 0)
        return Flat;
    
    size_t jump_addition = jump_for_stride(stride + addition, hi);
    if (jump_addition == 0)
        return Flat;
    
    double ratio = (double)jump_addition / (double)jump_no_addition;
    if (ratio < 2. - LOW_RATIO) decrease_count++;
    else if (ratio > LOW_RATIO) increase_count++;
    
    printf("Stride=%zu Addition=%zu: jump at %zu -> %zu spots (ratio %.2f)\n", stride, addition, jump_no_addition, jump_addition, ratio);
    if (decrease_count > increase_count) return Decrease;
    if (increase_count > decrease_count) return Increase;
    return Flat;
}

size_t detect_line_size(size_t capacity, size_t max_stride) {
    int seen_decrease = 0;
    for (size_t stride = 16; stride <= max_stride; stride *= 2) {
        trend_t t = trend_for(stride, capacity);
        printf("Stride=%zu: %s\n", stride, t == Decrease ? "Decrease" : t == Increase ? "Increase" : "Flat");
        printf("\033[A\033[2K");
        if (t == Decrease) seen_decrease = 1;
        if (t == Increase) {
            return seen_decrease ? stride / 2 : 0;
        }
    }
    return 0;
}
