#ifndef SENSOR_ACQUISITION_H
#define SENSOR_ACQUISITION_H

#include <stdbool.h>

#include "control_types.h"

typedef bool (*sensor_acquisition_read_fn)(
    sensor_data_t *sample,
    void *context);

typedef struct {
    sensor_acquisition_read_fn read;
    void *context;
} sensor_acquisition_t;

void sensor_acquisition_init(
    sensor_acquisition_t *acquisition);

void sensor_acquisition_set_source(
    sensor_acquisition_t *acquisition,
    sensor_acquisition_read_fn read,
    void *context);

sensor_data_t sensor_acquisition_read(
    const sensor_acquisition_t *acquisition);

#endif
