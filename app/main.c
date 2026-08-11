#include <stdint.h>
#include <stdio.h>

#include "board_adc.h"
#include "board_clock.h"
#include "board_encoder.h"
#include "board_led.h"
#include "board_profile.h"
#include "board_time.h"
#include "board_uart.h"

#define CONTROL_FREQUENCY_HZ   1000U
#define TELEMETRY_DIVIDER      10U
#define LED_TOGGLE_TICKS       500U

int main(void)
{
    uint32_t last_tick;
    uint32_t telemetry_divider = 0U;
    uint32_t led_divider = 0U;

    board_clock_init();
    board_led_init();
    board_uart_init(115200U);
    board_adc_init();
    board_encoder_init();
    board_profile_init();
    board_time_init();

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n");
    printf("[BOOT] inverted-pendulum\n");
    printf("[MCU] STM32F103C8T6\n");
    printf("[CLK] system=%lu Hz\n",
           (unsigned long)board_clock_get_hz());
    printf("[LOOP] control=%lu Hz telemetry=100 Hz\n",
           (unsigned long)CONTROL_FREQUENCY_HZ);
    printf("[ADC] PA7 ADC1_IN7\n");
    printf("[ENC] PB6/PB7 TIM4 quadrature x4\n");
    printf("[SAFE] motor output not initialized\n");

    last_tick = board_time_ticks();

    while (1) {
        uint32_t current_tick = board_time_ticks();

        if (current_tick == last_tick) {
            continue;
        }

        /*
         * Process one sample for each elapsed tick. During this sensor-only
         * phase, an occasional UART delay does not command the motor.
         */
        while (last_tick != current_tick) {
            uint32_t timestamp_us;
            uint16_t adc_raw;
            int32_t encoder_count;

            last_tick++;

            board_profile_high();

            timestamp_us = board_time_micros();
            adc_raw = board_adc_read_pendulum_raw();
            encoder_count = board_encoder_get_count();

            board_profile_low();

            telemetry_divider++;
            led_divider++;

            if (telemetry_divider >= TELEMETRY_DIVIDER) {
                telemetry_divider = 0U;

                printf("[SENS] t_us=%lu adc=%u enc=%ld\n",
                       (unsigned long)timestamp_us,
                       (unsigned int)adc_raw,
                       (long)encoder_count);
            }

            if (led_divider >= LED_TOGGLE_TICKS) {
                led_divider = 0U;
                board_led_toggle();
            }
        }
    }
}
