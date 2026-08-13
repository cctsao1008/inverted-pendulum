#ifndef LQI_CONTROLLER_H
#define LQI_CONTROLLER_H

#include <stdbool.h>

#include "control_config.h"
#include "control_types.h"

typedef struct {
    float integral_state;
    bool integration_enabled;
} lqi_controller_t;

void lqi_controller_init(
    lqi_controller_t *controller);

void lqi_controller_sync_mode(
    lqi_controller_t *controller,
    control_mode_t mode);

control_command_t lqi_controller_step(
    lqi_controller_t *controller,
    const control_config_t *config,
    const control_state_t *state);

#endif
