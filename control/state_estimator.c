#include "state_estimator.h"

#include <float.h>
#include <stddef.h>

#define STATE_ESTIMATOR_PI_F \
    3.14159265358979323846F

#define STATE_ESTIMATOR_TWO_PI_F \
    6.28318530717958647692F

#define STATE_ESTIMATOR_MIN_PERIOD_RATIO 0.5F
#define STATE_ESTIMATOR_MAX_PERIOD_RATIO 1.5F

static bool estimator_value_is_finite(float value)
{
    return (value == value) &&
           (value <= FLT_MAX) &&
           (value >= -FLT_MAX);
}

static bool estimator_pendulum_angle_is_valid(float angle_rad)
{
    return estimator_value_is_finite(angle_rad) &&
           (angle_rad >= -STATE_ESTIMATOR_PI_F) &&
           (angle_rad <= STATE_ESTIMATOR_PI_F);
}

static float estimator_wrapped_delta(
    float current_angle_rad,
    float previous_angle_rad)
{
    float delta =
        current_angle_rad - previous_angle_rad;

    if (delta > STATE_ESTIMATOR_PI_F) {
        delta -= STATE_ESTIMATOR_TWO_PI_F;
    } else if (delta < -STATE_ESTIMATOR_PI_F) {
        delta += STATE_ESTIMATOR_TWO_PI_F;
    }

    return delta;
}

static float estimator_filter_rate(
    float previous_rate,
    float raw_rate,
    float alpha)
{
    return previous_rate +
           alpha * (raw_rate - previous_rate);
}

static void estimator_write_state(
    const state_estimator_t *estimator,
    const state_estimator_input_t *input,
    control_state_t *state)
{
    state->pendulum_angle_rad =
        input->pendulum_angle_rad;
    state->pendulum_rate_rad_s =
        estimator->pendulum_rate_rad_s;

    state->arm_angle_rad =
        input->arm_angle_rad;
    state->arm_rate_rad_s =
        estimator->arm_rate_rad_s;

    state->timestamp_us = input->timestamp_us;
}

static void estimator_seed_from_sample(
    state_estimator_t *estimator,
    const state_estimator_input_t *input,
    control_state_t *state)
{
    estimator->initialized = true;
    estimator->previous_pendulum_angle_rad =
        input->pendulum_angle_rad;
    estimator->previous_arm_angle_rad =
        input->arm_angle_rad;
    estimator->pendulum_rate_rad_s = 0.0F;
    estimator->arm_rate_rad_s = 0.0F;
    estimator->previous_timestamp_us =
        input->timestamp_us;

    estimator_write_state(estimator, input, state);
}

void state_estimator_init(state_estimator_t *estimator)
{
    if (estimator == NULL) {
        return;
    }

    estimator->initialized = false;

    estimator->previous_pendulum_angle_rad = 0.0F;
    estimator->previous_arm_angle_rad = 0.0F;

    estimator->pendulum_rate_rad_s = 0.0F;
    estimator->arm_rate_rad_s = 0.0F;

    estimator->previous_timestamp_us = 0U;
}

state_estimator_result_t state_estimator_step(
    state_estimator_t *estimator,
    const control_config_t *config,
    const state_estimator_input_t *input,
    control_state_t *state)
{
    uint32_t elapsed_us;
    float elapsed_s;
    float minimum_period_s;
    float maximum_period_s;

    float pendulum_delta_rad;
    float arm_delta_rad;
    float raw_pendulum_rate_rad_s;
    float raw_arm_rate_rad_s;

    if ((estimator == NULL) ||
        (config == NULL) ||
        (input == NULL) ||
        (state == NULL)) {
        return STATE_ESTIMATOR_ERROR_ARGUMENT;
    }

    if (control_config_validate_estimator(config) !=
        CONTROL_CONFIG_OK) {
        return STATE_ESTIMATOR_ERROR_CONFIG;
    }

    if (!estimator_pendulum_angle_is_valid(
            input->pendulum_angle_rad) ||
        !estimator_value_is_finite(
            input->arm_angle_rad)) {
        return STATE_ESTIMATOR_ERROR_SAMPLE;
    }

    if (!estimator->initialized) {
        estimator_seed_from_sample(
            estimator, input, state);
        return STATE_ESTIMATOR_RESULT_INITIALIZED;
    }

    /*
     * Unsigned subtraction intentionally supports the
     * normal uint32_t timestamp wrap-around case.
     *
     * A delta in the upper half of the uint32_t range is
     * interpreted as a duplicate or backwards timestamp.
     */
    elapsed_us =
        input->timestamp_us -
        estimator->previous_timestamp_us;

    if ((elapsed_us == 0U) ||
        (elapsed_us >= 0x80000000U)) {
        return STATE_ESTIMATOR_ERROR_TIMESTAMP;
    }

    elapsed_s = (float)elapsed_us * 0.000001F;

    minimum_period_s =
        config->sample_period_s *
        STATE_ESTIMATOR_MIN_PERIOD_RATIO;

    maximum_period_s =
        config->sample_period_s *
        STATE_ESTIMATOR_MAX_PERIOD_RATIO;

    if (elapsed_s < minimum_period_s) {
        return STATE_ESTIMATOR_ERROR_TIMESTAMP;
    }

    if (elapsed_s > maximum_period_s) {
        /*
         * A missed real-time interval must not leave the estimator
         * permanently referenced to an obsolete timestamp. Re-seed from
         * the current sample and require one fresh interval before rates are
         * considered ready again.
         */
        estimator_seed_from_sample(
            estimator, input, state);
        return STATE_ESTIMATOR_RESULT_INITIALIZED;
    }

    pendulum_delta_rad = estimator_wrapped_delta(
        input->pendulum_angle_rad,
        estimator->previous_pendulum_angle_rad);

    /*
     * Pendulum angle is circular; arm position is continuous relative
     * to the configured home/reference and must not wrap at +/-pi.
     */
    arm_delta_rad =
        input->arm_angle_rad -
        estimator->previous_arm_angle_rad;

    /*
     * The configured period defines the admissible timing window.
     * Derivatives use measured elapsed time so accepted jitter does not
     * bias the estimated angular rates.
     */
    raw_pendulum_rate_rad_s =
        pendulum_delta_rad /
        elapsed_s;

    raw_arm_rate_rad_s =
        arm_delta_rad /
        elapsed_s;

    if (!estimator_value_is_finite(
            raw_pendulum_rate_rad_s) ||
        !estimator_value_is_finite(
            raw_arm_rate_rad_s)) {
        return STATE_ESTIMATOR_ERROR_SAMPLE;
    }

    estimator->pendulum_rate_rad_s =
        estimator_filter_rate(
            estimator->pendulum_rate_rad_s,
            raw_pendulum_rate_rad_s,
            config->rate_filter_alpha);

    estimator->arm_rate_rad_s =
        estimator_filter_rate(
            estimator->arm_rate_rad_s,
            raw_arm_rate_rad_s,
            config->rate_filter_alpha);

    estimator->previous_pendulum_angle_rad =
        input->pendulum_angle_rad;

    estimator->previous_arm_angle_rad =
        input->arm_angle_rad;

    estimator->previous_timestamp_us =
        input->timestamp_us;

    estimator_write_state(estimator, input, state);

    return STATE_ESTIMATOR_RESULT_OK;
}
