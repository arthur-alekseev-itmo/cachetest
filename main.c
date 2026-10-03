#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "measurements.h"
#include "detection.h"


int main(void) {
    int spots_count = 32;
    int strides_count = 20;
 
    table_t measurements = create_measurement_table(spots_count, strides_count);
 
    size_t count;
    entity_location_t *entities = entity_detection_run(&measurements, &count);
    for (int i = count - 1; i >= 0; i--)
        print_entity_location(&entities[i], stdout);
 
    free(entities);
    free(measurements.values);
    return 0;
}


