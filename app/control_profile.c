#include "control_profile.h"

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
}
