#ifndef OUTPUT_SAFETY_H
#define OUTPUT_SAFETY_H

#include <stdbool.h>

#include "control_types.h"
#include "state_safety.h"

typedef struct {
    bool configured;
    float max_pwm_percent;
} output_safety_limits_t;

void output_safety_limits_init_unconfigured(
    output_safety_limits_t *limits);

actuator_command_t output_safety_apply(
    const actuator_command_t *requested,
    const state_safety_result_t *state_safety,
    const output_safety_limits_t *limits);

#endif
