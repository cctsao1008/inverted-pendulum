#include <assert.h>
#include <float.h>

#include "control_profile.h"

int main(void)
{
    app_control_profile_t profile;

    app_control_profile_init_observe_only(
        &profile,
        0.001F);

    assert(control_config_validate_estimator(
        &profile.control) == CONTROL_CONFIG_OK);
    assert((control_config_validate(&profile.control) &
            CONTROL_CONFIG_ERROR_BALANCE_GAIN) != 0U);
    assert(!profile.control.balance_gains_configured);

    assert(control_runtime_config_validate(
        &profile.runtime) == CONTROL_RUNTIME_CONFIG_OK);
    assert(profile.runtime.estimator_mode ==
        STATE_ESTIMATOR_BASIC);
    assert(profile.runtime.balance_controller ==
        BALANCE_CONTROLLER_LQR);
    assert(!profile.runtime.motor_output_enabled);
    assert(profile.runtime.telemetry_enabled);
    assert(!profile.runtime.swing_up_enabled);
    assert(!profile.runtime.capture_enabled);

    assert(profile.state_safety.configured);
    assert(profile.state_safety.max_sample_age_us == 0U);
    assert(profile.state_safety.max_abs_pendulum_angle_rad == FLT_MAX);
    assert(profile.state_safety.max_abs_arm_angle_rad == FLT_MAX);
    assert(profile.state_safety.max_abs_pendulum_rate_rad_s == FLT_MAX);
    assert(profile.state_safety.max_abs_arm_rate_rad_s == FLT_MAX);

    return 0;
}
