#include <assert.h>
#include <math.h>

#include "actuator_mapper.h"

int main(void)
{
    control_state_t state = {0};
    control_command_t control;
    actuator_command_t actuator;

    control.valid = true;
    control.u = 0.25F;
    actuator = actuator_mapper_step(&control, &state);
    assert(actuator.valid);
    assert(actuator.direction == MOTOR_DIRECTION_RIGHT);
    assert(actuator.pwm_percent == 25.0F);

    control.u = -0.40F;
    actuator = actuator_mapper_step(&control, &state);
    assert(actuator.valid);
    assert(actuator.direction == MOTOR_DIRECTION_LEFT);
    assert(actuator.pwm_percent == 40.0F);

    control.u = 0.0F;
    actuator = actuator_mapper_step(&control, &state);
    assert(actuator.valid);
    assert(actuator.direction == MOTOR_DIRECTION_NONE);
    assert(actuator.pwm_percent == 0.0F);

    control.valid = false;
    actuator = actuator_mapper_step(&control, &state);
    assert(!actuator.valid);
    assert(actuator.pwm_percent == 0.0F);

    control.valid = true;
    control.u = NAN;
    actuator = actuator_mapper_step(&control, &state);
    assert(!actuator.valid);
    assert(actuator.pwm_percent == 0.0F);

    return 0;
}
