#include <assert.h>
#include <stddef.h>

#include "closed_loop_mode_manager.h"

static closed_loop_gate_result_t gate_result(bool allowed)
{
    closed_loop_gate_result_t result = {0};

    result.allowed = allowed;
    result.reject_flags = allowed ? CLOSED_LOOP_GATE_OK : CLOSED_LOOP_GATE_REJECT_MODE;
    return result;
}

int main(void)
{
    control_pipeline_t pipeline;
    closed_loop_gate_result_t gate;

    control_pipeline_init(&pipeline);

    app_closed_loop_mode_prepare(&pipeline, false);
    assert(pipeline.state_machine.requested_mode == CONTROL_MODE_DISABLED);

    app_closed_loop_mode_prepare(&pipeline, true);
    assert(pipeline.state_machine.requested_mode == CONTROL_MODE_IDLE);

    gate = gate_result(true);
    app_closed_loop_mode_apply_gate(&pipeline, true, &gate);
    assert(pipeline.state_machine.requested_mode == CONTROL_MODE_BALANCE);

    pipeline.state_machine.mode = CONTROL_MODE_BALANCE;
    pipeline.state_machine.requested_mode = CONTROL_MODE_BALANCE;
    gate = gate_result(false);
    app_closed_loop_mode_apply_gate(&pipeline, true, &gate);
    assert(pipeline.state_machine.requested_mode == CONTROL_MODE_IDLE);

    pipeline.state_machine.mode = CONTROL_MODE_IDLE;
    pipeline.state_machine.requested_mode = CONTROL_MODE_IDLE;
    app_closed_loop_mode_apply_gate(&pipeline, true, &gate);
    assert(pipeline.state_machine.requested_mode == CONTROL_MODE_IDLE);

    pipeline.state_machine.mode = CONTROL_MODE_BALANCE;
    pipeline.state_machine.requested_mode = CONTROL_MODE_BALANCE;
    app_closed_loop_mode_apply_gate(&pipeline, false, &gate);
    assert(pipeline.state_machine.requested_mode == CONTROL_MODE_DISABLED);

    app_closed_loop_mode_prepare(NULL, true);
    app_closed_loop_mode_apply_gate(NULL, true, &gate);
    app_closed_loop_mode_apply_gate(&pipeline, true, NULL);

    return 0;
}
