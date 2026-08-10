#include "board_profile.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

/*
 * PB8 is only a provisional profiling pin.
 * Keep disabled until its electrical use is confirmed on the actual board.
 */
#define BOARD_PROFILE_ENABLE 0

void board_profile_init(void)
{
#if BOARD_PROFILE_ENABLE
    rcc_periph_clock_enable(RCC_GPIOB);

    gpio_set_mode(
        GPIOB,
        GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        GPIO8);

    gpio_clear(GPIOB, GPIO8);
#endif
}

void board_profile_high(void)
{
#if BOARD_PROFILE_ENABLE
    gpio_set(GPIOB, GPIO8);
#endif
}

void board_profile_low(void)
{
#if BOARD_PROFILE_ENABLE
    gpio_clear(GPIOB, GPIO8);
#endif
}
