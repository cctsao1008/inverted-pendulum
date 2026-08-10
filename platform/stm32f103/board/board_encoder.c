#include "board_encoder.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

void board_encoder_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_TIM4);

    /*
     * PB6 = TIM4_CH1
     * PB7 = TIM4_CH2
     */
    gpio_set_mode(
        GPIOB,
        GPIO_MODE_INPUT,
        GPIO_CNF_INPUT_FLOAT,
        GPIO6 | GPIO7);

    rcc_periph_reset_pulse(RST_TIM4);
    timer_set_period(TIM4, 0xFFFFU);

    timer_ic_set_input(TIM4, TIM_IC1, TIM_IC_IN_TI1);
    timer_ic_set_input(TIM4, TIM_IC2, TIM_IC_IN_TI2);

    /*
     * Encoder mode 3: count on both TI1 and TI2 edges (quadrature x4).
     */
    timer_slave_set_mode(TIM4, TIM_SMCR_SMS_EM3);
    timer_set_counter(TIM4, 0U);
    timer_enable_counter(TIM4);
}

int32_t board_encoder_get_count(void)
{
    /*
     * Signed 16-bit count is sufficient for initial arm movement tests.
     * A 32-bit software extension can be added later if required.
     */
    return (int32_t)(int16_t)timer_get_counter(TIM4);
}
