#include "control_config.h"

#include <float.h>
#include <stddef.h>

#define CONTROL_PI_F 3.14159265358979323846F

static bool control_value_is_finite(float value)
{
    return (value == value) &&
           (value <= FLT_MAX) &&
           (value >= -FLT_MAX);
}

void control_config_init_invalid(control_config_t *config)
{
    uint32_t index;

    if (config == NULL) {
        return;
    }

    config->sample_period_s = 0.0F;
    config->capture_angle_rad = 0.0F;
    config->escape_angle_rad = 0.0F;
    config->max_normalized_output = 0.0F;
    config->rate_filter_alpha = 0.0F;

    for (index = 0U; index < 4U; index++) {
        config->balance_gain[index] = 0.0F;
    }

    config->balance_gains_configured = false;
}

uint32_t control_config_validate(
    const control_config_t *config)
{
    uint32_t errors = CONTROL_CONFIG_OK;
    uint32_t index;

    if (config == NULL) {
        return CONTROL_CONFIG_ERROR_NULL;
    }

    if (!control_value_is_finite(config->sample_period_s) ||
        (config->sample_period_s <= 0.0F) ||
        (config->sample_period_s > 0.01F)) {
        errors |= CONTROL_CONFIG_ERROR_SAMPLE_TIME;
    }

    if (!control_value_is_finite(config->capture_angle_rad) ||
        !control_value_is_finite(config->escape_angle_rad) ||
        (config->capture_angle_rad <= 0.0F) ||
        (config->escape_angle_rad <=
         config->capture_angle_rad) ||
        (config->escape_angle_rad > CONTROL_PI_F)) {
        errors |= CONTROL_CONFIG_ERROR_ANGLE_WINDOW;
    }

    if (!control_value_is_finite(
            config->max_normalized_output) ||
        (config->max_normalized_output <= 0.0F) ||
        (config->max_normalized_output > 1.0F)) {
        errors |= CONTROL_CONFIG_ERROR_OUTPUT_LIMIT;
    }

    if (!control_value_is_finite(
            config->rate_filter_alpha) ||
        (config->rate_filter_alpha <= 0.0F) ||
        (config->rate_filter_alpha > 1.0F)) {
        errors |= CONTROL_CONFIG_ERROR_RATE_FILTER;
    }

    if (!config->balance_gains_configured) {
        errors |= CONTROL_CONFIG_ERROR_BALANCE_GAIN;
    } else {
        for (index = 0U; index < 4U; index++) {
            if (!control_value_is_finite(
                    config->balance_gain[index])) {
                errors |= CONTROL_CONFIG_ERROR_BALANCE_GAIN;
            }
        }
    }

    return errors;
}
