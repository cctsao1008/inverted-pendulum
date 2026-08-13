#include "control_types.h"

control_command_t control_command_zero(void)
{
    control_command_t command;

    command.u = 0.0F;
    command.valid = true;

    return command;
}

actuator_command_t actuator_command_safe_off(void)
{
    actuator_command_t command;

    command.direction = MOTOR_DIRECTION_NONE;
    command.pwm_percent = 0.0F;
    command.brake = false;
    command.valid = true;

    return command;
}
