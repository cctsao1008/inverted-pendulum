#include "control_state_machine.h"

static bool mode_is_valid(control_mode_t mode)
{
    switch (mode) {
    case CONTROL_MODE_DISABLED:
    case CONTROL_MODE_IDLE:
    case CONTROL_MODE_SWING_UP:
    case CONTROL_MODE_CAPTURE:
    case CONTROL_MODE_BALANCE:
    case CONTROL_MODE_FAULT:
        return true;

    default:
        return false;
    }
}

static bool mode_is_enabled(
    control_mode_t mode,
    const control_runtime_config_t *runtime)
{
    switch (mode) {
    case CONTROL_MODE_SWING_UP:
        return runtime->swing_up_enabled;

    case CONTROL_MODE_CAPTURE:
        return runtime->capture_enabled;

    case CONTROL_MODE_DISABLED:
    case CONTROL_MODE_IDLE:
    case CONTROL_MODE_BALANCE:
    case CONTROL_MODE_FAULT:
        return true;

    default:
        return false;
    }
}

void control_state_machine_init(
    control_state_machine_t *machine)
{
    if (machine == 0) {
        return;
    }

    machine->mode = CONTROL_MODE_DISABLED;
    machine->requested_mode = CONTROL_MODE_DISABLED;
    machine->clear_fault_requested = false;
}

bool control_state_machine_request_mode(
    control_state_machine_t *machine,
    control_mode_t mode)
{
    if ((machine == 0) ||
        !mode_is_valid(mode)) {
        return false;
    }

    machine->requested_mode = mode;
    return true;
}

void control_state_machine_request_fault_clear(
    control_state_machine_t *machine)
{
    if (machine == 0) {
        return;
    }

    machine->clear_fault_requested = true;
}

void control_state_machine_step(
    control_state_machine_t *machine,
    const control_runtime_config_t *runtime,
    const control_state_t *state,
    const state_safety_result_t *safety)
{
    control_mode_t requested;

    (void)state;

    if ((machine == 0) ||
        (runtime == 0) ||
        (safety == 0)) {
        return;
    }

    if (machine->mode == CONTROL_MODE_FAULT) {
        if (machine->clear_fault_requested &&
            !safety->hard_fault) {
            machine->mode = CONTROL_MODE_DISABLED;
            machine->requested_mode =
                CONTROL_MODE_DISABLED;
        }

        machine->clear_fault_requested = false;
        return;
    }

    /*
     * An inactive system remains safely disabled when measurements are bad.
     * Once the control path is active, a hard safety fault latches FAULT.
     */
    if (safety->hard_fault &&
        (machine->mode != CONTROL_MODE_DISABLED)) {
        machine->mode = CONTROL_MODE_FAULT;
        machine->requested_mode = CONTROL_MODE_FAULT;
        machine->clear_fault_requested = false;
        return;
    }

    requested = machine->requested_mode;

    if (requested == CONTROL_MODE_FAULT) {
        machine->mode = CONTROL_MODE_FAULT;
        machine->clear_fault_requested = false;
        return;
    }

    if (requested == CONTROL_MODE_DISABLED) {
        machine->mode = CONTROL_MODE_DISABLED;
        machine->clear_fault_requested = false;
        return;
    }

    if (requested == CONTROL_MODE_IDLE) {
        if (!safety->hard_fault) {
            machine->mode = CONTROL_MODE_IDLE;
        }

        machine->clear_fault_requested = false;
        return;
    }

    if (!mode_is_enabled(requested, runtime)) {
        machine->clear_fault_requested = false;
        return;
    }

    if (safety->control_allowed) {
        machine->mode = requested;
    } else if (control_state_machine_mode_is_active(
                   machine->mode)) {
        /*
         * A non-hard loss of readiness drops an active controller to IDLE.
         * Hard failures are handled above and enter FAULT.
         */
        machine->mode = CONTROL_MODE_IDLE;
        machine->requested_mode = CONTROL_MODE_IDLE;
    }

    machine->clear_fault_requested = false;
}

control_mode_t control_state_machine_get_mode(
    const control_state_machine_t *machine)
{
    if (machine == 0) {
        return CONTROL_MODE_FAULT;
    }

    return machine->mode;
}

bool control_state_machine_mode_is_active(
    control_mode_t mode)
{
    return (mode == CONTROL_MODE_SWING_UP) ||
           (mode == CONTROL_MODE_CAPTURE) ||
           (mode == CONTROL_MODE_BALANCE);
}

bool control_state_machine_mode_is_admission_ready(
    control_mode_t mode)
{
    return (mode == CONTROL_MODE_IDLE) ||
           control_state_machine_mode_is_active(mode);
}
