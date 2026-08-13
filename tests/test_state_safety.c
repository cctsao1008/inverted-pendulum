#include <assert.h>

#include "state_safety.h"

static state_safety_limits_t valid_limits(void)
{
    state_safety_limits_t limits;

    limits.configured = true;
    limits.max_sample_age_us = 2000U;
    limits.max_abs_pendulum_angle_rad = 0.5F;
    limits.max_abs_arm_angle_rad = 2.0F;
    limits.max_abs_pendulum_rate_rad_s = 20.0F;
    limits.max_abs_arm_rate_rad_s = 30.0F;
    return limits;
}

static sensor_data_t valid_sensor(void)
{
    sensor_data_t sensor = {0};

    sensor.timestamp_us = 1000U;
    sensor.sample_age_us = 0U;
    sensor.valid = true;
    return sensor;
}

static control_state_t valid_state(void)
{
    control_state_t state = {0};

    state.timestamp_us = 1000U;
    state.pendulum_angle_rad = 0.1F;
    state.pendulum_rate_rad_s = 1.0F;
    state.arm_angle_rad = 0.2F;
    state.arm_rate_rad_s = 2.0F;
    return state;
}

int main(void)
{
    state_safety_limits_t limits = valid_limits();
    sensor_data_t sensor = valid_sensor();
    control_state_t state = valid_state();
    state_safety_result_t result;

    result = state_safety_check(
        &state, &sensor, true, &limits);
    assert(result.fault_flags == STATE_SAFETY_FAULT_NONE);
    assert(result.control_allowed);
    assert(!result.hard_fault);

    limits.configured = false;
    result = state_safety_check(
        &state, &sensor, true, &limits);
    assert((result.fault_flags & STATE_SAFETY_FAULT_CONFIG) != 0U);
    assert(!result.control_allowed);
    assert(!result.hard_fault);

    limits = valid_limits();
    sensor.valid = false;
    result = state_safety_check(
        &state, &sensor, true, &limits);
    assert((result.fault_flags &
            STATE_SAFETY_FAULT_SENSOR_INVALID) != 0U);
    assert(result.hard_fault);

    sensor = valid_sensor();
    sensor.sample_age_us = 2001U;
    result = state_safety_check(
        &state, &sensor, true, &limits);
    assert((result.fault_flags &
            STATE_SAFETY_FAULT_SAMPLE_TIMEOUT) != 0U);
    assert(result.hard_fault);

    sensor = valid_sensor();
    state = valid_state();
    state.arm_angle_rad = 2.1F;
    result = state_safety_check(
        &state, &sensor, true, &limits);
    assert((result.fault_flags & STATE_SAFETY_FAULT_ARM_LIMIT) != 0U);
    assert(result.hard_fault);

    state = valid_state();
    result = state_safety_check(
        &state, &sensor, false, &limits);
    assert((result.fault_flags &
            STATE_SAFETY_FAULT_ESTIMATE_NOT_READY) != 0U);
    assert(!result.control_allowed);
    assert(!result.hard_fault);

    return 0;
}
