#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "measurements.h"
#include "detection.h"
#include "disambiguation.h"
#include "utils.h"


int main(void) {
    int spots_count = 33;
    int strides_count = 18;
 
    table_t measurements = create_measurement_table(spots_count, strides_count);
 
    entity_location_t entity = entity_detection_run(&measurements);

    size_t stride = 16 << entity.stride_idx;
    size_t capacity = entity.spots * stride;
    size_t cache_line_size = detect_line_size(capacity, stride);
    char capacity_s[32];
    fmt_bytes(capacity, capacity_s, sizeof(capacity_s));
    printf("\nEntity characteristics:\n- capacity: %s\n- associativity: %zu\n- line size: %zu\n\n", capacity_s, entity.spots, cache_line_size);

    free(measurements.values);
    return 0;
}


