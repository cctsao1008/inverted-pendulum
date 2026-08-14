#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "motor_authority.h"

typedef struct {
    int8_t output;
    uint32_t calls;
} fake_motor_t;

static void fake_output(
    int8_t signed_percent,
    void *context)
{
    fake_motor_t *motor = (fake_motor_t *)context;

    motor->output = signed_percent;
    motor->calls++;
}

int main(void)
{
    fake_motor_t motor = {0};
    motor_authority_t authority;

    motor_authority_init(
        &authority,
        fake_output,
        &motor);

    assert(motor_authority_owner(&authority) ==
           MOTOR_AUTHORITY_NONE);
    assert(motor.output == 0);
    assert(motor.calls == 1U);

    assert(motor_authority_acquire(
        &authority,
        MOTOR_AUTHORITY_MAINTENANCE));
    assert(motor_authority_apply(
        &authority,
        MOTOR_AUTHORITY_MAINTENANCE,
        15));
    assert(motor.output == 15);
    assert(motor_authority_last_output(&authority) == 15);

    assert(!motor_authority_acquire(
        &authority,
        MOTOR_AUTHORITY_CONTROL));
    assert(!motor_authority_apply(
        &authority,
        MOTOR_AUTHORITY_CONTROL,
        -20));
    assert(motor.output == 15);

    motor_authority_release(
        &authority,
        MOTOR_AUTHORITY_MAINTENANCE);
    assert(motor_authority_owner(&authority) ==
           MOTOR_AUTHORITY_NONE);
    assert(motor.output == 0);

    assert(motor_authority_acquire(
        &authority,
        MOTOR_AUTHORITY_CONTROL));
    assert(motor_authority_apply(
        &authority,
        MOTOR_AUTHORITY_CONTROL,
        -20));
    assert(motor.output == -20);

    motor_authority_enter_fault(&authority);
    assert(motor_authority_owner(&authority) ==
           MOTOR_AUTHORITY_FAULT);
    assert(motor.output == 0);
    assert(!motor_authority_acquire(
        &authority,
        MOTOR_AUTHORITY_MAINTENANCE));
    assert(!motor_authority_apply(
        &authority,
        MOTOR_AUTHORITY_CONTROL,
        10));

    assert(motor_authority_clear_fault(&authority));
    assert(motor_authority_owner(&authority) ==
           MOTOR_AUTHORITY_NONE);
    assert(!motor_authority_apply(
        &authority,
        MOTOR_AUTHORITY_CONTROL,
        10));

    assert(!motor_authority_clear_fault(&authority));

    printf("motor_authority: PASS\n");
    return 0;
}
