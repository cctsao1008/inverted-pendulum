#include "controller_dispatch.h"

#include "capture_controller.h"
#include "energy_swing_up.h"
#include "lqr_controller.h"

control_command_t controller_dispatch(
    const control_state_machine_t *state_machine,
    const control_runtime_config_t *runtime,
    const control_config_t *config,
    lqi_controller_t *lqi,
    const control_state_t *state)
{
    control_mode_t mode;

    if ((state_machine == 0) ||
        (runtime == 0) ||
        (state == 0)) {
        control_command_t command =
            control_command_zero();

        command.valid = false;
        return command;
    }

    mode = control_state_machine_get_mode(
        state_machine);

    switch (mode) {
    case CONTROL_MODE_SWING_UP:
        return energy_swing_up_step(state);

    case CONTROL_MODE_CAPTURE:
        return capture_controller_step(state);

    case CONTROL_MODE_BALANCE:
        if (runtime->balance_controller ==
            BALANCE_CONTROLLER_LQI) {
            return lqi_controller_step(
                lqi,
                config,
                state);
        }

        return lqr_controller_step(
            config,
            state);

    case CONTROL_MODE_DISABLED:
    case CONTROL_MODE_IDLE:
    case CONTROL_MODE_FAULT:
    default:
        return control_command_zero();
    }
}
