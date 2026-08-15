#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "closed_loop_enable_gate.h"

static closed_loop_gate_input_t valid_input(void)
{
    closed_loop_gate_input_t input = {
        true,
        true,
        CONTROL_MODE_BALANCE,
        true,
        500U,
        100,
        MOTOR_AUTHORITY_NONE
    };

    return input;
}

int main(void)
{
    closed_loop_gate_config_t config;
    closed_loop_gate_input_t input;
    closed_loop_gate_result_t result;

    closed_loop_gate_config_init_unconfigured(&config);
    input = valid_input();

    result = closed_loop_gate_evaluate(&config, &input);
    assert(!result.allowed);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_CONFIG) != 0U);

    config.max_sample_age_us = 2000U;
    config.max_abs_entry_theta_mrad = 250;
    assert(closed_loop_gate_config_validate(&config));

    result = closed_loop_gate_evaluate(&config, &input);
    assert(result.allowed);
    assert(result.reject_flags == CLOSED_LOOP_GATE_OK);

    input.operator_enable_requested = false;
    result = closed_loop_gate_evaluate(&config, &input);
    assert(!result.allowed);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_OPERATOR) != 0U);
    input = valid_input();

    input.runtime_ready = false;
    result = closed_loop_gate_evaluate(&config, &input);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_RUNTIME) != 0U);
    input = valid_input();

    input.control_mode = CONTROL_MODE_IDLE;
    result = closed_loop_gate_evaluate(&config, &input);
    assert(result.allowed);
    assert(result.reject_flags == CLOSED_LOOP_GATE_OK);
    input = valid_input();

    input.control_mode = CONTROL_MODE_DISABLED;
    result = closed_loop_gate_evaluate(&config, &input);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_MODE) != 0U);
    input = valid_input();

    input.control_allowed = false;
    result = closed_loop_gate_evaluate(&config, &input);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_STATE_SAFETY) != 0U);
    input = valid_input();

    input.sample_age_us = 2001U;
    result = closed_loop_gate_evaluate(&config, &input);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_SAMPLE_STALE) != 0U);
    input = valid_input();

    input.theta_mrad = -251;
    result = closed_loop_gate_evaluate(&config, &input);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_ENTRY_ANGLE) != 0U);
    input = valid_input();

    input.authority_owner = MOTOR_AUTHORITY_MAINTENANCE;
    result = closed_loop_gate_evaluate(&config, &input);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_AUTHORITY) != 0U);

    input = valid_input();
    input.operator_enable_requested = false;
    input.runtime_ready = false;
    input.control_mode = CONTROL_MODE_DISABLED;
    input.control_allowed = false;
    input.sample_age_us = 5000U;
    input.theta_mrad = 1000;
    input.authority_owner = MOTOR_AUTHORITY_FAULT;

    result = closed_loop_gate_evaluate(&config, &input);
    assert(!result.allowed);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_OPERATOR) != 0U);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_RUNTIME) != 0U);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_MODE) != 0U);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_STATE_SAFETY) != 0U);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_SAMPLE_STALE) != 0U);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_ENTRY_ANGLE) != 0U);
    assert((result.reject_flags &
            CLOSED_LOOP_GATE_REJECT_AUTHORITY) != 0U);

    printf("closed_loop_enable_gate: PASS\n");
    return 0;
}
