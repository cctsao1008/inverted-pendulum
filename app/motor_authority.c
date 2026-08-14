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
    authority->control_last_update_ms = 0U;
    authority->control_enabled = false;
    authority->control_watchdog_started = false;

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

    if (owner == MOTOR_AUTHORITY_CONTROL) {
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

    if (owner == MOTOR_AUTHORITY_CONTROL) {
        authority->control_watchdog_started = false;
    }
}

bool motor_authority_apply(
    motor_authority_t *authority,
    motor_authority_owner_t owner,
    int8_t signed_percent)
{
    if ((authority == NULL) ||
        (owner != MOTOR_AUTHORITY_MAINTENANCE) ||
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


bool motor_authority_set_control_enabled(
    motor_authority_t *authority,
    bool enabled)
{
    if ((authority == NULL) ||
        (authority->owner == MOTOR_AUTHORITY_FAULT)) {
        return false;
    }

    if (enabled) {
        if (authority->owner == MOTOR_AUTHORITY_MAINTENANCE) {
            return false;
        }

        authority->control_enabled = true;
        return true;
    }

    authority->control_enabled = false;

    if (authority->owner == MOTOR_AUTHORITY_CONTROL) {
        motor_authority_safe_off(authority);
        authority->owner = MOTOR_AUTHORITY_NONE;
    }

    authority->control_watchdog_started = false;
    return true;
}

bool motor_authority_control_enabled(
    const motor_authority_t *authority)
{
    return (authority != NULL) &&
           authority->control_enabled;
}

bool motor_authority_control_command(
    motor_authority_t *authority,
    int8_t signed_percent,
    uint32_t now_ms)
{
    if ((authority == NULL) ||
        !authority->control_enabled ||
        (authority->owner == MOTOR_AUTHORITY_FAULT) ||
        (authority->owner == MOTOR_AUTHORITY_MAINTENANCE)) {
        return false;
    }

    if (authority->owner == MOTOR_AUTHORITY_NONE) {
        authority->owner = MOTOR_AUTHORITY_CONTROL;
    }

    if (authority->owner != MOTOR_AUTHORITY_CONTROL) {
        return false;
    }

    authority->last_output_percent = signed_percent;
    authority->control_last_update_ms = now_ms;
    authority->control_watchdog_started = true;

    if (authority->output != NULL) {
        authority->output(
            signed_percent,
            authority->output_context);
    }

    return true;
}

bool motor_authority_update_1ms(
    motor_authority_t *authority,
    uint32_t now_ms)
{
    uint32_t age_ms;

    if ((authority == NULL) ||
        (authority->owner != MOTOR_AUTHORITY_CONTROL) ||
        !authority->control_watchdog_started) {
        return false;
    }

    age_ms = now_ms - authority->control_last_update_ms;
    if (age_ms <= MOTOR_AUTHORITY_CONTROL_TIMEOUT_MS) {
        return false;
    }

    motor_authority_enter_fault(authority);
    return true;
}

void motor_authority_enter_fault(motor_authority_t *authority)
{
    if (authority == NULL) {
        return;
    }

    motor_authority_safe_off(authority);
    authority->control_enabled = false;
    authority->control_watchdog_started = false;
    authority->owner = MOTOR_AUTHORITY_FAULT;
}

bool motor_authority_clear_fault(motor_authority_t *authority)
{
    if ((authority == NULL) ||
        (authority->owner != MOTOR_AUTHORITY_FAULT)) {
        return false;
    }

    motor_authority_safe_off(authority);
    authority->control_enabled = false;
    authority->control_watchdog_started = false;
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
