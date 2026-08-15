#ifndef CLOSED_LOOP_MODE_MANAGER_H
#define CLOSED_LOOP_MODE_MANAGER_H

#include <stdbool.h>

#include "closed_loop_enable_gate.h"
#include "control_pipeline.h"

void app_closed_loop_mode_prepare(
    control_pipeline_t *pipeline,
    bool operator_enable_requested);

void app_closed_loop_mode_apply_gate(
    control_pipeline_t *pipeline,
    bool operator_enable_requested,
    const closed_loop_gate_result_t *gate_result);

#endif
