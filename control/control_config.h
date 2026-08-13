#ifndef CONTROL_CONFIG_H
#define CONTROL_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    /*
     * All physical quantities use SI units.
     */
    float sample_period_s;
    float capture_angle_rad;
    float escape_angle_rad;
    float max_normalized_output;
    float rate_filter_alpha;

    /*
     * State order:
     * [pendulum angle,
     *  pendulum angular velocity,
     *  arm angle,
     *  arm angular velocity]
     */
    float balance_gain[4];
    bool balance_gains_configured;
} control_config_t;

typedef enum {
    CONTROL_CONFIG_OK                 = 0U,
    CONTROL_CONFIG_ERROR_NULL         = (1U << 0),
    CONTROL_CONFIG_ERROR_SAMPLE_TIME  = (1U << 1),
    CONTROL_CONFIG_ERROR_ANGLE_WINDOW = (1U << 2),
    CONTROL_CONFIG_ERROR_OUTPUT_LIMIT = (1U << 3),
    CONTROL_CONFIG_ERROR_BALANCE_GAIN = (1U << 4),
    CONTROL_CONFIG_ERROR_RATE_FILTER  = (1U << 5)
} control_config_error_t;

void control_config_init_invalid(control_config_t *config);

/*
 * Module-scoped validators prevent estimator, safety, and controller
 * implementations from depending on unrelated configuration fields.
 */
uint32_t control_config_validate_estimator(
    const control_config_t *config);

uint32_t control_config_validate_safety(
    const control_config_t *config);

uint32_t control_config_validate_balance(
    const control_config_t *config);

uint32_t control_config_validate(
    const control_config_t *config);

#endif
