#include "closed_loop_mode_manager.h"

#include <stddef.h>

void app_closed_loop_mode_prepare(
    control_pipeline_t *pipeline,
    bool operator_enable_requested)
{
    control_mode_t mode;

    if (pipeline == NULL) {
        return;
    }

    mode = control_pipeline_get_mode(pipeline);

    if (!operator_enable_requested) {
        (void)control_pipeline_request_mode(
            pipeline,
            CONTROL_MODE_DISABLED);
        return;
    }

    if (mode == CONTROL_MODE_DISABLED) {
        (void)control_pipeline_request_mode(
            pipeline,
            CONTROL_MODE_IDLE);
    }
}

void app_closed_loop_mode_apply_gate(
    control_pipeline_t *pipeline,
    bool operator_enable_requested,
    const closed_loop_gate_result_t *gate_result)
{
    control_mode_t mode;

    if ((pipeline == NULL) || (gate_result == NULL)) {
        return;
    }

    mode = control_pipeline_get_mode(pipeline);

    if (!operator_enable_requested) {
        (void)control_pipeline_request_mode(
            pipeline,
            CONTROL_MODE_DISABLED);
        return;
    }

    if (gate_result->allowed) {
        (void)control_pipeline_request_mode(
            pipeline,
            CONTROL_MODE_BALANCE);
        return;
    }

    if (control_state_machine_mode_is_active(mode)) {
        (void)control_pipeline_request_mode(
            pipeline,
            CONTROL_MODE_IDLE);
    }
}
