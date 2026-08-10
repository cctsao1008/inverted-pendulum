#ifndef STATE_ESTIMATOR_H
#define STATE_ESTIMATOR_H

#include <stdbool.h>
#include <stdint.h>

#include "control_config.h"
#include "control_types.h"

typedef struct {
    float pendulum_angle_rad;
    float arm_angle_rad;
    uint32_t timestamp_us;
} state_estimator_input_t;

typedef enum {
    STATE_ESTIMATOR_RESULT_OK = 0,
    STATE_ESTIMATOR_RESULT_INITIALIZED,
    STATE_ESTIMATOR_ERROR_ARGUMENT,
    STATE_ESTIMATOR_ERROR_CONFIG,
    STATE_ESTIMATOR_ERROR_SAMPLE,
    STATE_ESTIMATOR_ERROR_TIMESTAMP
} state_estimator_result_t;

typedef struct {
    bool initialized;

    float previous_pendulum_angle_rad;
    float previous_arm_angle_rad;

    float pendulum_rate_rad_s;
    float arm_rate_rad_s;

    uint32_t previous_timestamp_us;
} state_estimator_t;

void state_estimator_init(state_estimator_t *estimator);

state_estimator_result_t state_estimator_step(
    state_estimator_t *estimator,
    const control_config_t *config,
    const state_estimator_input_t *input,
    control_state_t *state);

#endif
