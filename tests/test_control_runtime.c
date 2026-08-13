#include <assert.h>
#include <stdint.h>

#include "control_runtime.h"

static void test_defaults_are_supported(void)
{
    control_runtime_config_t runtime;

    control_runtime_config_init_defaults(&runtime);

    assert(control_runtime_config_validate(&runtime) ==
           CONTROL_RUNTIME_CONFIG_OK);
}

static void test_unimplemented_algorithms_are_rejected(void)
{
    control_runtime_config_t runtime;
    uint32_t errors;

    control_runtime_config_init_defaults(&runtime);
    runtime.estimator_mode = STATE_ESTIMATOR_KALMAN;
    errors = control_runtime_config_validate(&runtime);
    assert((errors &
            CONTROL_RUNTIME_CONFIG_ERROR_ESTIMATOR) != 0U);

    control_runtime_config_init_defaults(&runtime);
    runtime.balance_controller = BALANCE_CONTROLLER_LQI;
    errors = control_runtime_config_validate(&runtime);
    assert((errors &
            CONTROL_RUNTIME_CONFIG_ERROR_BALANCE) != 0U);
}

static void test_unimplemented_conditional_modes_are_rejected(void)
{
    control_runtime_config_t runtime;
    uint32_t errors;

    control_runtime_config_init_defaults(&runtime);
    runtime.swing_up_enabled = true;
    errors = control_runtime_config_validate(&runtime);
    assert((errors &
            CONTROL_RUNTIME_CONFIG_ERROR_SWING_UP) != 0U);

    control_runtime_config_init_defaults(&runtime);
    runtime.capture_enabled = true;
    errors = control_runtime_config_validate(&runtime);
    assert((errors &
            CONTROL_RUNTIME_CONFIG_ERROR_CAPTURE) != 0U);
}

static void test_invalid_enum_values_are_rejected(void)
{
    control_runtime_config_t runtime;
    uint32_t errors;

    control_runtime_config_init_defaults(&runtime);
    runtime.estimator_mode = (state_estimator_mode_t)99;
    runtime.balance_controller =
        (balance_controller_mode_t)99;

    errors = control_runtime_config_validate(&runtime);

    assert((errors &
            CONTROL_RUNTIME_CONFIG_ERROR_ESTIMATOR) != 0U);
    assert((errors &
            CONTROL_RUNTIME_CONFIG_ERROR_BALANCE) != 0U);
}

int main(void)
{
    assert(control_runtime_config_validate(0) ==
           CONTROL_RUNTIME_CONFIG_ERROR_ARGUMENT);

    test_defaults_are_supported();
    test_unimplemented_algorithms_are_rejected();
    test_unimplemented_conditional_modes_are_rejected();
    test_invalid_enum_values_are_rejected();

    return 0;
}
