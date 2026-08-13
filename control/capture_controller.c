#include "capture_controller.h"

control_command_t capture_controller_step(
    const control_state_t *state)
{
    (void)state;

    return control_command_zero();
}
