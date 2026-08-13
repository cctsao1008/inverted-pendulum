#include "control_runtime.h"

void control_runtime_config_init_defaults(
    control_runtime_config_t *config)
{
    if (config == 0) {
        return;
    }

    config->estimator_mode = STATE_ESTIMATOR_BASIC;
    config->balance_controller = BALANCE_CONTROLLER_LQR;
    config->swing_up_enabled = false;
    config->capture_enabled = false;
    config->motor_output_enabled = false;
    config->telemetry_enabled = false;
}

uint32_t control_runtime_config_validate(
    const control_runtime_config_t *config)
{
    uint32_t errors = CONTROL_RUNTIME_CONFIG_OK;

    if (config == 0) {
        return CONTROL_RUNTIME_CONFIG_ERROR_ARGUMENT;
    }

    switch (config->estimator_mode) {
    case STATE_ESTIMATOR_BASIC:
    case STATE_ESTIMATOR_KALMAN:
        break;

    default:
        errors |= CONTROL_RUNTIME_CONFIG_ERROR_ESTIMATOR;
        break;
    }

    switch (config->balance_controller) {
    case BALANCE_CONTROLLER_LQR:
    case BALANCE_CONTROLLER_LQI:
        break;

    default:
        errors |= CONTROL_RUNTIME_CONFIG_ERROR_BALANCE;
        break;
    }

    return errors;
}
