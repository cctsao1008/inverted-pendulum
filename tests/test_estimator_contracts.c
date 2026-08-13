#include <assert.h>
#include <math.h>

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

int main(void)
{
    control_config_t config;
    state_estimator_t estimator;
    state_estimator_input_t input = {0.0F, 3.13F, 1000U};
    control_state_t state;

    control_config_init_invalid(&config);
    config.sample_period_s = 0.001F;
    config.rate_filter_alpha = 1.0F;

    assert(control_config_validate_estimator(&config) ==
        CONTROL_CONFIG_OK);
    assert((control_config_validate(&config) &
            CONTROL_CONFIG_ERROR_BALANCE_GAIN) != 0U);

    state_estimator_init(&estimator);
    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_INITIALIZED);

    input.pendulum_angle_rad = 0.05F;
    input.arm_angle_rad = 3.15F;
    input.timestamp_us += 500U;

    assert(state_estimator_step(
        &estimator, &config, &input, &state) ==
        STATE_ESTIMATOR_RESULT_OK);

    assert(nearly_equal(
        state.pendulum_rate_rad_s,
        100.0F,
        0.01F));
    assert(nearly_equal(
        state.arm_rate_rad_s,
        40.0F,
        0.01F));
    assert(state.arm_angle_rad > TEST_PI_F);

    return 0;
}
