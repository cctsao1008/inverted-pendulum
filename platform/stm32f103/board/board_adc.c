#include "board_adc.h"

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define VBUS_ADC_CHANNEL ADC_CHANNEL6
#define PENDULUM_ADC_CHANNEL ADC_CHANNEL7

#define ADC_FULL_SCALE_COUNTS 4095U
#define ADC_REFERENCE_MV 3300U
#define VBUS_DIVIDER_RATIO 11U

static uint16_t board_adc_read_channel(uint8_t channel)
{
    uint8_t channel_sequence[1] = {channel};

    adc_set_regular_sequence(ADC1, 1U, channel_sequence);
    adc_start_conversion_regular(ADC1);

    while (!adc_eoc(ADC1)) {
    }

    return adc_read_regular(ADC1);
}

void board_adc_init(void)
{
    uint8_t channel_sequence[1] = {
        PENDULUM_ADC_CHANNEL
    };

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_ADC1);

    /* PA6 = VBUS sense; PA7 = pendulum angle. */
    gpio_set_mode(
        GPIOA,
        GPIO_MODE_INPUT,
        GPIO_CNF_INPUT_ANALOG,
        GPIO6 | GPIO7);

    /*
     * PCLK2 = 72 MHz; ADC clock = 72 / 6 = 12 MHz.
     * STM32F103 ADC maximum is 14 MHz.
     */
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6);

    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_set_sample_time(
        ADC1,
        PENDULUM_ADC_CHANNEL,
        ADC_SMPR_SMP_55DOT5CYC);
    adc_set_sample_time(
        ADC1,
        VBUS_ADC_CHANNEL,
        ADC_SMPR_SMP_55DOT5CYC);
    adc_set_regular_sequence(ADC1, 1U, channel_sequence);
    adc_enable_external_trigger_regular(
        ADC1,
        ADC_CR2_EXTSEL_SWSTART);

    adc_power_on(ADC1);

    for (volatile uint32_t delay = 0U; delay < 1000U; delay++) {
        __asm__ volatile ("nop");
    }

    adc_reset_calibration(ADC1);
    adc_calibrate(ADC1);
}

uint16_t board_adc_read_pendulum_raw(void)
{
    return board_adc_read_channel(PENDULUM_ADC_CHANNEL);
}

uint16_t board_adc_read_vbus_raw(void)
{
    return board_adc_read_channel(VBUS_ADC_CHANNEL);
}

uint32_t board_adc_vbus_raw_to_millivolts(uint16_t raw)
{
    /* Nominal conversion; calibrate against a trusted meter. */
    return (((uint32_t)raw * ADC_REFERENCE_MV * VBUS_DIVIDER_RATIO) +
            (ADC_FULL_SCALE_COUNTS / 2U)) /
           ADC_FULL_SCALE_COUNTS;
}

uint32_t board_adc_read_vbus_millivolts(void)
{
    return board_adc_vbus_raw_to_millivolts(
        board_adc_read_vbus_raw());
}
