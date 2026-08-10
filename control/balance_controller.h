#ifndef BALANCE_CONTROLLER_H
#define BALANCE_CONTROLLER_H

#include "control_config.h"
#include "control_types.h"

typedef enum {
    BALANCE_CONTROLLER_RESULT_OK = 0,
    BALANCE_CONTROLLER_ERROR_ARGUMENT,
    BALANCE_CONTROLLER_ERROR_CONFIG,
    BALANCE_CONTROLLER_ERROR_STATE,
    BALANCE_CONTROLLER_ERROR_NUMERIC
} balance_controller_result_t;

/*
 * Computes:
 *
 *     u = -Kx
 *
 * State order:
 *     x[0] = pendulum angle
 *     x[1] = pendulum angular rate
 *     x[2] = arm angle
 *     x[3] = arm angular rate
 *
 * On any error, normalized_output is set to zero whenever
 * the output pointer is valid.
 */
balance_controller_result_t balance_controller_compute(
    const control_config_t *config,
    const control_state_t *state,
    float *normalized_output);

#endif
