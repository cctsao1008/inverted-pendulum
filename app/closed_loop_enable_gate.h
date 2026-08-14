#ifndef CLOSED_LOOP_ENABLE_GATE_H
#define CLOSED_LOOP_ENABLE_GATE_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_authority.h"
#include "control_types.h"

typedef enum {
    CLOSED_LOOP_GATE_OK = 0U,
    CLOSED_LOOP_GATE_REJECT_CONFIG = (1U << 0),
    CLOSED_LOOP_GATE_REJECT_OPERATOR = (1U << 1),
    CLOSED_LOOP_GATE_REJECT_RUNTIME = (1U << 2),
    CLOSED_LOOP_GATE_REJECT_MODE = (1U << 3),
    CLOSED_LOOP_GATE_REJECT_STATE_SAFETY = (1U << 4),
    CLOSED_LOOP_GATE_REJECT_SAMPLE_STALE = (1U << 5),
    CLOSED_LOOP_GATE_REJECT_ENTRY_ANGLE = (1U << 6),
    CLOSED_LOOP_GATE_REJECT_AUTHORITY = (1U << 7)
} closed_loop_gate_reject_t;

typedef struct {
    uint32_t max_sample_age_us;
    int32_t max_abs_entry_theta_mrad;
} closed_loop_gate_config_t;

typedef struct {
    bool operator_enable_requested;
    bool runtime_ready;
    control_mode_t control_mode;
    bool control_allowed;
    uint32_t sample_age_us;
    int32_t theta_mrad;
    motor_authority_owner_t authority_owner;
} closed_loop_gate_input_t;

typedef struct {
    bool allowed;
    uint32_t reject_flags;
} closed_loop_gate_result_t;

void closed_loop_gate_config_init_unconfigured(
    closed_loop_gate_config_t *config);

bool closed_loop_gate_config_validate(
    const closed_loop_gate_config_t *config);

closed_loop_gate_result_t closed_loop_gate_evaluate(
    const closed_loop_gate_config_t *config,
    const closed_loop_gate_input_t *input);

#endif
