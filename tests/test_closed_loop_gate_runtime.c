#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "closed_loop_gate_runtime.h"

static void test_unavailable_status_fails_closed(void)
{
    closed_loop_gate_config_t config = {2000U, 250};
    control_pipeline_t pipeline;
    closed_loop_gate_result_t result;

    control_pipeline_init(&pipeline);

    result = app_closed_loop_gate_evaluate(
        &config,
        true,
        true,
        &pipeline,
        MOTOR_AUTHORITY_NONE);

    assert(!result.allowed);
    assert((result.reject_flags & CLOSED_LOOP_GATE_REJECT_RUNTIME) != 0U);
    assert((result.reject_flags & CLOSED_LOOP_GATE_REJECT_MODE) != 0U);
    assert((result.reject_flags & CLOSED_LOOP_GATE_REJECT_STATE_SAFETY) != 0U);
    assert((result.reject_flags & CLOSED_LOOP_GATE_REJECT_SAMPLE_STALE) != 0U);
}

static void test_runtime_not_ready_fails_closed(void)
{
    closed_loop_gate_config_t config = {2000U, 250};
    control_pipeline_t pipeline;
    closed_loop_gate_result_t result;

    control_pipeline_init(&pipeline);

    result = app_closed_loop_gate_evaluate(
        &config,
        false,
        false,
        &pipeline,
        MOTOR_AUTHORITY_NONE);

    assert(!result.allowed);
    assert((result.reject_flags & CLOSED_LOOP_GATE_REJECT_OPERATOR) != 0U);
    assert((result.reject_flags & CLOSED_LOOP_GATE_REJECT_RUNTIME) != 0U);
}

int main(void)
{
    test_unavailable_status_fails_closed();
    test_runtime_not_ready_fails_closed();
    return 0;
}
