#ifndef KALMAN_ESTIMATOR_H
#define KALMAN_ESTIMATOR_H

#include <stdbool.h>

#include "control_types.h"

typedef struct {
    bool initialized;
} kalman_estimator_t;

void kalman_estimator_init(
    kalman_estimator_t *estimator);

/*
 * Safe architecture stub.
 *
 * Returns false until a real Kalman implementation is installed. The state is
 * cleared rather than fabricated so downstream safety remains fail-closed.
 */
bool kalman_estimator_step(
    kalman_estimator_t *estimator,
    const sensor_data_t *sensor,
    control_state_t *state);

#endif
