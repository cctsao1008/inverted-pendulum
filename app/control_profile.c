#include "control_profile.h"

#include <float.h>
#include <stddef.h>

void app_control_profile_init_observe_only(
    app_control_profile_t *profile,
    float sample_period_s)
{
    if (profile == NULL) {
        return;
    }

    control_config_init_invalid(&profile->control);
    profile->control.sample_period_s = sample_period_s;

    /*
     * Observe-only bring-up avoids hiding sensor timing behind an unverified
     * filter tuning choice. Controller gains intentionally remain invalid.
     */
    profile->control.rate_filter_alpha = 1.0F;

    control_runtime_config_init_defaults(&profile->runtime);
    profile->runtime.telemetry_enabled = true;
    profile->runtime.motor_output_enabled = false;

    /*
     * Observe-only state safety is structurally configured so diagnostics can
     * distinguish real sensor/estimator/timestamp faults from a missing
     * configuration. It deliberately makes no plant-envelope claim yet:
     * physical angle/rate limits are non-restrictive finite sentinels and the
     * state-safety sample-age timeout is disabled. Admission freshness is
     * instrumented separately. Active-control profiles must replace these
     * sentinels with measured, provenance-backed limits before motor binding.
     */
    state_safety_limits_init_unconfigured(&profile->state_safety);
    profile->state_safety.configured = true;
    profile->state_safety.max_sample_age_us = 0U;
    profile->state_safety.max_abs_pendulum_angle_rad = FLT_MAX;
    profile->state_safety.max_abs_arm_angle_rad = FLT_MAX;
    profile->state_safety.max_abs_pendulum_rate_rad_s = FLT_MAX;
    profile->state_safety.max_abs_arm_rate_rad_s = FLT_MAX;
}
