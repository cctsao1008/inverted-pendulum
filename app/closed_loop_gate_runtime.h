#ifndef APP_CLOSED_LOOP_GATE_RUNTIME_H
#define APP_CLOSED_LOOP_GATE_RUNTIME_H

#include <stdbool.h>

#include "closed_loop_enable_gate.h"
#include "control_pipeline.h"
#include "motor_authority.h"

closed_loop_gate_result_t app_closed_loop_gate_evaluate(
    const closed_loop_gate_config_t *config,
    bool operator_enable_request,
    bool runtime_ready,
    const control_pipeline_t *pipeline,
    motor_authority_owner_t authority_owner);

#endif
