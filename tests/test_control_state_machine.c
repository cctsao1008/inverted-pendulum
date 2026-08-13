#include <assert.h>

#include "control_state_machine.h"

static state_safety_result_t safety_result(
    bool allowed,
    bool hard_fault)
{
    state_safety_result_t result = {0};

    result.control_allowed = allowed;
    result.hard_fault = hard_fault;
    return result;
}

int main(void)
{
    control_state_machine_t machine;
    control_runtime_config_t runtime;
    control_state_t state = {0};
    state_safety_result_t safety;

    control_runtime_config_init_defaults(&runtime);

    assert(!control_state_machine_mode_is_active(
        CONTROL_MODE_DISABLED));
    assert(!control_state_machine_mode_is_active(
        CONTROL_MODE_IDLE));
    assert(control_state_machine_mode_is_active(
        CONTROL_MODE_SWING_UP));
    assert(control_state_machine_mode_is_active(
        CONTROL_MODE_CAPTURE));
    assert(control_state_machine_mode_is_active(
        CONTROL_MODE_BALANCE));
    assert(!control_state_machine_mode_is_active(
        CONTROL_MODE_FAULT));

    control_state_machine_init(&machine);
    assert(control_state_machine_get_mode(&machine) ==
        CONTROL_MODE_DISABLED);

    safety = safety_result(true, false);
    assert(control_state_machine_request_mode(
        &machine, CONTROL_MODE_BALANCE));
    control_state_machine_step(
        &machine, &runtime, &state, &safety);
    assert(control_state_machine_get_mode(&machine) ==
        CONTROL_MODE_BALANCE);

    safety = safety_result(false, true);
    control_state_machine_step(
        &machine, &runtime, &state, &safety);
    assert(control_state_machine_get_mode(&machine) ==
        CONTROL_MODE_FAULT);

    control_state_machine_request_fault_clear(&machine);
    control_state_machine_step(
        &machine, &runtime, &state, &safety);
    assert(control_state_machine_get_mode(&machine) ==
        CONTROL_MODE_FAULT);

    safety = safety_result(false, false);
    control_state_machine_request_fault_clear(&machine);
    control_state_machine_step(
        &machine, &runtime, &state, &safety);
    assert(control_state_machine_get_mode(&machine) ==
        CONTROL_MODE_DISABLED);

    assert(control_state_machine_request_mode(
        &machine, CONTROL_MODE_SWING_UP));
    safety = safety_result(true, false);
    control_state_machine_step(
        &machine, &runtime, &state, &safety);
    assert(control_state_machine_get_mode(&machine) ==
        CONTROL_MODE_DISABLED);

    runtime.swing_up_enabled = true;
    control_state_machine_step(
        &machine, &runtime, &state, &safety);
    assert(control_state_machine_get_mode(&machine) ==
        CONTROL_MODE_SWING_UP);

    safety = safety_result(false, false);
    control_state_machine_step(
        &machine, &runtime, &state, &safety);
    assert(control_state_machine_get_mode(&machine) ==
        CONTROL_MODE_IDLE);

    return 0;
}
