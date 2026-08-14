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

/*
 * Half-period padding for the bit-banged clock.  Without it the loop toggles
 * SCLK at roughly 4 MHz on a 72 MHz core.  This link is write-only, so a
 * marginal edge cannot be detected in software; buy timing margin instead and
 * settle near 1 MHz, which is far below the controller's limit and tolerant of
 * ribbon-cable capacitance.
 */
#define OLED_CLOCK_PAD_LOOPS 6U

/*
 * Upper bound on the reset/settle spin below.  It exists so that an
 * init-order change that runs this before board_time_init() fails visibly
 * instead of deadlocking with no output.  At roughly ten cycles per iteration
 * this is about two seconds of 72 MHz core time, which no legitimate delay
 * here comes close to needing.
 */
#define OLED_DELAY_GUARD_LOOPS 20000000UL

static void clock_pad(void)
{
    volatile uint32_t count = OLED_CLOCK_PAD_LOOPS;

    while (count != 0U) {
        count--;
    }
}

static void delay_milliseconds(uint32_t milliseconds)
{
    uint32_t start = board_time_ticks();
    uint32_t guard = OLED_DELAY_GUARD_LOOPS;

    while ((uint32_t)(board_time_ticks() - start) < milliseconds) {
        if (guard == 0U) {
            /* SysTick is not advancing; give up rather than hang the boot. */
            return;
        }
        guard--;
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

        clock_pad();
        gpio_set(OLED_CLOCK_PORT, OLED_CLOCK_PIN);
        clock_pad();
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

    /*
     * Use the strongest output mode during bring-up and deliberately slow
     * software SPI above.  This maximizes edge and setup/hold margin without
     * claiming the previous GPIO mode was itself the root cause.
     */
    gpio_set_mode(
        GPIOB,
        GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        OLED_CLOCK_PIN | OLED_DATA_PIN | OLED_RESET_PIN);
    gpio_set_mode(
        GPIOA,
        GPIO_MODE_OUTPUT_50_MHZ,
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

void board_oled_delay_ms(
    uint32_t milliseconds,
    void *context)
{
    (void)context;

    delay_milliseconds(milliseconds);
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
