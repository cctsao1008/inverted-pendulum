#include "motor_authority.h"

#include <stddef.h>

static bool motor_authority_is_active_owner(
    motor_authority_owner_t owner)
{
    return (owner == MOTOR_AUTHORITY_MAINTENANCE) ||
           (owner == MOTOR_AUTHORITY_CONTROL);
}

static void motor_authority_safe_off(
    motor_authority_t *authority)
{
    authority->last_output_percent = 0;

    if (authority->output != NULL) {
        authority->output(0, authority->output_context);
    }
}

void motor_authority_init(
    motor_authority_t *authority,
    motor_authority_output_fn output,
    void *output_context)
{
    if (authority == NULL) {
        return;
    }

    authority->owner = MOTOR_AUTHORITY_NONE;
    authority->output = output;
    authority->output_context = output_context;
    authority->last_output_percent = 0;

    motor_authority_safe_off(authority);
}

bool motor_authority_acquire(
    motor_authority_t *authority,
    motor_authority_owner_t owner)
{
    if ((authority == NULL) ||
        !motor_authority_is_active_owner(owner) ||
        (authority->owner == MOTOR_AUTHORITY_FAULT)) {
        return false;
    }

    if (authority->owner == MOTOR_AUTHORITY_NONE) {
        authority->owner = owner;
        return true;
    }

    return authority->owner == owner;
}

void motor_authority_release(
    motor_authority_t *authority,
    motor_authority_owner_t owner)
{
    if ((authority == NULL) ||
        !motor_authority_is_active_owner(owner) ||
        (authority->owner != owner)) {
        return;
    }

    motor_authority_safe_off(authority);
    authority->owner = MOTOR_AUTHORITY_NONE;
}

bool motor_authority_apply(
    motor_authority_t *authority,
    motor_authority_owner_t owner,
    int8_t signed_percent)
{
    if ((authority == NULL) ||
        !motor_authority_is_active_owner(owner) ||
        (authority->owner != owner)) {
        return false;
    }

    authority->last_output_percent = signed_percent;

    if (authority->output != NULL) {
        authority->output(
            signed_percent,
            authority->output_context);
    }

    return true;
}

void motor_authority_enter_fault(motor_authority_t *authority)
{
    if (authority == NULL) {
        return;
    }

    motor_authority_safe_off(authority);
    authority->owner = MOTOR_AUTHORITY_FAULT;
}

bool motor_authority_clear_fault(motor_authority_t *authority)
{
    if ((authority == NULL) ||
        (authority->owner != MOTOR_AUTHORITY_FAULT)) {
        return false;
    }

    motor_authority_safe_off(authority);
    authority->owner = MOTOR_AUTHORITY_NONE;
    return true;
}

motor_authority_owner_t motor_authority_owner(
    const motor_authority_t *authority)
{
    return (authority == NULL)
        ? MOTOR_AUTHORITY_FAULT
        : authority->owner;
}

int8_t motor_authority_last_output(
    const motor_authority_t *authority)
{
    return (authority == NULL)
        ? 0
        : authority->last_output_percent;
}

const char *motor_authority_owner_name(
    motor_authority_owner_t owner)
{
    switch (owner) {
    case MOTOR_AUTHORITY_NONE:
        return "none";
    case MOTOR_AUTHORITY_MAINTENANCE:
        return "maintenance";
    case MOTOR_AUTHORITY_CONTROL:
        return "control";
    case MOTOR_AUTHORITY_FAULT:
    default:
        return "fault";
    }
}
