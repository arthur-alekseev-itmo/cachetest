#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define LOW_RATIO 1.06
#define HIGH_RATIO 1.15
#define ITERATIONS 8192
#define TABLE_REFINEMENT_ATTEMPTS 60
#define SMALL_VALUE_FINDING_ATTEMPTS 100


int64_t nanosecond_counter(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

typedef struct {
    int iterations;
    int stride;
    int spots;
} config_t;

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
    int stride_in_cell = config->stride / sizeof(cell_t);
    assert(config->stride > stride_in_cell);
    size_t test_array_size = ((size_t)config->spots) * stride_in_cell + 4096;
    cell_t *arr = calloc(test_array_size, sizeof(cell_t));
    if (!arr) {
        perror("calloc");
        exit(1);
    }
    size_t misalignment = (size_t)arr % 4096;
 
    int n = config->spots - 1;
    long *temp_indices = malloc((n > 0 ? n : 1) * sizeof(long));
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


double experiment_run_with(int stride, int spots) {
    config_t config = {.iterations = ITERATIONS, .stride = stride, .spots = spots};
    return experiment_run(&config);
}


typedef struct {
    size_t spots_count;
    size_t strides_count;
    double *values;
} table_t;
 
#define VAL(t, stride_idx, spot) ((t)->values[(size_t)(stride_idx) * (t)->spots_count + (spot)])
 
void msmt_to_string(double ratio, char *buf, size_t n) {
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


void fmt_bytes(size_t size, char *buf, size_t n) {
    static const char *names[] = {"B", "KB", "MB", "GB", "TB"};
    int order = 0;
    while (size >= 1024) {
        order++;
        size /= 1024;
    }
    const char *name = order < 5 ? names[order] : "?";
    snprintf(buf, n, "%ld%s", size, name);
}


void print_data_table(const table_t *t, FILE *out) {
    fprintf(out, "          ");
    for (int i = 0; i < t->strides_count; i++) {
        char b[32];
        fmt_bytes(16L << i, b, sizeof b);
        fprintf(out, "%s%4s", i ? " . " : "", b);
    }
    fprintf(out, "\n");
    for (int spots = 0; spots < t->spots_count; spots++) {
        fprintf(out, "Spots: %03d", spots + 1);
        for (int s = 0; s < t->strides_count; s++) {
            char b[64];
            msmt_to_string(VAL(t, s, spots), b, sizeof b);
            fprintf(out, " %s |", b);
        }
        fprintf(out, "\n");
    }
}

double min(double a, double b) {
    if (a > b) return b;
    return a;
}

void refine_measurement_table(table_t* t, int spots_count, int strides_count, double small_time) {
    for (size_t stride_idx = 0; stride_idx < strides_count; stride_idx++) {
        for (size_t spots_idx = 0; spots_idx < spots_count; spots_idx++) {
            size_t stride = 16 << stride_idx;
            size_t spots = spots_idx + 1;
            double result = experiment_run_with(stride, spots);
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

double get_small_time() {
    double small_time = 1000.;
    for (int i = 0; i < SMALL_VALUE_FINDING_ATTEMPTS; i++) {
        small_time = min(small_time, experiment_run_with(16, 2));
        small_time = min(small_time, experiment_run_with(16, 3));
        small_time = min(small_time, experiment_run_with(32, 2));
        small_time = min(small_time, experiment_run_with(32, 3));
    }
    return small_time;
}

table_t create_measurement_table(int spots_count, int strides_count) {
    double small_time = get_small_time();

    table_t t = {
        .spots_count = spots_count, 
        .strides_count = strides_count,
        .values = calloc((size_t)spots_count * strides_count, sizeof(double))
    };
    
    if (!t.values) {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < TABLE_REFINEMENT_ATTEMPTS; i++) {
        printf("Refining measuring table %d/%d\n", i, TABLE_REFINEMENT_ATTEMPTS);
        refine_measurement_table(&t, spots_count, strides_count, small_time);
        print_data_table(&t, stdout);
    }
    
    return t;
}


typedef struct {
    int stride_idx;
    int spots;
} entity_location_t;
 
void print_entity_location(const entity_location_t *e, FILE *out) {
    char b[32];
    fmt_bytes(16L << e->stride_idx, b, sizeof b);
    fprintf(out, "Entity: {stride: %s; spots: %d}\n", b, e->spots);
}
 
int different(int prev, int curr) {
    return !(prev == curr || prev - 1 == curr);
}

entity_location_t *entity_detection_run(const table_t *table, int *count) {
    int *jumps = malloc(table->strides_count * sizeof(int));
    for (int s = 0; s < table->strides_count; s++) {
        jumps[s] = -1;
        for (int sp = 0; sp < table->spots_count; sp++) {
            if (VAL(table, s, sp) > HIGH_RATIO) {
                jumps[s] = sp;
                break;
            }
        }
    }
 
    entity_location_t *entities = malloc(table->strides_count * sizeof *entities);
    int n = 0;
    for (int s = 1; s <= table->strides_count - 2; s++) {
        int pj = jumps[s - 1], cj = jumps[s], nj = jumps[s + 1];
        if (pj >= 0 && cj >= 0 && nj >= 0 && different(pj, cj) && cj == nj) {
            entities[n].spots = cj;
            entities[n].stride_idx = s;
            n++;
        }
    }
    free(jumps);
    *count = n;
    return entities;
}

int main(void) {
    int spots_count = 32;
    int strides_count = 20;
 
    table_t measurements = create_measurement_table(spots_count, strides_count);
 
    FILE *file = fopen("msmt.table", "w");
    if (!file) {
        perror("fopen");
        return 1;
    }
    print_data_table(&measurements, file);
    fclose(file);
 
    int count;
    entity_location_t *entities = entity_detection_run(&measurements, &count);
    for (int i = count - 1; i >= 0; i--)
        print_entity_location(&entities[i], stdout);
 
    free(entities);
    free(measurements.values);
    return 0;
}


