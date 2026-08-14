#ifndef MOTOR_AUTHORITY_H
#define MOTOR_AUTHORITY_H

#include <stdbool.h>
#include <stdint.h>

#define MOTOR_AUTHORITY_CONTROL_TIMEOUT_MS 5U

typedef enum {
    MOTOR_AUTHORITY_NONE = 0,
    MOTOR_AUTHORITY_MAINTENANCE,
    MOTOR_AUTHORITY_CONTROL,
    MOTOR_AUTHORITY_FAULT
} motor_authority_owner_t;

typedef void (*motor_authority_output_fn)(
    int8_t signed_percent,
    void *context);

typedef struct {
    motor_authority_owner_t owner;
    motor_authority_output_fn output;
    void *output_context;
    int8_t last_output_percent;
    uint32_t control_last_update_ms;
    bool control_enabled;
    bool control_watchdog_started;
} motor_authority_t;

void motor_authority_init(
    motor_authority_t *authority,
    motor_authority_output_fn output,
    void *output_context);

bool motor_authority_acquire(
    motor_authority_t *authority,
    motor_authority_owner_t owner);

void motor_authority_release(
    motor_authority_t *authority,
    motor_authority_owner_t owner);

bool motor_authority_apply(
    motor_authority_t *authority,
    motor_authority_owner_t owner,
    int8_t signed_percent);

bool motor_authority_set_control_enabled(
    motor_authority_t *authority,
    bool enabled);

bool motor_authority_control_enabled(
    const motor_authority_t *authority);

bool motor_authority_control_command(
    motor_authority_t *authority,
    int8_t signed_percent,
    uint32_t now_ms);

bool motor_authority_update_1ms(
    motor_authority_t *authority,
    uint32_t now_ms);

void motor_authority_enter_fault(motor_authority_t *authority);
bool motor_authority_clear_fault(motor_authority_t *authority);

motor_authority_owner_t motor_authority_owner(
    const motor_authority_t *authority);

int8_t motor_authority_last_output(
    const motor_authority_t *authority);

const char *motor_authority_owner_name(
    motor_authority_owner_t owner);

#endif
