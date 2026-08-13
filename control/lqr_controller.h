#ifndef LQR_CONTROLLER_H
#define LQR_CONTROLLER_H

#include "control_config.h"
#include "control_types.h"

control_command_t lqr_controller_step(
    const control_config_t *config,
    const control_state_t *state);

#endif
