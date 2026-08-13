#ifndef ESTIMATOR_DISPATCH_H
#define ESTIMATOR_DISPATCH_H

#include <stdbool.h>

#include "control_config.h"
#include "control_types.h"
#include "kalman_estimator.h"
#include "state_estimator.h"

typedef struct {
    state_estimator_t basic;
    kalman_estimator_t kalman;
} estimator_dispatch_t;

void estimator_dispatch_init(
    estimator_dispatch_t *dispatcher);

bool estimator_dispatch_step(
    estimator_dispatch_t *dispatcher,
    state_estimator_mode_t mode,
    const control_config_t *config,
    const sensor_data_t *sensor,
    control_state_t *state);

#endif
