#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "control_config.h"

static control_config_t make_valid_test_config(void)
{
    control_config_t config;

    control_config_init_invalid(&config);

    /*
     * Unit-test fixture only.
     * These are not identified plant parameters or production gains.
     */
    config.sample_period_s = 0.001F;
    config.capture_angle_rad = 0.10F;
    config.escape_angle_rad = 0.35F;
    config.max_normalized_output = 0.80F;

    config.balance_gain[0] = 1.0F;
    config.balance_gain[1] = 1.0F;
    config.balance_gain[2] = 1.0F;
    config.balance_gain[3] = 1.0F;
    config.balance_gains_configured = true;

    return config;
}

static void test_null_config_is_rejected(void)
{
    assert(control_config_validate(NULL) ==
           CONTROL_CONFIG_ERROR_NULL);
}

static void test_default_config_is_rejected(void)
{
    control_config_t config;
    uint32_t errors;

    control_config_init_invalid(&config);
    errors = control_config_validate(&config);

    assert((errors &
            CONTROL_CONFIG_ERROR_SAMPLE_TIME) != 0U);
    assert((errors &
            CONTROL_CONFIG_ERROR_ANGLE_WINDOW) != 0U);
    assert((errors &
            CONTROL_CONFIG_ERROR_OUTPUT_LIMIT) != 0U);
    assert((errors &
            CONTROL_CONFIG_ERROR_BALANCE_GAIN) != 0U);
}

static void test_valid_config_is_accepted(void)
{
    control_config_t config = make_valid_test_config();

    assert(control_config_validate(&config) ==
           CONTROL_CONFIG_OK);
}

static void test_invalid_angle_order_is_rejected(void)
{
    control_config_t config = make_valid_test_config();

    config.capture_angle_rad = 0.40F;
    config.escape_angle_rad = 0.20F;

    assert((control_config_validate(&config) &
            CONTROL_CONFIG_ERROR_ANGLE_WINDOW) != 0U);
}

static void test_nan_gain_is_rejected(void)
{
    control_config_t config = make_valid_test_config();

    config.balance_gain[2] = 0.0F / 0.0F;

    assert((control_config_validate(&config) &
            CONTROL_CONFIG_ERROR_BALANCE_GAIN) != 0U);
}

int main(void)
{
    test_null_config_is_rejected();
    test_default_config_is_rejected();
    test_valid_config_is_accepted();
    test_invalid_angle_order_is_rejected();
    test_nan_gain_is_rejected();

    printf("PASS: control configuration tests\n");
    return 0;
}
