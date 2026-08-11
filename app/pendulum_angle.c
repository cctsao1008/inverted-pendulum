#include "pendulum_angle.h"

#define PENDULUM_TWO_PI_F 6.28318530717958647692F
#define PENDULUM_HALF_REV_COUNTS \
    (PENDULUM_ADC_COUNTS_PER_REVOLUTION / 2U)

int16_t pendulum_angle_wrapped_counts(
    uint16_t adc_raw,
    uint16_t upright_adc,
    int8_t direction)
{
    int32_t delta =
        (int32_t)adc_raw - (int32_t)upright_adc;

    delta =
        (delta + (int32_t)PENDULUM_HALF_REV_COUNTS) &
        ((int32_t)PENDULUM_ADC_COUNTS_PER_REVOLUTION - 1);
    delta -= (int32_t)PENDULUM_HALF_REV_COUNTS;

    if (direction < 0) {
        delta = -delta;

        /* Keep the result in [-2048, 2047] after inversion. */
        if (delta == (int32_t)PENDULUM_HALF_REV_COUNTS) {
            delta = -(int32_t)PENDULUM_HALF_REV_COUNTS;
        }
    }

    return (int16_t)delta;
}

float pendulum_angle_radians(
    uint16_t adc_raw,
    uint16_t upright_adc,
    int8_t direction)
{
    int16_t counts = pendulum_angle_wrapped_counts(
        adc_raw,
        upright_adc,
        direction);

    return (float)counts *
           (PENDULUM_TWO_PI_F /
            (float)PENDULUM_ADC_COUNTS_PER_REVOLUTION);
}
