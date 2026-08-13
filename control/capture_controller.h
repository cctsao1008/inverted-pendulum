#ifndef CAPTURE_CONTROLLER_H
#define CAPTURE_CONTROLLER_H

#include "control_types.h"

control_command_t capture_controller_step(
    const control_state_t *state);

#endif
