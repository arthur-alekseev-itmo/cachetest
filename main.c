#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "measurements.h"
#include "detection.h"
#include "disambiguation.h"


int main(void) {
    int spots_count = 32;
    int strides_count = 18;
 
    table_t measurements = create_measurement_table(spots_count, strides_count);
 
    size_t count;
    entity_location_t *entities = entity_detection_run(&measurements, &count);

    for (int i = count - 1; i >= 0; i--) {
        entity_location_t entity = entities[i];
        size_t stride = 16 << entities->stride_idx;
        size_t capacity = entity.spots * stride;
        size_t cache_line_size = detect_line_size(capacity, stride);
        printf("Entity characteristics:\n- capacity: %zu\n- associativiy: %zu\n- line size: %zu\n\n", capacity, entity.spots, cache_line_size);
    }

    free(entities);
    free(measurements.values);
    return 0;
}


