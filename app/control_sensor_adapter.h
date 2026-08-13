#ifndef CONTROL_SENSOR_ADAPTER_H
#define CONTROL_SENSOR_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#include "control_types.h"

typedef uint32_t (*control_sensor_now_us_fn)(void *context);

typedef struct {
    bool initialized;
    uint32_t encoder_counts_per_revolution;
    int32_t previous_raw_encoder_count;
    int64_t accumulated_encoder_counts;
    sensor_data_t latest_sample;
    control_sensor_now_us_fn now_us;
    void *time_context;
} control_sensor_adapter_t;

void control_sensor_adapter_init(
    control_sensor_adapter_t *adapter,
    int32_t initial_raw_encoder_count,
    uint32_t encoder_counts_per_revolution,
    control_sensor_now_us_fn now_us,
    void *time_context);

void control_sensor_adapter_update(
    control_sensor_adapter_t *adapter,
    uint32_t timestamp_us,
    uint16_t pendulum_adc_raw,
    int32_t raw_arm_encoder_count,
    uint16_t vbus_adc_raw,
    uint32_t vbus_millivolts,
    uint16_t pendulum_upright_adc,
    int8_t pendulum_direction);

bool control_sensor_adapter_read(
    sensor_data_t *sample,
    void *context);

#endif
