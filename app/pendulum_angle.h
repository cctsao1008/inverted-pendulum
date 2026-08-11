#ifndef PENDULUM_ANGLE_H
#define PENDULUM_ANGLE_H

#include <stdint.h>

#define PENDULUM_ADC_COUNTS_PER_REVOLUTION 4096U

int16_t pendulum_angle_wrapped_counts(
    uint16_t adc_raw,
    uint16_t upright_adc,
    int8_t direction);

float pendulum_angle_radians(
    uint16_t adc_raw,
    uint16_t upright_adc,
    int8_t direction);

#endif
