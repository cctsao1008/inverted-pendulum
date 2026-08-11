#include <stdint.h>
#include <stdio.h>

#include "board_adc.h"
#include "board_button.h"
#include "board_clock.h"
#include "board_encoder.h"
#include "board_led.h"
#include "board_profile.h"
#include "board_time.h"
#include "board_uart.h"
#include "telemetry_toggle.h"

#define CONTROL_FREQUENCY_HZ   1000U
#define TELEMETRY_PERIOD_TICKS 100U
#define BUTTON_DEBOUNCE_TICKS  50U
#define LED_TOGGLE_TICKS       500U

int main(void)
{
    uint32_t last_tick;
    uint32_t telemetry_divider = 0U;
    uint32_t led_divider = 0U;
    telemetry_toggle_t telemetry_toggle;

    board_clock_init();
    board_led_init();
    board_button_init();
    board_uart_init(115200U);
    board_adc_init();
    board_encoder_init();
    board_profile_init();
    board_time_init();

    telemetry_toggle_init(
        &telemetry_toggle,
        board_button_is_m_pressed(),
        board_time_ticks());

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n");
    printf("[BOOT] inverted-pendulum\n");
    printf("[MCU] STM32F103C8T6\n");
    printf("[CLK] system=%lu Hz\n",
           (unsigned long)board_clock_get_hz());
    printf("[LOOP] control=%lu Hz\n",
           (unsigned long)CONTROL_FREQUENCY_HZ);
    printf("[ADC] PA7 ADC1_IN7\n");
    printf("[ENC] PB6/PB7 TIM4 quadrature x4\n");
    printf("[TELEM] M button PA3 toggles 10 Hz output; default=off\n");
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
            bool telemetry_changed;

            last_tick++;

            board_profile_high();

            timestamp_us = board_time_micros();
            adc_raw = board_adc_read_pendulum_raw();
            encoder_count = board_encoder_get_count();

            board_profile_low();

            telemetry_changed = telemetry_toggle_update(
                &telemetry_toggle,
                board_button_is_m_pressed(),
                last_tick,
                BUTTON_DEBOUNCE_TICKS);

            if (telemetry_changed) {
                telemetry_divider = 0U;
                printf("[TELEM] %s, period=100 ms\n",
                       telemetry_toggle_is_enabled(&telemetry_toggle)
                           ? "enabled"
                           : "disabled");
            }

            led_divider++;

            if (telemetry_toggle_is_enabled(&telemetry_toggle)) {
                telemetry_divider++;

                if (telemetry_divider >= TELEMETRY_PERIOD_TICKS) {
                    telemetry_divider = 0U;

                    printf("[SENS] t_us=%lu adc=%u enc=%ld\n",
                           (unsigned long)timestamp_us,
                           (unsigned int)adc_raw,
                           (long)encoder_count);
                }
            } else {
                telemetry_divider = 0U;
            }

            if (led_divider >= LED_TOGGLE_TICKS) {
                led_divider = 0U;
                board_led_toggle();
            }
        }
    }
}
