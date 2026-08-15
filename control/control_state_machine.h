#ifndef CONTROL_STATE_MACHINE_H
#define CONTROL_STATE_MACHINE_H

#include <stdbool.h>

#include "control_runtime.h"
#include "control_types.h"
#include "state_safety.h"

typedef struct {
    control_mode_t mode;
    control_mode_t requested_mode;

    bool clear_fault_requested;
} control_state_machine_t;

void control_state_machine_init(
    control_state_machine_t *machine);

bool control_state_machine_request_mode(
    control_state_machine_t *machine,
    control_mode_t mode);

void control_state_machine_request_fault_clear(
    control_state_machine_t *machine);

void control_state_machine_step(
    control_state_machine_t *machine,
    const control_runtime_config_t *runtime,
    const control_state_t *state,
    const state_safety_result_t *safety);

control_mode_t control_state_machine_get_mode(
    const control_state_machine_t *machine);

bool control_state_machine_mode_is_active(
    control_mode_t mode);

bool control_state_machine_mode_is_admission_ready(
    control_mode_t mode);

#endif
