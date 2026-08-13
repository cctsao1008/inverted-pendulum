#include "lqr_controller.h"

#include "balance_controller.h"

control_command_t lqr_controller_step(
    const control_config_t *config,
    const control_state_t *state)
{
    control_command_t command =
        control_command_zero();
    float normalized_output = 0.0F;

    if (balance_controller_compute(
            config,
            state,
            &normalized_output) !=
        BALANCE_CONTROLLER_RESULT_OK) {
        command.valid = false;
        return command;
    }

    command.u = normalized_output;
    command.valid = true;

    return command;
}
