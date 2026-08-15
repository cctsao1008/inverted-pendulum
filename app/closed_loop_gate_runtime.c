#include "closed_loop_gate_runtime.h"

#include <stdint.h>

closed_loop_gate_result_t app_closed_loop_gate_evaluate(
    const closed_loop_gate_config_t *config,
    bool operator_enable_request,
    bool runtime_ready,
    const control_pipeline_t *pipeline,
    motor_authority_owner_t authority_owner)
{
    control_pipeline_status_t status = {0};
    bool status_available =
        runtime_ready &&
        control_pipeline_get_status(pipeline, &status);
    closed_loop_gate_input_t input = {
        operator_enable_request,
        status_available,
        CONTROL_MODE_DISABLED,
        false,
        UINT32_MAX,
        0,
        authority_owner
    };

    if (status_available) {
        input.control_mode = status.control_mode;
        input.control_allowed = status.control_allowed;
        input.sample_age_us = status.sensor.sample_age_us;
        input.theta_mrad = (int32_t)(
            status.state.pendulum_angle_rad * 1000.0F);
    }

    return closed_loop_gate_evaluate(config, &input);
}
