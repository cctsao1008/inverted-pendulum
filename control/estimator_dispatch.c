#include "estimator_dispatch.h"

static void clear_state(
    control_state_t *state,
    uint32_t timestamp_us)
{
    state->pendulum_angle_rad = 0.0F;
    state->pendulum_rate_rad_s = 0.0F;
    state->arm_angle_rad = 0.0F;
    state->arm_rate_rad_s = 0.0F;
    state->timestamp_us = timestamp_us;
}

void estimator_dispatch_init(
    estimator_dispatch_t *dispatcher)
{
    if (dispatcher == 0) {
        return;
    }

    state_estimator_init(&dispatcher->basic);
    kalman_estimator_init(&dispatcher->kalman);
}

bool estimator_dispatch_step(
    estimator_dispatch_t *dispatcher,
    state_estimator_mode_t mode,
    const control_config_t *config,
    const sensor_data_t *sensor,
    control_state_t *state)
{
    state_estimator_input_t input;
    state_estimator_result_t result;

    if ((dispatcher == 0) ||
        (sensor == 0) ||
        (state == 0)) {
        return false;
    }

    clear_state(state, sensor->timestamp_us);

    if (!sensor->valid) {
        return false;
    }

    switch (mode) {
    case STATE_ESTIMATOR_BASIC:
        if (config == 0) {
            return false;
        }

        input.pendulum_angle_rad =
            sensor->pendulum_angle_rad;
        input.arm_angle_rad =
            sensor->arm_angle_rad;
        input.timestamp_us =
            sensor->timestamp_us;

        result = state_estimator_step(
            &dispatcher->basic,
            config,
            &input,
            state);

        return result == STATE_ESTIMATOR_RESULT_OK;

    case STATE_ESTIMATOR_KALMAN:
        return kalman_estimator_step(
            &dispatcher->kalman,
            sensor,
            state);

    default:
        return false;
    }
}
