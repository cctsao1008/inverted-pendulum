#include "board_led.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

/*
 * Initial bring-up assumption:
 * C03C user LED on PA4, active low.
 */
#define BOARD_LED_PORT GPIOA
#define BOARD_LED_PIN  GPIO4

void board_led_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_set_mode(
        BOARD_LED_PORT,
        GPIO_MODE_OUTPUT_2_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        BOARD_LED_PIN);

    gpio_set(BOARD_LED_PORT, BOARD_LED_PIN);
}

void board_led_toggle(void)
{
    gpio_toggle(BOARD_LED_PORT, BOARD_LED_PIN);
}
