#include "board_oled.h"

#include "board_time.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define OLED_CLOCK_PORT GPIOB
#define OLED_CLOCK_PIN  GPIO5
#define OLED_DATA_PORT  GPIOB
#define OLED_DATA_PIN   GPIO4
#define OLED_RESET_PORT GPIOB
#define OLED_RESET_PIN  GPIO3
#define OLED_DC_PORT    GPIOA
#define OLED_DC_PIN     GPIO15

static void delay_milliseconds(uint32_t milliseconds)
{
    uint32_t start = board_time_ticks();

    while ((uint32_t)(board_time_ticks() - start) < milliseconds) {
        /* Busy wait only during OLED reset at startup. */
    }
}

static void write_byte(uint8_t value)
{
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++) {
        gpio_clear(OLED_CLOCK_PORT, OLED_CLOCK_PIN);

        if ((value & 0x80U) != 0U) {
            gpio_set(OLED_DATA_PORT, OLED_DATA_PIN);
        } else {
            gpio_clear(OLED_DATA_PORT, OLED_DATA_PIN);
        }

        gpio_set(OLED_CLOCK_PORT, OLED_CLOCK_PIN);
        value <<= 1;
    }

    gpio_clear(OLED_CLOCK_PORT, OLED_CLOCK_PIN);
}

void board_oled_init(void)
{
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    /* Reclaim PA15/PB3/PB4 from JTAG while keeping SWD on PA13/PA14. */
    gpio_primary_remap(
        AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON,
        0U);

    gpio_set_mode(
        GPIOB,
        GPIO_MODE_OUTPUT_2_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        OLED_CLOCK_PIN | OLED_DATA_PIN | OLED_RESET_PIN);
    gpio_set_mode(
        GPIOA,
        GPIO_MODE_OUTPUT_2_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        OLED_DC_PIN);

    gpio_clear(OLED_CLOCK_PORT, OLED_CLOCK_PIN);
    gpio_clear(OLED_DATA_PORT, OLED_DATA_PIN);
    gpio_clear(OLED_DC_PORT, OLED_DC_PIN);
    gpio_set(OLED_RESET_PORT, OLED_RESET_PIN);
}

void board_oled_reset(void *context)
{
    (void)context;

    /* Match the known-working Forest vendor firmware reset sequence. */
    gpio_clear(OLED_RESET_PORT, OLED_RESET_PIN);
    delay_milliseconds(100U);
    gpio_set(OLED_RESET_PORT, OLED_RESET_PIN);
    delay_milliseconds(10U);
}

void board_oled_self_test(void)
{
    /*
     * SSD1315 has no readback on this board.  Exercise only controller
     * command transport here: force every segment on, blank the panel, then
     * return to RAM-driven display mode before the normal UI starts.
     */
    board_oled_write_command(0xA5U, NULL);
    board_oled_write_command(0xAFU, NULL);
    delay_milliseconds(500U);
    board_oled_write_command(0xAEU, NULL);
    delay_milliseconds(250U);
    board_oled_write_command(0xAFU, NULL);
    board_oled_write_command(0xA4U, NULL);
    delay_milliseconds(250U);
}

void board_oled_write_command(
    uint8_t command,
    void *context)
{
    (void)context;

    gpio_clear(OLED_DC_PORT, OLED_DC_PIN);
    write_byte(command);
}

void board_oled_write_data(
    const uint8_t *data,
    size_t length,
    void *context)
{
    size_t index;

    (void)context;

    if (data == NULL) {
        return;
    }

    gpio_set(OLED_DC_PORT, OLED_DC_PIN);
    for (index = 0U; index < length; index++) {
        write_byte(data[index]);
    }
}
