#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "control_config.h"
#include "state_estimator.h"

#define TEST_PI_F 3.14159265358979323846F

static bool nearly_equal(
    float actual,
    float expected,
    float tolerance)
{
    return fabsf(actual - expected) <= tolerance;
}

static control_config_t make_valid_config(void)
{
    control_config_t config;

    control_config_init_invalid(&config);

    config.sample_period_s = 0.001F;
    config.capture_angle_rad = 0.10F;
    config.escape_angle_rad = 0.35F;
    config.max_normalized_output = 0.80F;
    config.rate_filter_alpha = 0.25F;

    config.balance_gain[0] = 1.0F;
    config.balance_gain[1] = 1.0F;
    config.balance_gain[2] = 1.0F;
    config.balance_gain[3] = 1.0F;
    config.balance_gains_configured = true;

    return config;
}

static state_estimator_input_t make_input(void)
{
    state_estimator_input_t input;

    input.pendulum_angle_rad = 0.0F;
    input.arm_angle_rad = 0.0F;
    input.timestamp_us = 1000U;

    return input;
}

static void test_first_sample_has_zero_rate(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    state_estimator_init(&estimator);

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    assert(state.pendulum_rate_rad_s == 0.0F);
    assert(state.arm_rate_rad_s == 0.0F);
    assert(state.timestamp_us == input.timestamp_us);
}

static void test_step_is_filtered(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    state_estimator_init(&estimator);

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = 0.10F;
    input.timestamp_us += 1000U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_OK);

    /*
     * Raw rate = 0.10 / 0.001 = 100 rad/s.
     * Filtered rate = 0 + 0.25 * 100 = 25 rad/s.
     */
    assert(nearly_equal(
        state.pendulum_rate_rad_s,
        25.0F,
        0.001F));
}

static void test_constant_ramp_converges(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;
    uint32_t index;

    state_estimator_init(&estimator);

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    for (index = 0U; index < 20U; index++) {
        input.pendulum_angle_rad += 0.01F;
        input.timestamp_us += 1000U;

        assert(state_estimator_step(
            &estimator, &config, &input, &state) ==
            STATE_ESTIMATOR_RESULT_OK);
    }

    /*
     * Ramp rate is 10 rad/s. The filtered derivative
     * should converge close to that value.
     */
    assert(nearly_equal(
        state.pendulum_rate_rad_s,
        10.0F,
        0.05F));
}

static void test_filter_reduces_rate_reversal(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    state_estimator_init(&estimator);

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = 0.01F;
    input.timestamp_us += 1000U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_OK);

    input.pendulum_angle_rad = 0.0F;
    input.timestamp_us += 1000U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_OK);

    /*
     * Second raw rate is -10 rad/s. Filtering keeps
     * the resulting magnitude below the raw magnitude.
     */
    assert(fabsf(state.pendulum_rate_rad_s) < 10.0F);
}

static void test_angle_wrap_uses_shortest_delta(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    config.rate_filter_alpha = 1.0F;
    state_estimator_init(&estimator);

    input.pendulum_angle_rad = 3.13F;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = -3.13F;
    input.timestamp_us += 1000U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_OK);

    /*
     * Wrapped delta is approximately +0.023185 rad,
     * rather than approximately -6.26 rad.
     */
    assert(state.pendulum_rate_rad_s > 20.0F);
    assert(state.pendulum_rate_rad_s < 25.0F);
}

static void test_invalid_angle_is_rejected(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    state_estimator_init(&estimator);
    input.pendulum_angle_rad = TEST_PI_F + 0.01F;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_ERROR_SAMPLE);

    assert(!estimator.initialized);
}

static void test_duplicate_timestamp_is_rejected(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    state_estimator_init(&estimator);

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = 0.10F;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_ERROR_TIMESTAMP);

    /*
     * Rejected samples must not alter estimator history.
     */
    assert(estimator.previous_pendulum_angle_rad == 0.0F);
    assert(estimator.previous_timestamp_us == 1000U);
}

static void test_too_early_sample_is_rejected(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    state_estimator_init(&estimator);

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = 0.10F;
    input.timestamp_us += 250U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_ERROR_TIMESTAMP);
    assert(estimator.previous_timestamp_us == 1000U);
}

static void test_long_gap_reseeds_estimator(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    config.rate_filter_alpha = 1.0F;
    state_estimator_init(&estimator);

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = 0.20F;
    input.arm_angle_rad = 0.30F;
    input.timestamp_us += 2000U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);
    assert(estimator.previous_timestamp_us == input.timestamp_us);
    assert(state.pendulum_rate_rad_s == 0.0F);
    assert(state.arm_rate_rad_s == 0.0F);

    input.pendulum_angle_rad += 0.01F;
    input.arm_angle_rad += 0.02F;
    input.timestamp_us += 1000U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_OK);
    assert(nearly_equal(state.pendulum_rate_rad_s, 10.0F, 0.01F));
    assert(nearly_equal(state.arm_rate_rad_s, 20.0F, 0.01F));
}

static void test_uint32_timestamp_wrap_is_supported(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    state_estimator_init(&estimator);

    input.timestamp_us = UINT32_MAX - 499U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = 0.01F;
    input.timestamp_us = 500U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_OK);
}

static void test_reset_prevents_derivative_kick(void)
{
    state_estimator_t estimator;
    control_config_t config = make_valid_config();
    state_estimator_input_t input = make_input();
    control_state_t state;

    state_estimator_init(&estimator);

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = 0.10F;
    input.timestamp_us += 1000U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_OK);

    assert(state.pendulum_rate_rad_s != 0.0F);

    state_estimator_init(&estimator);

    input.pendulum_angle_rad = -0.20F;
    input.timestamp_us += 1000U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    assert(state.pendulum_rate_rad_s == 0.0F);
    assert(state.arm_rate_rad_s == 0.0F);
}

int main(void)
{
    test_first_sample_has_zero_rate();
    test_step_is_filtered();
    test_constant_ramp_converges();
    test_filter_reduces_rate_reversal();
    test_angle_wrap_uses_shortest_delta();
    test_invalid_angle_is_rejected();
    test_duplicate_timestamp_is_rejected();
    test_too_early_sample_is_rejected();
    test_long_gap_reseeds_estimator();
    test_uint32_timestamp_wrap_is_supported();
    test_reset_prevents_derivative_kick();

    printf("PASS: state estimator tests\n");
    return 0;
}
