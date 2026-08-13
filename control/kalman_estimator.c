#include "kalman_estimator.h"

void kalman_estimator_init(
    kalman_estimator_t *estimator)
{
    if (estimator == 0) {
        return;
    }

    estimator->initialized = false;
}

bool kalman_estimator_step(
    kalman_estimator_t *estimator,
    const sensor_data_t *sensor,
    control_state_t *state)
{
    if ((estimator == 0) ||
        (sensor == 0) ||
        (state == 0)) {
        return false;
    }

    state->pendulum_angle_rad = 0.0F;
    state->pendulum_rate_rad_s = 0.0F;
    state->arm_angle_rad = 0.0F;
    state->arm_rate_rad_s = 0.0F;
    state->timestamp_us = sensor->timestamp_us;

    estimator->initialized = false;

    return false;
}
