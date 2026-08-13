#include "motor_output.h"

void motor_output_init(
    motor_output_t *output)
{
    if (output == 0) {
        return;
    }

    output->apply = 0;
    output->safe_off = 0;
    output->context = 0;
}

void motor_output_set_sink(
    motor_output_t *output,
    motor_output_apply_fn apply,
    motor_output_safe_off_fn safe_off,
    void *context)
{
    if (output == 0) {
        return;
    }

    output->apply = apply;
    output->safe_off = safe_off;
    output->context = context;
}

void motor_output_apply(
    const motor_output_t *output,
    const actuator_command_t *command)
{
    if ((output == 0) ||
        (command == 0) ||
        !command->valid ||
        (output->apply == 0)) {
        motor_output_safe_off(output);
        return;
    }

    output->apply(
        command,
        output->context);
}

void motor_output_safe_off(
    const motor_output_t *output)
{
    if ((output == 0) ||
        (output->safe_off == 0)) {
        return;
    }

    output->safe_off(output->context);
}
