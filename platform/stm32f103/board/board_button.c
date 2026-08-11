#include "board_button.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define M_BUTTON_PORT GPIOA
#define M_BUTTON_PIN  GPIO3

void board_button_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_set_mode(
        M_BUTTON_PORT,
        GPIO_MODE_INPUT,
        GPIO_CNF_INPUT_PULL_UPDOWN,
        M_BUTTON_PIN);

    /* Select the STM32F1 internal pull-up through the GPIO output latch. */
    gpio_set(M_BUTTON_PORT, M_BUTTON_PIN);
}

bool board_button_is_m_pressed(void)
{
    return (gpio_get(M_BUTTON_PORT, M_BUTTON_PIN) == 0U);
}
