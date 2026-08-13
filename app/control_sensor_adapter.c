#include "control_sensor_adapter.h"

#include <stddef.h>

#include "pendulum_angle.h"

#define CONTROL_SENSOR_TWO_PI_F 6.28318530717958647692F

static int32_t encoder_delta_16(
    int32_t current,
    int32_t previous)
{
    return (int32_t)(int16_t)(
        (uint16_t)current - (uint16_t)previous);
}

void control_sensor_adapter_init(
    control_sensor_adapter_t *adapter,
    int32_t initial_raw_encoder_count,
    uint32_t encoder_counts_per_revolution,
    control_sensor_now_us_fn now_us,
    void *time_context)
{
    if (adapter == NULL) {
        return;
    }

    adapter->initialized = true;
    adapter->encoder_counts_per_revolution =
        encoder_counts_per_revolution;
    adapter->previous_raw_encoder_count =
        initial_raw_encoder_count;
    adapter->accumulated_encoder_counts = 0;
    adapter->latest_sample = (sensor_data_t){0};
    adapter->latest_sample.valid = false;
    adapter->now_us = now_us;
    adapter->time_context = time_context;
}

void control_sensor_adapter_update(
    control_sensor_adapter_t *adapter,
    uint32_t timestamp_us,
    uint16_t pendulum_adc_raw,
    int32_t raw_arm_encoder_count,
    uint16_t vbus_adc_raw,
    uint32_t vbus_millivolts,
    uint16_t pendulum_upright_adc,
    int8_t pendulum_direction)
{
    int32_t encoder_delta;

    if ((adapter == NULL) ||
        !adapter->initialized) {
        return;
    }

    encoder_delta = encoder_delta_16(
        raw_arm_encoder_count,
        adapter->previous_raw_encoder_count);

    adapter->previous_raw_encoder_count =
        raw_arm_encoder_count;
    adapter->accumulated_encoder_counts +=
        (int64_t)encoder_delta;

    adapter->latest_sample.timestamp_us = timestamp_us;
    adapter->latest_sample.sample_age_us = 0U;
    adapter->latest_sample.pendulum_adc_raw =
        pendulum_adc_raw;
    adapter->latest_sample.arm_encoder_count =
        raw_arm_encoder_count;
    adapter->latest_sample.vbus_adc_raw = vbus_adc_raw;
    adapter->latest_sample.vbus_millivolts =
        vbus_millivolts;

    adapter->latest_sample.pendulum_angle_rad =
        pendulum_angle_radians(
            pendulum_adc_raw,
            pendulum_upright_adc,
            pendulum_direction);

    if (adapter->encoder_counts_per_revolution > 0U) {
        adapter->latest_sample.arm_angle_rad =
            (float)adapter->accumulated_encoder_counts *
            (CONTROL_SENSOR_TWO_PI_F /
             (float)adapter->encoder_counts_per_revolution);
    } else {
        adapter->latest_sample.arm_angle_rad = 0.0F;
    }

    adapter->latest_sample.valid =
        (adapter->encoder_counts_per_revolution > 0U) &&
        (adapter->now_us != NULL) &&
        (pendulum_adc_raw <
         PENDULUM_ADC_COUNTS_PER_REVOLUTION) &&
        (pendulum_upright_adc <
         PENDULUM_ADC_COUNTS_PER_REVOLUTION) &&
        ((pendulum_direction == 1) ||
         (pendulum_direction == -1));
}

bool control_sensor_adapter_read(
    sensor_data_t *sample,
    void *context)
{
    control_sensor_adapter_t *adapter =
        (control_sensor_adapter_t *)context;

    if ((sample == NULL) ||
        (adapter == NULL) ||
        !adapter->initialized ||
        !adapter->latest_sample.valid ||
        (adapter->now_us == NULL)) {
        return false;
    }

    *sample = adapter->latest_sample;
    sample->sample_age_us =
        adapter->now_us(adapter->time_context) -
        sample->timestamp_us;

    return true;
}
