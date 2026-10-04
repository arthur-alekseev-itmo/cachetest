#include "disambiguation.h"
#include "experiment.h"
#include "measurements.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define MEASUREMENT_COUNT 4

typedef enum {
    Decrease,
    Increase,
    Flat
} trend_t;

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

    size_t addition = stride / 2;
    if (addition < 8 || addition % 8 != 0)
        return Flat;
    
    size_t jump_addition = jump_for_stride(stride + addition, hi);
    if (jump_addition == 0)
        return Flat;
    
    double ratio = (double)jump_addition / (double)jump_no_addition;
    if (ratio < 2. - LOW_RATIO) return Decrease; 
    else if (ratio > LOW_RATIO) return Increase; 
    return Flat;
}

size_t detect_line_size_once(size_t capacity, size_t max_stride) {
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

int compare(const void *a, const void *b) {
    const size_t arg1 = *(const size_t*)a;
    const size_t arg2 = *(const size_t*)b;
    
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}


size_t detect_line_size(size_t capacity, size_t max_stride) {
    size_t measurements[MEASUREMENT_COUNT];
    for (size_t i = 0; i < MEASUREMENT_COUNT; i++) {
        printf("Detecting line size, attempt %zu/%d", i, MEASUREMENT_COUNT);
        measurements[i] = detect_line_size_once(capacity, max_stride);
        printf("Got size: %zu\n", measurements[i]);
    }
    qsort(measurements, sizeof(measurements) / sizeof(measurements[0]), sizeof(measurements[0]), compare);
    return measurements[MEASUREMENT_COUNT / 2];
}

