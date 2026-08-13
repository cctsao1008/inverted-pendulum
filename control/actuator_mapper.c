#include "actuator_mapper.h"

#include <float.h>

static bool value_is_finite(float value)
{
    return (value == value) &&
           (value <= FLT_MAX) &&
           (value >= -FLT_MAX);
}

actuator_command_t actuator_mapper_step(
    const control_command_t *control,
    const control_state_t *state)
{
    actuator_command_t actuator =
        actuator_command_safe_off();

    (void)state;

    if ((control == 0) ||
        !control->valid ||
        !value_is_finite(control->u)) {
        actuator.valid = false;
        return actuator;
    }

    if (control->u > 0.0F) {
        actuator.direction =
            MOTOR_DIRECTION_RIGHT;
        actuator.pwm_percent =
            control->u * 100.0F;
    } else if (control->u < 0.0F) {
        actuator.direction =
            MOTOR_DIRECTION_LEFT;
        actuator.pwm_percent =
            -control->u * 100.0F;
    } else {
        actuator.direction =
            MOTOR_DIRECTION_NONE;
        actuator.pwm_percent = 0.0F;
    }

    actuator.brake = false;
    actuator.valid = true;

    return actuator;
}
