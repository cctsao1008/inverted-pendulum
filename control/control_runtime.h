#ifndef CONTROL_RUNTIME_H
#define CONTROL_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "control_types.h"

typedef struct {
    state_estimator_mode_t estimator_mode;
    balance_controller_mode_t balance_controller;

    bool swing_up_enabled;
    bool capture_enabled;
    bool motor_output_enabled;
    bool telemetry_enabled;
} control_runtime_config_t;

typedef enum {
    CONTROL_RUNTIME_CONFIG_OK = 0U,
    CONTROL_RUNTIME_CONFIG_ERROR_ARGUMENT = (1U << 0),
    CONTROL_RUNTIME_CONFIG_ERROR_ESTIMATOR = (1U << 1),
    CONTROL_RUNTIME_CONFIG_ERROR_BALANCE = (1U << 2),
    CONTROL_RUNTIME_CONFIG_ERROR_SWING_UP = (1U << 3),
    CONTROL_RUNTIME_CONFIG_ERROR_CAPTURE = (1U << 4)
} control_runtime_config_error_t;

void control_runtime_config_init_defaults(
    control_runtime_config_t *config);

/*
 * Runtime validation accepts only capabilities with a real implementation.
 * Stubs remain linkable architecture placeholders but are not selectable as
 * supported runtime behavior until their implementation is complete.
 */
uint32_t control_runtime_config_validate(
    const control_runtime_config_t *config);

#endif
