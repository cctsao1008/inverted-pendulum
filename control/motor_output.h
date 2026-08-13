#ifndef MOTOR_OUTPUT_H
#define MOTOR_OUTPUT_H

#include "control_types.h"

typedef void (*motor_output_apply_fn)(
    const actuator_command_t *command,
    void *context);

typedef void (*motor_output_safe_off_fn)(
    void *context);

typedef struct {
    motor_output_apply_fn apply;
    motor_output_safe_off_fn safe_off;
    void *context;
} motor_output_t;

void motor_output_init(
    motor_output_t *output);

void motor_output_set_sink(
    motor_output_t *output,
    motor_output_apply_fn apply,
    motor_output_safe_off_fn safe_off,
    void *context);

void motor_output_apply(
    const motor_output_t *output,
    const actuator_command_t *command);

void motor_output_safe_off(
    const motor_output_t *output);

#endif
