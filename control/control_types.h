#ifndef CONTROL_TYPES_H
#define CONTROL_TYPES_H

#include <stdint.h>

typedef struct {
    float pendulum_angle_rad;
    float pendulum_rate_rad_s;
    float arm_angle_rad;
    float arm_rate_rad_s;
    uint32_t timestamp_us;
} control_state_t;

typedef struct {
    float normalized_command;
    uint8_t enable;
} control_output_t;

#endif
