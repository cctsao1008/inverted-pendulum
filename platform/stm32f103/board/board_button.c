#include "board_button.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define KEY_PORT GPIOA

static uint16_t key_pin(board_key_t key)
{
    switch (key) {
    case BOARD_KEY_M:
        return GPIO3;
    case BOARD_KEY_X:
        return GPIO2;
    case BOARD_KEY_PLUS:
        return GPIO11;
    case BOARD_KEY_MINUS:
        return GPIO12;
    case BOARD_KEY_USER:
        return GPIO5;
    case BOARD_KEY_COUNT:
    default:
        return 0U;
    }
}

void board_button_init(void)
{
    const uint16_t pins =
        GPIO2 | GPIO3 | GPIO5 | GPIO11 | GPIO12;

    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_set_mode(
        KEY_PORT,
        GPIO_MODE_INPUT,
        GPIO_CNF_INPUT_PULL_UPDOWN,
        pins);

    /* Select internal pull-ups. The board also provides 10 kOhm pull-ups. */
    gpio_set(KEY_PORT, pins);
}

bool board_key_is_pressed(board_key_t key)
{
    uint16_t pin = key_pin(key);

    if (pin == 0U) {
        return false;
    }

    return (gpio_get(KEY_PORT, pin) == 0U);
}

uint32_t board_keys_pressed_mask(void)
{
    uint32_t mask = 0U;
    board_key_t key;

    for (key = BOARD_KEY_M; key < BOARD_KEY_COUNT; key++) {
        if (board_key_is_pressed(key)) {
            mask |= BOARD_KEY_MASK(key);
        }
    }

    return mask;
}

bool board_button_is_m_pressed(void)
{
    return board_key_is_pressed(BOARD_KEY_M);
}
