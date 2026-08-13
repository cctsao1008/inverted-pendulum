#ifndef STATE_SAFETY_H
#define STATE_SAFETY_H

#include <stdbool.h>
#include <stdint.h>

#include "control_types.h"

typedef enum {
    STATE_SAFETY_FAULT_NONE =
        0U,
    STATE_SAFETY_FAULT_ARGUMENT =
        (1U << 0),
    STATE_SAFETY_FAULT_CONFIG =
        (1U << 1),
    STATE_SAFETY_FAULT_SENSOR_INVALID =
        (1U << 2),
    STATE_SAFETY_FAULT_ESTIMATE_NOT_READY =
        (1U << 3),
    STATE_SAFETY_FAULT_STATE_NONFINITE =
        (1U << 4),
    STATE_SAFETY_FAULT_TIMESTAMP =
        (1U << 5),
    STATE_SAFETY_FAULT_SAMPLE_TIMEOUT =
        (1U << 6),
    STATE_SAFETY_FAULT_PENDULUM_LIMIT =
        (1U << 7),
    STATE_SAFETY_FAULT_ARM_LIMIT =
        (1U << 8),
    STATE_SAFETY_FAULT_PENDULUM_RATE_LIMIT =
        (1U << 9),
    STATE_SAFETY_FAULT_ARM_RATE_LIMIT =
        (1U << 10)
} state_safety_fault_t;

typedef struct {
    bool configured;

    uint32_t max_sample_age_us;

    float max_abs_pendulum_angle_rad;
    float max_abs_arm_angle_rad;
    float max_abs_pendulum_rate_rad_s;
    float max_abs_arm_rate_rad_s;
} state_safety_limits_t;

typedef struct {
    uint32_t fault_flags;

    bool control_allowed;
    bool hard_fault;
} state_safety_result_t;

void state_safety_limits_init_unconfigured(
    state_safety_limits_t *limits);

state_safety_result_t state_safety_check(
    const control_state_t *state,
    const sensor_data_t *sensor,
    bool estimate_ready,
    const state_safety_limits_t *limits);

#endif
