#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

#include "balance_controller.h"
#include "control_config.h"
#include "control_types.h"

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

    config.balance_gain[0] = 2.0F;
    config.balance_gain[1] = 3.0F;
    config.balance_gain[2] = 4.0F;
    config.balance_gain[3] = 5.0F;
    config.balance_gains_configured = true;

    return config;
}

static control_state_t make_zero_state(void)
{
    control_state_t state;

    state.pendulum_angle_rad = 0.0F;
    state.pendulum_rate_rad_s = 0.0F;
    state.arm_angle_rad = 0.0F;
    state.arm_rate_rad_s = 0.0F;
    state.timestamp_us = 1000U;

    return state;
}

static void test_zero_state_produces_zero_output(void)
{
    control_config_t config = make_valid_config();
    control_state_t state = make_zero_state();
    float output = 1.0F;

    assert(balance_controller_compute(
        &config, &state, &output) ==
        BALANCE_CONTROLLER_RESULT_OK);

    assert(output == 0.0F);
}

static void test_all_four_states_are_used(void)
{
    control_config_t config = make_valid_config();
    control_state_t state = make_zero_state();
    float output = 0.0F;

    state.pendulum_angle_rad = 0.10F;
    state.pendulum_rate_rad_s = 0.02F;
    state.arm_angle_rad = -0.03F;
    state.arm_rate_rad_s = 0.04F;

    /*
     * Kx =
     *     2 * 0.10
     *   + 3 * 0.02
     *   + 4 * -0.03
     *   + 5 * 0.04
     *   = 0.34
     *
     * u = -Kx = -0.34
     */
    assert(balance_controller_compute(
        &config, &state, &output) ==
        BALANCE_CONTROLLER_RESULT_OK);

    assert(nearly_equal(output, -0.34F, 0.0001F));
}

static void test_positive_feedback_saturates_negative(void)
{
    control_config_t config = make_valid_config();
    control_state_t state = make_zero_state();
    float output = 0.0F;

    state.pendulum_angle_rad = 1.0F;

    assert(balance_controller_compute(
        &config, &state, &output) ==
        BALANCE_CONTROLLER_RESULT_OK);

    assert(output == -config.max_normalized_output);
}

static void test_negative_feedback_saturates_positive(void)
{
    control_config_t config = make_valid_config();
    control_state_t state = make_zero_state();
    float output = 0.0F;

    state.pendulum_angle_rad = -1.0F;

    assert(balance_controller_compute(
        &config, &state, &output) ==
        BALANCE_CONTROLLER_RESULT_OK);

    assert(output == config.max_normalized_output);
}

static void test_invalid_state_fails_closed(void)
{
    control_config_t config = make_valid_config();
    control_state_t state = make_zero_state();
    float output = 0.50F;

    state.pendulum_rate_rad_s = NAN;

    assert(balance_controller_compute(
        &config, &state, &output) ==
        BALANCE_CONTROLLER_ERROR_STATE);

    assert(output == 0.0F);
}

static void test_invalid_config_fails_closed(void)
{
    control_config_t config;
    control_state_t state = make_zero_state();
    float output = 0.50F;

    control_config_init_invalid(&config);

    assert(balance_controller_compute(
        &config, &state, &output) ==
        BALANCE_CONTROLLER_ERROR_CONFIG);

    assert(output == 0.0F);
}

static void test_numeric_overflow_fails_closed(void)
{
    control_config_t config = make_valid_config();
    control_state_t state = make_zero_state();
    float output = 0.50F;

    state.pendulum_angle_rad = FLT_MAX;

    assert(balance_controller_compute(
        &config, &state, &output) ==
        BALANCE_CONTROLLER_ERROR_NUMERIC);

    assert(output == 0.0F);
}

static void test_null_argument_fails_closed(void)
{
    control_config_t config = make_valid_config();
    control_state_t state = make_zero_state();
    float output = 0.50F;

    assert(balance_controller_compute(
        NULL, &state, &output) ==
        BALANCE_CONTROLLER_ERROR_ARGUMENT);

    assert(output == 0.0F);

    output = 0.50F;

    assert(balance_controller_compute(
        &config, NULL, &output) ==
        BALANCE_CONTROLLER_ERROR_ARGUMENT);

    assert(output == 0.0F);

    assert(balance_controller_compute(
        &config, &state, NULL) ==
        BALANCE_CONTROLLER_ERROR_ARGUMENT);
}

int main(void)
{
    test_zero_state_produces_zero_output();
    test_all_four_states_are_used();
    test_positive_feedback_saturates_negative();
    test_negative_feedback_saturates_positive();
    test_invalid_state_fails_closed();
    test_invalid_config_fails_closed();
    test_numeric_overflow_fails_closed();
    test_null_argument_fails_closed();

    printf("PASS: balance controller tests\n");
    return 0;
}
