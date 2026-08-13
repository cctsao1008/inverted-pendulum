#include "lqi_controller.h"

void lqi_controller_init(
    lqi_controller_t *controller)
{
    if (controller == 0) {
        return;
    }

    controller->integral_state = 0.0F;
    controller->integration_enabled = false;
}

void lqi_controller_sync_mode(
    lqi_controller_t *controller,
    control_mode_t mode)
{
    if (controller == 0) {
        return;
    }

    if (mode == CONTROL_MODE_BALANCE) {
        controller->integration_enabled = true;
        return;
    }

    controller->integration_enabled = false;
    controller->integral_state = 0.0F;
}

control_command_t lqi_controller_step(
    lqi_controller_t *controller,
    const control_config_t *config,
    const control_state_t *state)
{
    (void)controller;
    (void)config;
    (void)state;

    return control_command_zero();
}
