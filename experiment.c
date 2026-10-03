#include "experiment.h"

#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define ASSERT_ALLOCATED(x) if (!(x)) { perror("Failed allocation"); exit(1); }

int64_t nanosecond_counter(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

typedef int64_t cell_t;
 
void shuffle(long *a, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        long t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

typedef struct {
    size_t first_index;
    cell_t* array;
} test_array_t;

bool test_array_integrity(test_array_t array, size_t expected_iterations) {
    size_t current_index = array.array[array.first_index];
    size_t current_iter = 1;
    while (current_index != array.first_index) {
        current_iter++;
        current_index = array.array[current_index];
    }
    return expected_iterations == current_iter;
}
 
test_array_t create_test_array(const config_t *config) {
    size_t stride_in_cell = config->stride / sizeof(cell_t);
    assert(config->stride > stride_in_cell);
    size_t test_array_size = ((size_t)config->spots) * stride_in_cell + 4096 + config->offset;
    cell_t *arr = calloc(test_array_size, sizeof(cell_t));
    ASSERT_ALLOCATED(arr);
    
    size_t misalignment = (size_t)arr % 4096 + config->offset;
 
    int n = config->spots - 1;
    long *temp_indices = malloc((n > 0 ? n : 1) * sizeof(long));
    ASSERT_ALLOCATED(temp_indices);
    for (int i = 0; i < n; i++)
        temp_indices[i] = misalignment + (size_t)(i + 1) * stride_in_cell;
    shuffle(temp_indices, n);
 
    long current = misalignment;
    for (int i = 0; i < n; i++) {
        arr[current] = temp_indices[i];
        current = temp_indices[i];
    }
    arr[current] = misalignment;

    free(temp_indices);
    test_array_t result = { .first_index = misalignment, .array = arr };
    assert(test_array_integrity(result, config->spots));
    return result;
}

volatile cell_t trashcan;

void run_through_array(volatile test_array_t array, size_t end_iteration) {
    size_t idx = array.first_index;
    cell_t *data = array.array;
    for (size_t iteration = 0; iteration != end_iteration; iteration++) {
        cell_t new_index = data[idx];
        idx = new_index;
    }
    trashcan = idx;
}


double experiment_run(const config_t *config) {
    test_array_t array = create_test_array(config);
    size_t end_iteration = (long)config->spots * config->iterations;
 
    run_through_array(array, config->spots * 8); /* Warmup */

    int64_t time_start = nanosecond_counter();
    run_through_array(array, end_iteration);
    int64_t time_end = nanosecond_counter();
 
    double time_diff = (double)(time_end - time_start);
    double avg_time = time_diff / config->spots / config->iterations;

    free(array.array);
    return avg_time;
}


double experiment_run_with(size_t stride, size_t spots, size_t iterations) {
    size_t offset = rand() % 100 * 4096;
    config_t config = {
        .iterations = iterations, 
        .stride = stride, 
        .spots = spots, 
        .offset = offset
    };
    return experiment_run(&config);
}



