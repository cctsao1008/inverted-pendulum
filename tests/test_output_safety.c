#include <assert.h>

#include "output_safety.h"

static state_safety_result_t allowed_safety(void)
{
    state_safety_result_t result = {0};

    result.control_allowed = true;
    return result;
}

int main(void)
{
    output_safety_limits_t limits;
    state_safety_result_t safety = allowed_safety();
    actuator_command_t requested;
    actuator_command_t output;

    output_safety_limits_init_unconfigured(&limits);
    requested.direction = MOTOR_DIRECTION_RIGHT;
    requested.pwm_percent = 50.0F;
    requested.brake = false;
    requested.valid = true;

    output = output_safety_apply(
        &requested, &safety, &limits);
    assert(output.direction == MOTOR_DIRECTION_NONE);
    assert(output.pwm_percent == 0.0F);

    limits.configured = true;
    limits.max_pwm_percent = 30.0F;
    output = output_safety_apply(
        &requested, &safety, &limits);
    assert(output.direction == MOTOR_DIRECTION_RIGHT);
    assert(output.pwm_percent == 30.0F);

    safety.control_allowed = false;
    output = output_safety_apply(
        &requested, &safety, &limits);
    assert(output.direction == MOTOR_DIRECTION_NONE);
    assert(output.pwm_percent == 0.0F);

    safety.control_allowed = true;
    requested.pwm_percent = -1.0F;
    output = output_safety_apply(
        &requested, &safety, &limits);
    assert(output.direction == MOTOR_DIRECTION_NONE);
    assert(output.pwm_percent == 0.0F);

    requested.direction = MOTOR_DIRECTION_NONE;
    requested.pwm_percent = 10.0F;
    output = output_safety_apply(
        &requested, &safety, &limits);
    assert(output.direction == MOTOR_DIRECTION_NONE);
    assert(output.pwm_percent == 0.0F);

    return 0;
}
