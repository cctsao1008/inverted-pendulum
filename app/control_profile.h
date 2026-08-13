#ifndef APP_CONTROL_PROFILE_H
#define APP_CONTROL_PROFILE_H

#include "control_config.h"
#include "control_runtime.h"

typedef struct {
    control_config_t control;
    control_runtime_config_t runtime;
} app_control_profile_t;

void app_control_profile_init_observe_only(
    app_control_profile_t *profile,
    float sample_period_s);

#endif
