#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "control_sensor_adapter.h"

static uint32_t fake_now_us(void *context)
{
    return *(uint32_t *)context;
}

int main(void)
{
    control_sensor_adapter_t adapter;
    sensor_data_t sample;
    uint32_t now_us = 2025U;
    const float expected_arm_angle =
        16.0F * (6.28318530717958647692F / 1040.0F);

    control_sensor_adapter_init(
        &adapter,
        32760,
        1040U,
        fake_now_us,
        &now_us);

    assert(!control_sensor_adapter_read(&sample, &adapter));

    control_sensor_adapter_update(
        &adapter,
        2000U,
        2928U,
        -32760,
        1000U,
        8865U,
        2928U,
        1);

    assert(control_sensor_adapter_read(&sample, &adapter));
    assert(sample.valid);
    assert(sample.sample_age_us == 25U);
    assert(sample.arm_encoder_count == -32760);
    assert(fabsf(sample.pendulum_angle_rad) < 0.000001F);
    assert(fabsf(sample.arm_angle_rad - expected_arm_angle) < 0.00001F);

    now_us = 3020U;
    control_sensor_adapter_update(
        &adapter,
        3000U,
        2928U,
        -32750,
        1001U,
        8874U,
        2928U,
        1);

    assert(control_sensor_adapter_read(&sample, &adapter));
    assert(sample.sample_age_us == 20U);
    assert(sample.arm_angle_rad > expected_arm_angle);

    control_sensor_adapter_update(
        &adapter,
        4000U,
        2928U,
        -32740,
        1001U,
        8874U,
        2928U,
        0);
    assert(!control_sensor_adapter_read(&sample, &adapter));

    return 0;
}
