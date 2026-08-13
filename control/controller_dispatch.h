#ifndef CONTROLLER_DISPATCH_H
#define CONTROLLER_DISPATCH_H

#include "control_config.h"
#include "control_runtime.h"
#include "control_state_machine.h"
#include "control_types.h"
#include "lqi_controller.h"

control_command_t controller_dispatch(
    const control_state_machine_t *state_machine,
    const control_runtime_config_t *runtime,
    const control_config_t *config,
    lqi_controller_t *lqi,
    const control_state_t *state);

#endif
