#include "state_safety.h"

#include <float.h>

static bool value_is_finite(float value)
{
    return (value == value) &&
           (value <= FLT_MAX) &&
           (value >= -FLT_MAX);
}

static float absolute_value(float value)
{
    return (value < 0.0F) ? -value : value;
}

static bool limits_are_valid(
    const state_safety_limits_t *limits)
{
    if ((limits == 0) ||
        !limits->configured) {
        return false;
    }

    if ((limits->max_abs_pendulum_angle_rad <= 0.0F) ||
        (limits->max_abs_arm_angle_rad <= 0.0F) ||
        (limits->max_abs_pendulum_rate_rad_s <= 0.0F) ||
        (limits->max_abs_arm_rate_rad_s <= 0.0F)) {
        return false;
    }

    return value_is_finite(
               limits->max_abs_pendulum_angle_rad) &&
           value_is_finite(
               limits->max_abs_arm_angle_rad) &&
           value_is_finite(
               limits->max_abs_pendulum_rate_rad_s) &&
           value_is_finite(
               limits->max_abs_arm_rate_rad_s);
}

void state_safety_limits_init_unconfigured(
    state_safety_limits_t *limits)
{
    if (limits == 0) {
        return;
    }

    limits->configured = false;
    limits->max_sample_age_us = 0U;
    limits->max_abs_pendulum_angle_rad = 0.0F;
    limits->max_abs_arm_angle_rad = 0.0F;
    limits->max_abs_pendulum_rate_rad_s = 0.0F;
    limits->max_abs_arm_rate_rad_s = 0.0F;
}

state_safety_result_t state_safety_check(
    const control_state_t *state,
    const sensor_data_t *sensor,
    bool estimate_ready,
    const state_safety_limits_t *limits)
{
    state_safety_result_t result;

    result.fault_flags = STATE_SAFETY_FAULT_NONE;
    result.control_allowed = false;
    result.hard_fault = false;

    if ((state == 0) ||
        (sensor == 0) ||
        (limits == 0)) {
        result.fault_flags =
            STATE_SAFETY_FAULT_ARGUMENT;
        result.hard_fault = true;
        return result;
    }

    if (!limits_are_valid(limits)) {
        result.fault_flags |=
            STATE_SAFETY_FAULT_CONFIG;
    }

    if (!sensor->valid) {
        result.fault_flags |=
            STATE_SAFETY_FAULT_SENSOR_INVALID;
        result.hard_fault = true;
    }

    if (!estimate_ready) {
        result.fault_flags |=
            STATE_SAFETY_FAULT_ESTIMATE_NOT_READY;
    }

    if (!value_is_finite(state->pendulum_angle_rad) ||
        !value_is_finite(state->pendulum_rate_rad_s) ||
        !value_is_finite(state->arm_angle_rad) ||
        !value_is_finite(state->arm_rate_rad_s)) {
        result.fault_flags |=
            STATE_SAFETY_FAULT_STATE_NONFINITE;
        result.hard_fault = true;
    }

    if (estimate_ready &&
        (state->timestamp_us != sensor->timestamp_us)) {
        result.fault_flags |=
            STATE_SAFETY_FAULT_TIMESTAMP;
        result.hard_fault = true;
    }

    if ((limits->max_sample_age_us > 0U) &&
        (sensor->sample_age_us >
         limits->max_sample_age_us)) {
        result.fault_flags |=
            STATE_SAFETY_FAULT_SAMPLE_TIMEOUT;
        result.hard_fault = true;
    }

    if (limits_are_valid(limits)) {
        if (absolute_value(
                state->pendulum_angle_rad) >
            limits->max_abs_pendulum_angle_rad) {
            result.fault_flags |=
                STATE_SAFETY_FAULT_PENDULUM_LIMIT;
            result.hard_fault = true;
        }

        if (absolute_value(
                state->arm_angle_rad) >
            limits->max_abs_arm_angle_rad) {
            result.fault_flags |=
                STATE_SAFETY_FAULT_ARM_LIMIT;
            result.hard_fault = true;
        }

        if (absolute_value(
                state->pendulum_rate_rad_s) >
            limits->max_abs_pendulum_rate_rad_s) {
            result.fault_flags |=
                STATE_SAFETY_FAULT_PENDULUM_RATE_LIMIT;
            result.hard_fault = true;
        }

        if (absolute_value(
                state->arm_rate_rad_s) >
            limits->max_abs_arm_rate_rad_s) {
            result.fault_flags |=
                STATE_SAFETY_FAULT_ARM_RATE_LIMIT;
            result.hard_fault = true;
        }
    }

    result.control_allowed =
        result.fault_flags == STATE_SAFETY_FAULT_NONE;

    return result;
}
