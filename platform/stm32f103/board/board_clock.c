#include "board_clock.h"

#include <libopencm3/stm32/rcc.h>

void board_clock_init(void)
{
    rcc_clock_setup_pll(
        &rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

uint32_t board_clock_get_hz(void)
{
    return 72000000U;
}
