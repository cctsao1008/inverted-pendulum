#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "control_config.h"

typedef enum {
    SAFETY_STATE_DISABLED = 0,
    SAFETY_STATE_READY,
    SAFETY_STATE_CAPTURE,
    SAFETY_STATE_BALANCE,
    SAFETY_STATE_FAULT
} safety_state_t;

typedef enum {
    SAFETY_FAULT_NONE            = 0U,
    SAFETY_FAULT_ARGUMENT        = (1U << 0),
    SAFETY_FAULT_CONFIG          = (1U << 1),
    SAFETY_FAULT_SAMPLE_INVALID  = (1U << 2),
    SAFETY_FAULT_SAMPLE_TIMEOUT  = (1U << 3),
    SAFETY_FAULT_ESCAPE_ANGLE    = (1U << 4)
} safety_fault_t;

typedef struct {
    uint32_t capture_required_samples;
    uint32_t sample_timeout_us;
} safety_limits_t;

typedef struct {
    bool arm_request;
    bool clear_fault_request;
    bool sample_valid;

    float pendulum_angle_rad;
    uint32_t sample_age_us;
} safety_input_t;

typedef struct {
    safety_state_t state;
    uint32_t fault_flags;
    uint32_t capture_sample_count;
} safety_manager_t;

void safety_manager_init(safety_manager_t *manager);

bool safety_manager_step(
    safety_manager_t *manager,
    const control_config_t *config,
    const safety_limits_t *limits,
    const safety_input_t *input);

#endif
