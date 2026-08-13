#include "output_safety.h"

#include <float.h>

static bool value_is_finite(float value)
{
    return (value == value) &&
           (value <= FLT_MAX) &&
           (value >= -FLT_MAX);
}

static bool limits_are_valid(
    const output_safety_limits_t *limits)
{
    if ((limits == 0) ||
        !limits->configured ||
        !value_is_finite(limits->max_pwm_percent)) {
        return false;
    }

    return (limits->max_pwm_percent > 0.0F) &&
           (limits->max_pwm_percent <= 100.0F);
}

void output_safety_limits_init_unconfigured(
    output_safety_limits_t *limits)
{
    if (limits == 0) {
        return;
    }

    limits->configured = false;
    limits->max_pwm_percent = 0.0F;
}

actuator_command_t output_safety_apply(
    const actuator_command_t *requested,
    const state_safety_result_t *state_safety,
    const output_safety_limits_t *limits)
{
    actuator_command_t output =
        actuator_command_safe_off();

    if ((requested == 0) ||
        (state_safety == 0) ||
        !requested->valid ||
        !state_safety->control_allowed ||
        !limits_are_valid(limits) ||
        !value_is_finite(requested->pwm_percent) ||
        (requested->pwm_percent < 0.0F)) {
        return output;
    }

    output = *requested;

    if (output.direction == MOTOR_DIRECTION_NONE) {
        output.pwm_percent = 0.0F;
        output.brake = false;
        output.valid = true;
        return output;
    }

    if (output.pwm_percent >
        limits->max_pwm_percent) {
        output.pwm_percent =
            limits->max_pwm_percent;
    }

    output.valid = true;
    return output;
}
