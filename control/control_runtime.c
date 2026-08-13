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

    /*
     * BASIC is the only estimator with a completed feedback-path
     * implementation. KALMAN remains a safe architecture stub.
     */
    if (config->estimator_mode != STATE_ESTIMATOR_BASIC) {
        errors |= CONTROL_RUNTIME_CONFIG_ERROR_ESTIMATOR;
    }

    /*
     * LQR is the only completed balance controller. LQI remains a safe stub
     * until its feedback law and lifecycle contract are implemented.
     */
    if (config->balance_controller != BALANCE_CONTROLLER_LQR) {
        errors |= CONTROL_RUNTIME_CONFIG_ERROR_BALANCE;
    }

    if (config->swing_up_enabled) {
        errors |= CONTROL_RUNTIME_CONFIG_ERROR_SWING_UP;
    }

    if (config->capture_enabled) {
        errors |= CONTROL_RUNTIME_CONFIG_ERROR_CAPTURE;
    }

    return errors;
}
