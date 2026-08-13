#ifndef ACTUATOR_MAPPER_H
#define ACTUATOR_MAPPER_H

#include "control_types.h"

/*
 * Basic initial mapping:
 *     normalized u -> direction + |u| * 100 percent PWM.
 *
 * Dead-zone, VBUS normalization, braking policy and asymmetric compensation
 * belong here later without changing the controller interface.
 */
actuator_command_t actuator_mapper_step(
    const control_command_t *control,
    const control_state_t *state);

#endif
