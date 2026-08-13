#include "sensor_acquisition.h"

static sensor_data_t sensor_data_invalid(void)
{
    sensor_data_t sample = {0};

    sample.valid = false;
    return sample;
}

void sensor_acquisition_init(
    sensor_acquisition_t *acquisition)
{
    if (acquisition == 0) {
        return;
    }

    acquisition->read = 0;
    acquisition->context = 0;
}

void sensor_acquisition_set_source(
    sensor_acquisition_t *acquisition,
    sensor_acquisition_read_fn read,
    void *context)
{
    if (acquisition == 0) {
        return;
    }

    acquisition->read = read;
    acquisition->context = context;
}

sensor_data_t sensor_acquisition_read(
    const sensor_acquisition_t *acquisition)
{
    sensor_data_t sample = sensor_data_invalid();

    if ((acquisition == 0) ||
        (acquisition->read == 0)) {
        return sample;
    }

    if (!acquisition->read(
            &sample,
            acquisition->context)) {
        sample.valid = false;
    }

    return sample;
}
