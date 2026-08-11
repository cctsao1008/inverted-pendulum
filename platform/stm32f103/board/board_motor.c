#include "board_motor.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#define MOTOR_PWM_PERIOD_TICKS 3600U

static board_motor_channel_t selected_channel = BOARD_MOTOR_CHANNEL_D2;

void board_motor_stop(void)
{
    /* Stop both channels so a selection change cannot leave stale output. */
    timer_set_oc_value(TIM3, TIM_OC3, 0U);
    timer_set_oc_value(TIM3, TIM_OC4, 0U);
    gpio_clear(GPIOB, GPIO12 | GPIO13 | GPIO14 | GPIO15);
}

void board_motor_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_TIM3);

    /* Establish the safe GPIO levels before enabling output modes. */
    gpio_clear(GPIOB, GPIO0 | GPIO1 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
    gpio_set_mode(
        GPIOB,
        GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_PUSHPULL,
        GPIO12 | GPIO13 | GPIO14 | GPIO15);
    gpio_set_mode(
        GPIOB,
        GPIO_MODE_OUTPUT_50_MHZ,
        GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
        GPIO0 | GPIO1);

    /* APB1 timer clock is 72 MHz: 72 MHz / 3600 = 20 kHz. */
    rcc_periph_reset_pulse(RST_TIM3);
    timer_set_prescaler(TIM3, 0U);
    timer_set_period(TIM3, MOTOR_PWM_PERIOD_TICKS - 1U);
    timer_set_oc_mode(TIM3, TIM_OC3, TIM_OCM_PWM1);
    timer_set_oc_mode(TIM3, TIM_OC4, TIM_OCM_PWM1);
    timer_set_oc_value(TIM3, TIM_OC3, 0U);
    timer_set_oc_value(TIM3, TIM_OC4, 0U);
    timer_enable_oc_preload(TIM3, TIM_OC3);
    timer_enable_oc_preload(TIM3, TIM_OC4);
    timer_enable_preload(TIM3);
    timer_enable_oc_output(TIM3, TIM_OC3);
    timer_enable_oc_output(TIM3, TIM_OC4);
    timer_generate_event(TIM3, TIM_EGR_UG);
    timer_enable_counter(TIM3);

    board_motor_stop();
}

void board_motor_select_channel(board_motor_channel_t channel)
{
    board_motor_stop();

    if (channel == BOARD_MOTOR_CHANNEL_D1) {
        selected_channel = BOARD_MOTOR_CHANNEL_D1;
    } else {
        selected_channel = BOARD_MOTOR_CHANNEL_D2;
    }
}

board_motor_channel_t board_motor_get_channel(void)
{
    return selected_channel;
}

void board_motor_set_percent(int8_t signed_percent)
{
    uint32_t magnitude;
    uint32_t compare;

    if (signed_percent > 100) {
        signed_percent = 100;
    } else if (signed_percent < -100) {
        signed_percent = -100;
    }

    if (signed_percent == 0) {
        board_motor_stop();
        return;
    }

    magnitude = (signed_percent < 0)
        ? (uint32_t)(-(int32_t)signed_percent)
        : (uint32_t)signed_percent;
    compare = (MOTOR_PWM_PERIOD_TICKS * magnitude) / 100U;

    if (selected_channel == BOARD_MOTOR_CHANNEL_D1) {
        if (signed_percent > 0) {
            gpio_clear(GPIOB, GPIO15);
            gpio_set(GPIOB, GPIO14);
        } else {
            gpio_clear(GPIOB, GPIO14);
            gpio_set(GPIOB, GPIO15);
        }

        timer_set_oc_value(TIM3, TIM_OC3, compare);
    } else {
        if (signed_percent > 0) {
            gpio_clear(GPIOB, GPIO12);
            gpio_set(GPIOB, GPIO13);
        } else {
            gpio_clear(GPIOB, GPIO13);
            gpio_set(GPIOB, GPIO12);
        }

        timer_set_oc_value(TIM3, TIM_OC4, compare);
    }
}
