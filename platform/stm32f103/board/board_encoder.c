#include "board_encoder.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

void board_encoder_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);

    /*
     * PA0 = TIM2_CH1
     * PA1 = TIM2_CH2
     * These are the A0/A1 signals on the assembled six-pin connector.
     */
    gpio_set_mode(
        GPIOA,
        GPIO_MODE_INPUT,
        GPIO_CNF_INPUT_FLOAT,
        GPIO0 | GPIO1);

    rcc_periph_reset_pulse(RST_TIM2);
    timer_set_period(TIM2, 0xFFFFU);

    timer_ic_set_input(TIM2, TIM_IC1, TIM_IC_IN_TI1);
    timer_ic_set_input(TIM2, TIM_IC2, TIM_IC_IN_TI2);

    /* Match the vendor TIM2 encoder example's digital input filter value 10. */
    timer_ic_set_filter(TIM2, TIM_IC1, TIM_IC_DTF_DIV_16_N_5);
    timer_ic_set_filter(TIM2, TIM_IC2, TIM_IC_DTF_DIV_16_N_5);

    /*
     * Encoder mode 3: count on both TI1 and TI2 edges (quadrature x4).
     */
    timer_slave_set_mode(TIM2, TIM_SMCR_SMS_EM3);
    timer_set_counter(TIM2, 0U);
    timer_enable_counter(TIM2);
}

int32_t board_encoder_get_count(void)
{
    /*
     * Signed 16-bit count is sufficient for initial arm movement tests.
     * A 32-bit software extension can be added later if required.
     */
    return (int32_t)(int16_t)timer_get_counter(TIM2);
}
