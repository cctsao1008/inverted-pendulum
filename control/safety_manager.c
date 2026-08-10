#include "safety_manager.h"

#include <float.h>
#include <stddef.h>

#define SAFETY_MAX_CAPTURE_SAMPLES 100000U
#define SAFETY_MAX_TIMEOUT_US      10000000U

static bool safety_value_is_finite(float value)
{
    return (value == value) &&
           (value <= FLT_MAX) &&
           (value >= -FLT_MAX);
}

static float safety_absf(float value)
{
    return (value < 0.0F) ? -value : value;
}

static bool safety_limits_are_valid(
    const safety_limits_t *limits)
{
    if (limits == NULL) {
        return false;
    }

    if ((limits->capture_required_samples == 0U) ||
        (limits->capture_required_samples >
         SAFETY_MAX_CAPTURE_SAMPLES)) {
        return false;
    }

    if ((limits->sample_timeout_us == 0U) ||
        (limits->sample_timeout_us >
         SAFETY_MAX_TIMEOUT_US)) {
        return false;
    }

    return true;
}

static void safety_latch_fault(
    safety_manager_t *manager,
    uint32_t fault)
{
    manager->fault_flags |= fault;
    manager->capture_sample_count = 0U;
    manager->state = SAFETY_STATE_FAULT;
}

static uint32_t safety_check_sample(
    const safety_limits_t *limits,
    const safety_input_t *input)
{
    if (!input->sample_valid ||
        !safety_value_is_finite(
            input->pendulum_angle_rad)) {
        return SAFETY_FAULT_SAMPLE_INVALID;
    }

    if (input->sample_age_us >
        limits->sample_timeout_us) {
        return SAFETY_FAULT_SAMPLE_TIMEOUT;
    }

    return SAFETY_FAULT_NONE;
}

void safety_manager_init(safety_manager_t *manager)
{
    if (manager == NULL) {
        return;
    }

    manager->state = SAFETY_STATE_DISABLED;
    manager->fault_flags = SAFETY_FAULT_NONE;
    manager->capture_sample_count = 0U;
}

bool safety_manager_step(
    safety_manager_t *manager,
    const control_config_t *config,
    const safety_limits_t *limits,
    const safety_input_t *input)
{
    uint32_t sample_fault;
    float angle_magnitude;

    if (manager == NULL) {
        return false;
    }

    if ((config == NULL) ||
        (limits == NULL) ||
        (input == NULL)) {
        safety_latch_fault(
            manager,
            SAFETY_FAULT_ARGUMENT);
        return false;
    }

    if ((control_config_validate(config) !=
         CONTROL_CONFIG_OK) ||
        !safety_limits_are_valid(limits)) {
        safety_latch_fault(
            manager,
            SAFETY_FAULT_CONFIG);
        return false;
    }

    /*
     * A latched fault can only be cleared while disarmed
     * and with an explicit clear request.
     */
    if (manager->state == SAFETY_STATE_FAULT) {
        if (!input->arm_request &&
            input->clear_fault_request) {
            safety_manager_init(manager);
        }

        return false;
    }

    /*
     * Disarming is always accepted immediately.
     * Sensor validity is not required while disabled.
     */
    if (!input->arm_request) {
        safety_manager_init(manager);
        return false;
    }

    sample_fault =
        safety_check_sample(limits, input);

    if (sample_fault != SAFETY_FAULT_NONE) {
        safety_latch_fault(manager, sample_fault);
        return false;
    }

    angle_magnitude =
        safety_absf(input->pendulum_angle_rad);

    switch (manager->state) {
    case SAFETY_STATE_DISABLED:
    case SAFETY_STATE_READY:
        if (angle_magnitude <=
            config->capture_angle_rad) {
            manager->state = SAFETY_STATE_CAPTURE;
            manager->capture_sample_count = 1U;

            if (manager->capture_sample_count >=
                limits->capture_required_samples) {
                manager->state =
                    SAFETY_STATE_BALANCE;
            }
        } else {
            manager->state = SAFETY_STATE_READY;
            manager->capture_sample_count = 0U;
        }
        break;

    case SAFETY_STATE_CAPTURE:
        if (angle_magnitude >
            config->capture_angle_rad) {
            manager->state = SAFETY_STATE_READY;
            manager->capture_sample_count = 0U;
            break;
        }

        if (manager->capture_sample_count <
            limits->capture_required_samples) {
            manager->capture_sample_count++;
        }

        if (manager->capture_sample_count >=
            limits->capture_required_samples) {
            manager->state = SAFETY_STATE_BALANCE;
        }
        break;

    case SAFETY_STATE_BALANCE:
        if (angle_magnitude >
            config->escape_angle_rad) {
            safety_latch_fault(
                manager,
                SAFETY_FAULT_ESCAPE_ANGLE);
            return false;
        }
        break;

    case SAFETY_STATE_FAULT:
    default:
        safety_latch_fault(
            manager,
            SAFETY_FAULT_ARGUMENT);
        return false;
    }

    return manager->state == SAFETY_STATE_BALANCE;
}
