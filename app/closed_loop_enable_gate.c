#include "closed_loop_enable_gate.h"

#include <limits.h>
#include <stddef.h>

#include "control_state_machine.h"

void closed_loop_gate_config_init_unconfigured(
    closed_loop_gate_config_t *config)
{
    if (config == NULL) {
        return;
    }

    config->max_sample_age_us = 0U;
    config->max_abs_entry_theta_mrad = 0;
}

bool closed_loop_gate_config_validate(
    const closed_loop_gate_config_t *config)
{
    return (config != NULL) &&
           (config->max_sample_age_us != 0U) &&
           (config->max_abs_entry_theta_mrad > 0);
}

static uint32_t closed_loop_gate_abs_theta_mrad(
    int32_t theta_mrad)
{
    if (theta_mrad >= 0) {
        return (uint32_t)theta_mrad;
    }

    if (theta_mrad == INT32_MIN) {
        return (uint32_t)INT32_MAX + 1U;
    }

    return (uint32_t)(-theta_mrad);
}

closed_loop_gate_result_t closed_loop_gate_evaluate(
    const closed_loop_gate_config_t *config,
    const closed_loop_gate_input_t *input)
{
    closed_loop_gate_result_t result = {
        false,
        CLOSED_LOOP_GATE_OK
    };

    if (!closed_loop_gate_config_validate(config) ||
        (input == NULL)) {
        result.reject_flags |= CLOSED_LOOP_GATE_REJECT_CONFIG;
        return result;
    }

    if (!input->operator_enable_requested) {
        result.reject_flags |= CLOSED_LOOP_GATE_REJECT_OPERATOR;
    }

    if (!input->runtime_ready) {
        result.reject_flags |= CLOSED_LOOP_GATE_REJECT_RUNTIME;
    }

    if (!control_state_machine_mode_is_active(
            input->control_mode)) {
        result.reject_flags |= CLOSED_LOOP_GATE_REJECT_MODE;
    }

    if (!input->control_allowed) {
        result.reject_flags |= CLOSED_LOOP_GATE_REJECT_STATE_SAFETY;
    }

    if (input->sample_age_us > config->max_sample_age_us) {
        result.reject_flags |= CLOSED_LOOP_GATE_REJECT_SAMPLE_STALE;
    }

    if (closed_loop_gate_abs_theta_mrad(input->theta_mrad) >
        (uint32_t)config->max_abs_entry_theta_mrad) {
        result.reject_flags |= CLOSED_LOOP_GATE_REJECT_ENTRY_ANGLE;
    }

    if (input->authority_owner != MOTOR_AUTHORITY_NONE) {
        result.reject_flags |= CLOSED_LOOP_GATE_REJECT_AUTHORITY;
    }

    result.allowed =
        (result.reject_flags == CLOSED_LOOP_GATE_OK);

    return result;
}
