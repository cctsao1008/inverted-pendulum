#include "balance_controller.h"
#include "control_math.h"

#include <float.h>
#include <stddef.h>

static bool controller_value_is_finite(float value)
{
    return (value == value) &&
           (value <= FLT_MAX) &&
           (value >= -FLT_MAX);
}

static bool controller_state_is_valid(
    const control_state_t *state)
{
    return controller_value_is_finite(
               state->pendulum_angle_rad) &&
           controller_value_is_finite(
               state->pendulum_rate_rad_s) &&
           controller_value_is_finite(
               state->arm_angle_rad) &&
           controller_value_is_finite(
               state->arm_rate_rad_s);
}

balance_controller_result_t balance_controller_compute(
    const control_config_t *config,
    const control_state_t *state,
    float *normalized_output)
{
    float state_vector[4];
    float feedback;
    float command;

    if (normalized_output == NULL) {
        return BALANCE_CONTROLLER_ERROR_ARGUMENT;
    }

    /*
     * Fail closed before examining any other argument.
     */
    *normalized_output = 0.0F;

    if ((config == NULL) || (state == NULL)) {
        return BALANCE_CONTROLLER_ERROR_ARGUMENT;
    }

    if (control_config_validate_balance(config) !=
        CONTROL_CONFIG_OK) {
        return BALANCE_CONTROLLER_ERROR_CONFIG;
    }

    if (!controller_state_is_valid(state)) {
        return BALANCE_CONTROLLER_ERROR_STATE;
    }

    state_vector[0] = state->pendulum_angle_rad;
    state_vector[1] = state->pendulum_rate_rad_s;
    state_vector[2] = state->arm_angle_rad;
    state_vector[3] = state->arm_rate_rad_s;

    feedback = control_math_dot_f32(
        config->balance_gain,
        state_vector,
        4U);

    if (!controller_value_is_finite(feedback)) {
        return BALANCE_CONTROLLER_ERROR_NUMERIC;
    }

    command = -feedback;

    if (!controller_value_is_finite(command)) {
        return BALANCE_CONTROLLER_ERROR_NUMERIC;
    }

    if (command > config->max_normalized_output) {
        command = config->max_normalized_output;
    } else if (
        command < -config->max_normalized_output) {
        command = -config->max_normalized_output;
    }

    *normalized_output = command;

    return BALANCE_CONTROLLER_RESULT_OK;
}
