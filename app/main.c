#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board_adc.h"
#include "board_button.h"
#include "board_clock.h"
#include "board_encoder.h"
#include "board_led.h"
#include "board_motor.h"
#include "board_oled.h"
#include "board_profile.h"
#include "board_time.h"
#include "board_uart.h"
#include "build_info.h"
#include "closed_loop_enable_gate.h"
#include "closed_loop_gate_runtime.h"
#include "command_service.h"
#include "control_pipeline.h"
#include "control_profile.h"
#include "control_sensor_adapter.h"
#include "key_service.h"
#include "local_ui.h"
#include "motor_authority.h"
#include "motor_test_service.h"
#include "pendulum_angle.h"
#include "runtime_parameters.h"
#include "ssd1315.h"
#include "telemetry_toggle.h"

#define CONTROL_FREQUENCY_HZ          1000U
#define LED_TOGGLE_TICKS               500U
#define OLED_UI_REFRESH_TICKS          100U
#define PERF_REPORT_TICKS             5000U
#define OLED_BACKGROUND_FLUSH_BYTES      8U

typedef struct {
    bool valid;
    control_trace_record_t latest;
} control_trace_capture_t;

typedef struct {
    bool pending;
    uint32_t timestamp_us;
    uint16_t adc_raw;
    int32_t angle_mrad;
    int32_t encoder_count;
    uint16_t vbus_raw;
    uint32_t vbus_mv;
    bool control_valid;
    control_mode_t control_mode;
    bool control_allowed;
    uint32_t fault_flags;
    int32_t theta_mrad;
    int32_t theta_rate_mrad_s;
    int32_t phi_mrad;
    int32_t phi_rate_mrad_s;
    uint32_t sample_age_us;
    uint32_t missed_cycles;
    bool gate_allowed;
    uint32_t gate_reject_flags;
} telemetry_report_t;

typedef struct {
    char data[256];
    uint16_t length;
} text_buffer_t;

static const char *control_mode_name(control_mode_t mode);

static void text_put_char(text_buffer_t *buffer, char value)
{
    if (buffer->length < (uint16_t)sizeof(buffer->data)) {
        buffer->data[buffer->length++] = value;
    }
}

static void text_put_string(text_buffer_t *buffer, const char *text)
{
    while ((text != NULL) && (*text != '\0')) {
        text_put_char(buffer, *text++);
    }
}

static void text_put_u32(text_buffer_t *buffer, uint32_t value)
{
    char digits[10];
    uint8_t count = 0U;

    do {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while ((value != 0U) && (count < sizeof(digits)));

    while (count != 0U) {
        text_put_char(buffer, digits[--count]);
    }
}

static void text_put_i32(text_buffer_t *buffer, int32_t value)
{
    uint32_t magnitude;

    if (value < 0) {
        text_put_char(buffer, '-');
        magnitude = (uint32_t)(-(value + 1)) + 1U;
    } else {
        magnitude = (uint32_t)value;
    }

    text_put_u32(buffer, magnitude);
}

static void text_put_hex32(text_buffer_t *buffer, uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    int8_t shift;

    for (shift = 28; shift >= 0; shift -= 4) {
        text_put_char(
            buffer,
            hex[(value >> (uint8_t)shift) & 0x0FU]);
    }
}

static void telemetry_write_report(const telemetry_report_t *report)
{
    text_buffer_t line = {{0}, 0U};

    text_put_string(&line, "[SENS] t_us=");
    text_put_u32(&line, report->timestamp_us);
    text_put_string(&line, " adc=");
    text_put_u32(&line, report->adc_raw);
    text_put_string(&line, " angle_mrad=");
    text_put_i32(&line, report->angle_mrad);
    text_put_string(&line, " enc=");
    text_put_i32(&line, report->encoder_count);
    text_put_string(&line, " vbus_adc=");
    text_put_u32(&line, report->vbus_raw);
    text_put_string(&line, " vbus_mV=");
    text_put_u32(&line, report->vbus_mv);
    text_put_char(&line, '\n');
    board_uart_write(line.data, line.length);

    if (!report->control_valid) {
        return;
    }

    line.length = 0U;
    text_put_string(&line, "[CTRL] mode=");
    text_put_string(&line, control_mode_name(report->control_mode));
    text_put_string(&line, " allowed=");
    text_put_u32(&line, report->control_allowed ? 1U : 0U);
    text_put_string(&line, " faults=0x");
    text_put_hex32(&line, report->fault_flags);
    text_put_string(&line, " theta_mrad=");
    text_put_i32(&line, report->theta_mrad);
    text_put_string(&line, " theta_rate_mrad_s=");
    text_put_i32(&line, report->theta_rate_mrad_s);
    text_put_string(&line, " phi_mrad=");
    text_put_i32(&line, report->phi_mrad);
    text_put_string(&line, " phi_rate_mrad_s=");
    text_put_i32(&line, report->phi_rate_mrad_s);
    text_put_string(&line, " sample_age_us=");
    text_put_u32(&line, report->sample_age_us);
    text_put_string(&line, " missed_cycles=");
    text_put_u32(&line, report->missed_cycles);
    text_put_string(&line, " gate_allowed=");
    text_put_u32(&line, report->gate_allowed ? 1U : 0U);
    text_put_string(&line, " gate_reject=0x");
    text_put_hex32(&line, report->gate_reject_flags);
    text_put_string(&line, " motor_sink=unbound\n");
    board_uart_write(line.data, line.length);
}

static void command_write(
    const char *text,
    void *context)
{
    (void)context;
    board_uart_write(text, (uint32_t)strlen(text));
}

static void motor_authority_board_output(
    int8_t signed_percent,
    void *context)
{
    (void)context;
    board_motor_set_percent(signed_percent);
}

static void motor_test_output(
    int8_t signed_percent,
    void *context)
{
    motor_authority_t *authority =
        (motor_authority_t *)context;

    if (authority == NULL) {
        return;
    }

    if (signed_percent != 0) {
        if (!motor_authority_acquire(
                authority,
                MOTOR_AUTHORITY_MAINTENANCE)) {
            return;
        }

        (void)motor_authority_apply(
            authority,
            MOTOR_AUTHORITY_MAINTENANCE,
            signed_percent);
        return;
    }

    if (motor_authority_owner(authority) ==
        MOTOR_AUTHORITY_MAINTENANCE) {
        (void)motor_authority_apply(
            authority,
            MOTOR_AUTHORITY_MAINTENANCE,
            0);
        motor_authority_release(
            authority,
            MOTOR_AUTHORITY_MAINTENANCE);
    }
}

static uint32_t read_vbus_millivolts(void *context)
{
    (void)context;
    return board_adc_read_vbus_millivolts();
}

static uint32_t read_monotonic_milliseconds(void *context)
{
    (void)context;
    return board_time_ticks();
}

static uint32_t read_monotonic_microseconds(void *context)
{
    (void)context;
    return board_time_micros();
}

static void capture_control_trace(
    const control_trace_record_t *record,
    void *context)
{
    control_trace_capture_t *capture =
        (control_trace_capture_t *)context;

    if ((record == NULL) || (capture == NULL)) {
        return;
    }

    capture->latest = *record;
    capture->valid = true;
}

static const char *control_mode_name(control_mode_t mode)
{
    switch (mode) {
    case CONTROL_MODE_DISABLED:
        return "disabled";
    case CONTROL_MODE_IDLE:
        return "idle";
    case CONTROL_MODE_SWING_UP:
        return "swing-up";
    case CONTROL_MODE_CAPTURE:
        return "capture";
    case CONTROL_MODE_BALANCE:
        return "balance";
    case CONTROL_MODE_FAULT:
    default:
        return "fault";
    }
}

static int32_t read_motor_encoder(void *context)
{
    (void)context;
    return board_encoder_get_count();
}

static const char *get_motor_channel(void *context)
{
    (void)context;
    return (board_motor_get_channel() == BOARD_MOTOR_CHANNEL_D1)
        ? "d1"
        : "d2";
}

static bool set_motor_channel(const char *channel, void *context)
{
    (void)context;

    if (strcmp(channel, "d1") == 0) {
        board_motor_select_channel(BOARD_MOTOR_CHANNEL_D1);
        return true;
    }

    if (strcmp(channel, "d2") == 0) {
        board_motor_select_channel(BOARD_MOTOR_CHANNEL_D2);
        return true;
    }

    return false;
}

static bool oled_command(
    command_oled_operation_t operation,
    uint8_t value,
    void *context)
{
    ssd1315_t *display = (ssd1315_t *)context;

    if (display == NULL) {
        return false;
    }

    switch (operation) {
    case COMMAND_OLED_DISPLAY_ON:
        ssd1315_send_command(display, SSD1315_CMD_DISPLAY_ON);
        break;
    case COMMAND_OLED_DISPLAY_OFF:
        ssd1315_send_command(display, SSD1315_CMD_DISPLAY_OFF);
        break;
    case COMMAND_OLED_ENTIRE_ON:
        ssd1315_send_command(display, SSD1315_CMD_ENTIRE_ON);
        break;
    case COMMAND_OLED_RESUME_RAM:
        ssd1315_send_command(display, SSD1315_CMD_RESUME_RAM);
        break;
    case COMMAND_OLED_INVERT_ON:
        ssd1315_send_command(display, SSD1315_CMD_INVERT_DISPLAY);
        break;
    case COMMAND_OLED_INVERT_OFF:
        ssd1315_send_command(display, SSD1315_CMD_NORMAL_DISPLAY);
        break;
    case COMMAND_OLED_CONTRAST:
        ssd1315_set_contrast(display, value);
        break;
    case COMMAND_OLED_IREF:
        ssd1315_set_iref(display, value);
        break;
    case COMMAND_OLED_VCOM:
        ssd1315_set_vcom(display, value);
        break;
    case COMMAND_OLED_CHARGE_PUMP:
        ssd1315_send_command_pair(
            display,
            SSD1315_CMD_CHARGE_PUMP,
            (value != 0U)
                ? SSD1315_CHARGE_PUMP_ON
                : SSD1315_CHARGE_PUMP_OFF);
        break;
    case COMMAND_OLED_REINIT:
        return ssd1315_reinit(display);
    case COMMAND_OLED_SELF_TEST:
        ssd1315_self_test(display);
        break;
    case COMMAND_OLED_PATTERN:
        ssd1315_test_pattern(display);
        break;
    case COMMAND_OLED_RAW:
    default:
        ssd1315_send_command(display, value);
        break;
    }

    return true;
}

static void oled_status(
    uint8_t *contrast,
    uint8_t *iref,
    uint8_t *vcom,
    void *context)
{
    const ssd1315_t *display = (const ssd1315_t *)context;

    *contrast = ssd1315_get_contrast(display);
    *iref = ssd1315_get_iref(display);
    *vcom = ssd1315_get_vcom(display);
}

static const char *characterize_phase_name(
    motor_characterize_state_t state)
{
    if (state == MOTOR_CHARACTERIZE_RAMP_UP) {
        return "ramp-up";
    }
    if (state == MOTOR_CHARACTERIZE_CONFIRM) {
        return "confirm";
    }
    if (state == MOTOR_CHARACTERIZE_RAMP_DOWN) {
        return "ramp-down";
    }
    return "idle";
}

int main(void)
{
    uint32_t last_tick;
    uint32_t telemetry_phase = 0U;
    uint32_t led_divider = 0U;
    uint32_t ui_divider = 0U;
    uint32_t perf_ticks = 0U;
    uint32_t perf_control_total_us = 0U;
    uint32_t perf_control_max_us = 0U;
    uint32_t perf_work_total_us = 0U;
    uint32_t perf_work_max_us = 0U;
    uint32_t perf_telemetry_total_us = 0U;
    uint32_t perf_telemetry_max_us = 0U;
    uint32_t perf_oled_total_us = 0U;
    uint32_t perf_oled_max_us = 0U;
    uint32_t perf_missed_base = 0U;
    uint32_t control_missed_cycles = 0U;
    bool control_runtime_ready;
    bool oled_ready;
    bool perf_report_due = false;
    uint32_t perf_report_control_avg_us = 0U;
    uint32_t perf_report_control_max_us = 0U;
    uint32_t perf_report_work_avg_us = 0U;
    uint32_t perf_report_work_max_us = 0U;
    uint32_t perf_report_telemetry_avg_us = 0U;
    uint32_t perf_report_telemetry_max_us = 0U;
    uint32_t perf_report_oled_avg_us = 0U;
    uint32_t perf_report_oled_max_us = 0U;
    uint32_t perf_report_missed = 0U;
    uint16_t last_control_upright_adc;
    int8_t last_control_pendulum_direction;
    int8_t last_characterize_percent = 0;
    motor_characterize_state_t last_characterize_state =
        MOTOR_CHARACTERIZE_IDLE;
    telemetry_toggle_t telemetry_toggle;
    runtime_parameters_t parameters;
    motor_authority_t motor_authority;
    motor_test_service_t motor_test;
    command_service_t command_service;
    key_service_t key_service;
    local_ui_t local_ui;
    control_pipeline_t control_pipeline;
    control_sensor_adapter_t control_sensor_adapter;
    app_control_profile_t control_profile;
    control_trace_capture_t control_trace_capture = {0};
    closed_loop_gate_config_t closed_loop_gate_config;
    closed_loop_gate_result_t closed_loop_gate_result = {
        false, CLOSED_LOOP_GATE_REJECT_CONFIG};
    telemetry_report_t telemetry_report = {0};
    static ssd1315_t oled;
    static local_ui_frame_t local_ui_frame;

    board_clock_init();
    board_led_init();
    board_motor_init();
    motor_authority_init(
        &motor_authority,
        motor_authority_board_output,
        NULL);
    board_button_init();
    board_uart_init(115200U);
    board_adc_init();
    board_encoder_init();
    board_profile_init();
    board_time_init();

    local_ui_init(&local_ui);
    board_oled_init();
    oled_ready = ssd1315_init(
        &oled,
        board_oled_reset,
        board_oled_delay_ms,
        board_oled_write_command,
        board_oled_write_data,
        NULL,
        local_ui_get_contrast(&local_ui));

    if (oled_ready) {
        ssd1315_self_test(&oled);
    }

    key_service_init(
        &key_service,
        (uint8_t)BOARD_KEY_COUNT,
        board_keys_pressed_mask(),
        board_time_ticks());
    telemetry_toggle_init(
        &telemetry_toggle,
        false,
        board_time_ticks());
    runtime_parameters_init_defaults(&parameters);
    last_control_upright_adc = parameters.pendulum_upright_adc;
    last_control_pendulum_direction = parameters.pendulum_direction;

    app_control_profile_init_observe_only(
        &control_profile,
        1.0F / (float)CONTROL_FREQUENCY_HZ);

    closed_loop_gate_config.max_sample_age_us =
        parameters.control_gate_max_sample_age_us;
    closed_loop_gate_config.max_abs_entry_theta_mrad =
        parameters.control_gate_max_entry_theta_mrad;

    control_sensor_adapter_init(
        &control_sensor_adapter,
        board_encoder_get_count(),
        MOTOR_ENCODER_COUNTS_PER_OUTPUT_REV,
        read_monotonic_microseconds,
        NULL);

    control_pipeline_init(&control_pipeline);
    control_runtime_ready =
        control_pipeline_set_control_config(
            &control_pipeline,
            &control_profile.control) &&
        control_pipeline_set_runtime_config(
            &control_pipeline,
            &control_profile.runtime) &&
        control_pipeline_set_state_safety_limits(
            &control_pipeline,
            &control_profile.state_safety);

    control_pipeline_set_sensor_source(
        &control_pipeline,
        control_sensor_adapter_read,
        &control_sensor_adapter);
    control_pipeline_set_trace_sink(
        &control_pipeline,
        capture_control_trace,
        &control_trace_capture);

    motor_test_service_init_with_encoder(
        &motor_test,
        motor_test_output,
        &motor_authority,
        read_monotonic_milliseconds,
        NULL,
        read_motor_encoder,
        NULL);
    command_service_init(
        &command_service,
        &parameters,
        &telemetry_toggle,
        &motor_test,
        read_vbus_millivolts,
        NULL,
        get_motor_channel,
        set_motor_channel,
        NULL,
        oled_command,
        oled_status,
        &oled,
        command_write,
        NULL);

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n");
    printf("[BOOT] inverted-pendulum\n");
    printf("[FW] version=%s git=%s\n",
           FW_VERSION,
           FW_GIT_DESCRIBE);
    printf("[BUILD] utc=%s toolchain=gcc-%s\n",
           FW_BUILD_UTC,
           __VERSION__);
    printf("[MCU] STM32F103C8T6\n");
    printf("[CLK] system=%lu Hz\n",
           (unsigned long)board_clock_get_hz());
    printf("[LOOP] control=%lu Hz\n",
           (unsigned long)CONTROL_FREQUENCY_HZ);
    printf(
        "[PERF] window_ms=%lu metric=scheduled-work/1ms "
        "idle-spin=excluded\n",
        (unsigned long)PERF_REPORT_TICKS);
    printf("[ADC] PA7 ADC1_IN7; VBUS PA6 ADC1_IN6 divider=11:1\n");
    printf("[VBUS] nominal_mV=%lu calibration=required\n",
           (unsigned long)board_adc_read_vbus_millivolts());
    printf("[ENC] PA0/PA1 TIM2 quadrature x4 filter=10; timebase=TIM4\n");
    printf("[CLI] type help; text maintenance mode; script buffer in RAM\n");
    printf("[ANGLE] upright_adc=%u direction=%d wrap=[-pi,+pi)\n",
           (unsigned int)parameters.pendulum_upright_adc,
           (int)parameters.pendulum_direction);
    printf("[KEY] M=PA3 X=PA2 +=PA11 -=PA12 USER=PA5 active-low\n");
    printf("[TELEM] USER button toggles output; default=off rate=%u Hz\n",
           (unsigned int)parameters.telemetry_rate_hz);
    /*
     * Report what the panel was told, never that it answered.  This link has
     * no readback, so any "ready" here would be a claim the firmware cannot
     * make.
     */
    printf("[OLED] SSD1315 128x64 soft-spi clk=PB5 data=PB4 reset=PB3 dc=PA15\n");
    printf(
        "[OLED] programmed contrast=0x%02X iref=0xAD:0x%02X vcom=0xDB:0x%02X "
        "pump=on; write-only bus, panel response unverifiable\n",
        (unsigned int)ssd1315_get_contrast(&oled),
        (unsigned int)ssd1315_get_iref(&oled),
        (unsigned int)ssd1315_get_vcom(&oled));
    printf(
        "[OLED] driver=%s self-test=%s; type 'oled help' to sweep settings\n",
        oled_ready ? "commands-sent" : "callback-error",
        oled_ready ? "entire-on/off/ram" : "skipped");
    printf(
        "[CONTROL] runtime=%s profile=observe-only motor_sink=unbound "
        "arm_home=boot-position\n",
        control_runtime_ready ? "ready" : "config-error");
    printf(
        "[CONTROL_GATE] mode=observe-only operator=disabled "
        "config=ready sample_age_us<=%lu entry_theta_mrad<=%ld "
        "allowed=0 motor_sink=unbound\n",
        (unsigned long)closed_loop_gate_config.max_sample_age_us,
        (long)closed_loop_gate_config.max_abs_entry_theta_mrad);
    printf(
        "[MOTOR_AUTH] owner=%s maintenance=arbiter control=unbound "
        "gate=disabled watchdog=%lums fault=latching\n",
        motor_authority_owner_name(
            motor_authority_owner(&motor_authority)),
        (unsigned long)MOTOR_AUTHORITY_CONTROL_TIMEOUT_MS);
    printf("[MOTOR] channel=d2 default; d1=PB0/CH3+PB14/PB15; d2=PB1/CH4+PB13/PB12; 20kHz\n");
    printf("[PATTERN] right=+15%%/5s left=-15%%/5s both=right/5s-stop/1s-left/5s\n");
    printf("[COMMISSION] characterize=5..30%% rise/2%% fall/1%%; "
           "encoder=1040 counts/rev; position loop disabled\n");
    printf("[RESPONSE] configurable=30..80%%/1000..10000ms; "
           "coast observation=5000ms\n");
    printf("[BRAKE] observer=1ms position/10ms fast/40ms slow; neutral=1ms reverse=10..20%% max=300ms settle=300ms\n");
    printf(
        "[SAFE] motor stopped; arm=30000ms test limit=%d%% max=%lums; "
        "type motor status\n",
        MOTOR_TEST_MAX_PERCENT,
        (unsigned long)MOTOR_TEST_MAX_DURATION_MS);

    last_tick = board_time_ticks();

    while (1) {
        uint32_t current_tick = board_time_ticks();
        uint32_t pending_ticks = current_tick - last_tick;
        char received_character;

        if (pending_ticks > 1U) {
            uint32_t missed = pending_ticks - 1U;

            if ((UINT32_MAX - control_missed_cycles) < missed) {
                control_missed_cycles = UINT32_MAX;
            } else {
                control_missed_cycles += missed;
            }
        }

        /*
         * Drain every tick that existed before accepting a new command. This
         * preserves maintenance-service timing, but the control pipeline runs
         * only for the newest real sample. Historical control cycles are
         * recorded as missed rather than replayed back-to-back.
         */
        while (last_tick != current_tick) {
            uint32_t timestamp_us;
            uint16_t adc_raw;
            int32_t encoder_count;
            float pendulum_angle_rad;
            bool control_cycle_due;
            bool motor_test_completed;
            uint32_t control_path_start_us = 0U;
            uint32_t tick_work_start_us = 0U;
            uint32_t control_path_us = 0U;
            uint32_t telemetry_work_us = 0U;
            uint32_t oled_work_us = 0U;

            last_tick++;
            control_cycle_due = (last_tick == current_tick);

            if (control_cycle_due) {
                tick_work_start_us = board_time_micros();
                control_path_start_us = tick_work_start_us;
            }

            board_profile_high();

            adc_raw = board_adc_read_pendulum_raw();
            encoder_count = board_encoder_get_count();
            timestamp_us = board_time_micros();
            pendulum_angle_rad = pendulum_angle_radians(
                adc_raw,
                parameters.pendulum_upright_adc,
                parameters.pendulum_direction);

            if (control_cycle_due && control_runtime_ready) {
                uint16_t vbus_raw = board_adc_read_vbus_raw();

                /*
                 * Calibration changes redefine theta. Re-seed estimator
                 * history before the first sample in the new coordinate.
                 */
                if ((parameters.pendulum_upright_adc !=
                     last_control_upright_adc) ||
                    (parameters.pendulum_direction !=
                     last_control_pendulum_direction)) {
                    control_runtime_ready =
                        control_pipeline_set_control_config(
                            &control_pipeline,
                            &control_profile.control);
                    last_control_upright_adc =
                        parameters.pendulum_upright_adc;
                    last_control_pendulum_direction =
                        parameters.pendulum_direction;
                }

                control_sensor_adapter_update(
                    &control_sensor_adapter,
                    timestamp_us,
                    adc_raw,
                    encoder_count,
                    vbus_raw,
                    board_adc_vbus_raw_to_millivolts(vbus_raw),
                    parameters.pendulum_upright_adc,
                    parameters.pendulum_direction);
                control_pipeline_step(&control_pipeline);
            }

            if (control_cycle_due) {
                closed_loop_gate_result =
                    app_closed_loop_gate_evaluate(
                        &closed_loop_gate_config,
                        parameters.control_enable_request,
                        control_runtime_ready,
                        &control_pipeline,
                        motor_authority_owner(&motor_authority));
            }

            board_profile_low();

            if (control_cycle_due) {
                control_path_us =
                    board_time_micros() - control_path_start_us;
            }

            (void)motor_authority_update_1ms(
                &motor_authority,
                last_tick);

            motor_test_completed =
                motor_test_service_update_1ms(&motor_test);

            {
                motor_characterize_state_t state =
                    motor_test_service_characterize_state(&motor_test);
                int8_t percent = motor_test_service_percent(&motor_test);

                if (((state == MOTOR_CHARACTERIZE_RAMP_UP) ||
                     (state == MOTOR_CHARACTERIZE_CONFIRM) ||
                     (state == MOTOR_CHARACTERIZE_RAMP_DOWN)) &&
                    ((state != last_characterize_state) ||
                     (percent != last_characterize_percent))) {
                    printf("[MOTOR] characterize phase=%s output_pct=%d\n",
                           characterize_phase_name(state),
                           (int)percent);
                }
                last_characterize_state = state;
                last_characterize_percent = percent;
            }

            if (motor_test_completed) {
                motor_identify_state_t identify_state =
                    motor_test_service_identify_state(&motor_test);
                motor_characterize_state_t characterize_state =
                    motor_test_service_characterize_state(&motor_test);
                motor_response_state_t response_state =
                    motor_test_service_response_state(&motor_test);
                motor_brake_state_t brake_state = motor_test_service_brake_state(&motor_test);

                if ((brake_state == MOTOR_BRAKE_DONE) ||
                    (brake_state == MOTOR_BRAKE_REVERSAL)) {
                    const char *stop_reason =
                        brake_state == MOTOR_BRAKE_DONE ? "stable" : "reversal";
                    const char *direction =
                        motor_test_service_brake_direction(&motor_test) > 0
                            ? "right" : "left";
                    int brake_drive_pct =
                        (int)motor_test_service_brake_drive_percent(&motor_test);
                    unsigned long brake_drive_ms =
                        (unsigned long)motor_test_service_brake_drive_ms(&motor_test);
                    int brake_pct =
                        (int)motor_test_service_brake_percent(&motor_test);
                    long drive_delta =
                        (long)motor_test_service_brake_drive_delta(&motor_test);
                    long cutoff_velocity =
                        (long)motor_test_service_brake_cutoff_velocity_counts_s(&motor_test);
                    long neutral_delta =
                        (long)motor_test_service_brake_neutral_delta(&motor_test);
                    long entry_velocity =
                        (long)motor_test_service_brake_entry_velocity_counts_s(&motor_test);
                    unsigned long brake_time_ms =
                        (unsigned long)motor_test_service_brake_time_ms(&motor_test);
                    long braking_delta =
                        (long)motor_test_service_brake_delta(&motor_test);
                    long release_velocity =
                        (long)motor_test_service_brake_release_velocity_counts_s(&motor_test);
                    long settling_delta =
                        (long)motor_test_service_brake_settle_delta(&motor_test);
                    long final_velocity =
                        (long)motor_test_service_brake_final_velocity_counts_s(&motor_test);
                    long peak_velocity =
                        (long)motor_test_service_brake_peak_velocity_counts_s(&motor_test);
                    unsigned long vbus_mV =
                        (unsigned long)board_adc_read_vbus_millivolts();

                    printf("[MOTOR] brake-response complete stop_reason=%s direction=%s drive_pct=%d drive_ms=%lu brake_pct=%d drive_delta=%ld cutoff_velocity_counts_s=%ld neutral_delta=%ld brake_entry_velocity_counts_s=%ld brake_time_ms=%lu braking_delta=%ld release_velocity_counts_s=%ld settling_delta=%ld final_velocity_counts_s=%ld peak_velocity_counts_s=%ld output_pct=0 armed=0 vbus_mV=%lu\n",
                           stop_reason, direction, brake_drive_pct,
                           brake_drive_ms, brake_pct, drive_delta,
                           cutoff_velocity, neutral_delta, entry_velocity,
                           brake_time_ms, braking_delta, release_velocity,
                           settling_delta, final_velocity, peak_velocity,
                           vbus_mV);
                    printf("[MOTOR_CSV] brake_response,%s,%s,%d,%lu,%d,%ld,%ld,%ld,%ld,%lu,%ld,%ld,%ld,%ld,%ld,%lu\n",
                           stop_reason, direction, brake_drive_pct,
                           brake_drive_ms, brake_pct, drive_delta,
                           cutoff_velocity, neutral_delta, entry_velocity,
                           brake_time_ms, braking_delta, release_velocity,
                           settling_delta, final_velocity, peak_velocity,
                           vbus_mV);
                } else if ((brake_state == MOTOR_BRAKE_NO_MOTION) || (brake_state == MOTOR_BRAKE_ENCODER_IMPLAUSIBLE) || (brake_state == MOTOR_BRAKE_TIMEOUT)) {
                    const char *reason = brake_state == MOTOR_BRAKE_NO_MOTION ? "no-motion" : (brake_state == MOTOR_BRAKE_TIMEOUT ? "brake-timeout" : "encoder-implausible");
                    printf("[MOTOR] brake-response failed reason=%s direction=%s brake_pct=%d drive_delta=%ld braking_delta=%ld output_pct=0 armed=0 vbus_mV=%lu\n", reason,
                           motor_test_service_brake_direction(&motor_test) > 0 ? "right" : "left",
                           (int)motor_test_service_brake_percent(&motor_test),
                           (long)motor_test_service_brake_drive_delta(&motor_test),
                           (long)motor_test_service_brake_delta(&motor_test),
                           (unsigned long)board_adc_read_vbus_millivolts());
                } else if (response_state == MOTOR_RESPONSE_DONE) {
                    printf("[MOTOR] response complete direction=%s "
                           "output_pct=%d drive_ms=%lu drive_delta=%ld "
                           "cutoff_velocity_counts_s=%ld "
                           "stop_time_ms=%lu coast_delta=%ld "
                           "peak_velocity_counts_s=%ld output_pct=0 "
                           "armed=0 vbus_mV=%lu\n",
                           motor_test_service_response_direction(&motor_test) > 0
                               ? "right" : "left",
                           (int)motor_test_service_response_percent(&motor_test),
                           (unsigned long)motor_test_service_response_drive_ms(&motor_test),
                           (long)motor_test_service_response_drive_delta(&motor_test),
                           (long)motor_test_service_response_cutoff_velocity_counts_s(&motor_test),
                           (unsigned long)motor_test_service_response_stop_ms(&motor_test),
                           (long)motor_test_service_response_coast_delta(&motor_test),
                           (long)motor_test_service_response_peak_velocity_counts_s(&motor_test),
                           (unsigned long)board_adc_read_vbus_millivolts());
                } else if ((response_state == MOTOR_RESPONSE_NO_MOTION) ||
                           (response_state == MOTOR_RESPONSE_ENCODER_IMPLAUSIBLE) ||
                           (response_state == MOTOR_RESPONSE_COAST_TIMEOUT)) {
                    const char *reason = "coast-timeout";
                    if (response_state == MOTOR_RESPONSE_NO_MOTION) {
                        reason = "no-motion";
                    } else if (response_state ==
                               MOTOR_RESPONSE_ENCODER_IMPLAUSIBLE) {
                        reason = "encoder-implausible";
                    }
                    printf("[MOTOR] response failed reason=%s direction=%s "
                           "output_pct=%d drive_delta=%ld coast_delta=%ld "
                           "peak_velocity_counts_s=%ld output_pct=0 armed=0 "
                           "vbus_mV=%lu\n",
                           reason,
                           motor_test_service_response_direction(&motor_test) > 0
                               ? "right" : "left",
                           (int)motor_test_service_response_percent(&motor_test),
                           (long)motor_test_service_response_drive_delta(&motor_test),
                           (long)motor_test_service_response_coast_delta(&motor_test),
                           (long)motor_test_service_response_peak_velocity_counts_s(&motor_test),
                           (unsigned long)board_adc_read_vbus_millivolts());
                } else if (characterize_state == MOTOR_CHARACTERIZE_DONE) {
                    printf("[MOTOR] characterize complete direction=%s "
                           "encoder_sign=%d breakaway_pct=%d "
                           "breakaway_velocity_counts_s=%ld "
                           "minimum_sustain_pct=%d dropout_pct=%d "
                           "peak_velocity_counts_s=%ld output_pct=0 "
                           "armed=0 vbus_mV=%lu\n",
                           motor_test_service_characterize_direction(
                               &motor_test) > 0 ? "right" : "left",
                           (int)motor_test_service_characterize_encoder_sign(
                               &motor_test),
                           (int)motor_test_service_characterize_breakaway_percent(
                               &motor_test),
                           (long)motor_test_service_characterize_breakaway_velocity_counts_s(
                               &motor_test),
                           (int)motor_test_service_characterize_minimum_sustain_percent(
                               &motor_test),
                           (int)motor_test_service_characterize_dropout_percent(
                               &motor_test),
                           (long)motor_test_service_characterize_peak_velocity_counts_s(
                               &motor_test),
                           (unsigned long)board_adc_read_vbus_millivolts());
                } else if ((characterize_state ==
                            MOTOR_CHARACTERIZE_NO_MOTION) ||
                           (characterize_state ==
                            MOTOR_CHARACTERIZE_WRONG_DIRECTION) ||
                           (characterize_state ==
                            MOTOR_CHARACTERIZE_ENCODER_IMPLAUSIBLE) ||
                           (characterize_state ==
                            MOTOR_CHARACTERIZE_TIMEOUT)) {
                    const char *reason = "timeout";

                    if (characterize_state ==
                        MOTOR_CHARACTERIZE_NO_MOTION) {
                        reason = "no-motion-or-unstable";
                    } else if (characterize_state ==
                               MOTOR_CHARACTERIZE_WRONG_DIRECTION) {
                        reason = "direction-reversal";
                    } else if (characterize_state ==
                               MOTOR_CHARACTERIZE_ENCODER_IMPLAUSIBLE) {
                        reason = "encoder-implausible";
                    }
                    printf("[MOTOR] characterize failed reason=%s "
                           "direction=%s peak_velocity_counts_s=%ld "
                           "output_pct=0 armed=0 vbus_mV=%lu\n",
                           reason,
                           motor_test_service_characterize_direction(
                               &motor_test) > 0 ? "right" : "left",
                           (long)motor_test_service_characterize_peak_velocity_counts_s(
                               &motor_test),
                           (unsigned long)board_adc_read_vbus_millivolts());
                } else if (identify_state == MOTOR_IDENTIFY_DONE) {
                    printf("[MOTOR] identify complete encoder_delta=%ld "
                           "motor_encoder_sign=%d peak_velocity_counts_s=%ld "
                           "output_pct=0 armed=0 vbus_mV=%lu\n",
                           (long)motor_test_service_identify_delta(&motor_test),
                           (int)motor_test_service_identify_sign(&motor_test),
                           (long)motor_test_service_identify_peak_velocity_counts_s(&motor_test),
                           (unsigned long)board_adc_read_vbus_millivolts());
                } else if (identify_state == MOTOR_IDENTIFY_NO_MOTION) {
                    printf("[MOTOR] identify failed reason=no-motion "
                           "encoder_delta=%ld output_pct=0 armed=0 vbus_mV=%lu\n",
                           (long)motor_test_service_identify_delta(&motor_test),
                           (unsigned long)board_adc_read_vbus_millivolts());
                } else {
                    printf("[MOTOR] operation complete output_pct=0 armed=0 vbus_mV=%lu\n",
                           (unsigned long)board_adc_read_vbus_millivolts());
                }
            }

            command_service_update_1ms(&command_service);

            if (control_cycle_due) {
                key_service_events_t key_events =
                    key_service_update(
                        &key_service,
                        board_keys_pressed_mask(),
                        last_tick);

                if ((key_events.pressed &
                     BOARD_KEY_MASK(BOARD_KEY_M)) != 0U) {
                    local_ui_next_page(&local_ui);
                }
                if ((key_events.pressed &
                     BOARD_KEY_MASK(BOARD_KEY_X)) != 0U) {
                    local_ui_previous_page(&local_ui);
                }
                if (((key_events.pressed | key_events.repeat) &
                     BOARD_KEY_MASK(BOARD_KEY_PLUS)) != 0U) {
                    uint8_t contrast =
                        local_ui_increase_contrast(&local_ui);
                    if (oled_ready) {
                        ssd1315_set_contrast(&oled, contrast);
                    }
                }
                if (((key_events.pressed | key_events.repeat) &
                     BOARD_KEY_MASK(BOARD_KEY_MINUS)) != 0U) {
                    uint8_t contrast =
                        local_ui_decrease_contrast(&local_ui);
                    if (oled_ready) {
                        ssd1315_set_contrast(&oled, contrast);
                    }
                }
                if ((key_events.pressed &
                     BOARD_KEY_MASK(BOARD_KEY_USER)) != 0U) {
                    (void)telemetry_toggle_toggle(&telemetry_toggle);
                    telemetry_phase = 0U;
                    printf("[TELEM] %s rate=%u Hz source=user-key\n",
                           telemetry_toggle_is_enabled(&telemetry_toggle)
                               ? "enabled"
                               : "disabled",
                           (unsigned int)parameters.telemetry_rate_hz);
                }
            }

            led_divider++;

            if (control_cycle_due) {
                telemetry_work_us = board_time_micros();
            }

            if (telemetry_toggle_is_enabled(&telemetry_toggle)) {
                telemetry_phase += parameters.telemetry_rate_hz;

                if (telemetry_phase >= CONTROL_FREQUENCY_HZ) {
                    uint16_t vbus_raw;
                    uint32_t vbus_mv;

                    telemetry_phase -= CONTROL_FREQUENCY_HZ;

                    vbus_raw = board_adc_read_vbus_raw();
                    vbus_mv =
                        board_adc_vbus_raw_to_millivolts(vbus_raw);

                    telemetry_report.timestamp_us = timestamp_us;
                    telemetry_report.adc_raw = adc_raw;
                    telemetry_report.angle_mrad =
                        (int32_t)(pendulum_angle_rad * 1000.0F);
                    telemetry_report.encoder_count = encoder_count;
                    telemetry_report.vbus_raw = vbus_raw;
                    telemetry_report.vbus_mv = vbus_mv;
                    telemetry_report.control_valid =
                        control_cycle_due &&
                        control_trace_capture.valid;

                    if (telemetry_report.control_valid) {
                        const control_trace_record_t *record =
                            &control_trace_capture.latest;
                        telemetry_report.control_mode =
                            record->control_mode;
                        telemetry_report.control_allowed =
                            record->control_allowed;
                        telemetry_report.fault_flags =
                            record->state_safety_flags;
                        telemetry_report.theta_mrad = (int32_t)(
                            record->state.pendulum_angle_rad * 1000.0F);
                        telemetry_report.theta_rate_mrad_s = (int32_t)(
                            record->state.pendulum_rate_rad_s * 1000.0F);
                        telemetry_report.phi_mrad = (int32_t)(
                            record->state.arm_angle_rad * 1000.0F);
                        telemetry_report.phi_rate_mrad_s = (int32_t)(
                            record->state.arm_rate_rad_s * 1000.0F);
                        telemetry_report.sample_age_us =
                            record->sensor.sample_age_us;
                    }

                    telemetry_report.missed_cycles =
                        control_missed_cycles;
                    telemetry_report.gate_allowed =
                        closed_loop_gate_result.allowed;
                    telemetry_report.gate_reject_flags =
                        closed_loop_gate_result.reject_flags;
                    telemetry_report.pending = true;
                }
            } else {
                telemetry_phase = 0U;
            }

            if (control_cycle_due) {
                telemetry_work_us =
                    board_time_micros() - telemetry_work_us;
                oled_work_us = board_time_micros();
            }

            if (control_cycle_due && oled_ready) {
                /* Only prepare framebuffer state inside the 1 kHz workload. */
                ui_divider++;

                if ((ui_divider >= OLED_UI_REFRESH_TICKS) &&
                    ssd1315_is_idle(&oled)) {
                    local_ui_snapshot_t snapshot = {0};
                    uint8_t row;

                    snapshot.control_trace_valid =
                        control_trace_capture.valid;
                    snapshot.control_runtime_ready =
                        control_runtime_ready;
                    snapshot.telemetry_enabled =
                        telemetry_toggle_is_enabled(&telemetry_toggle);
                    snapshot.missed_cycles = control_missed_cycles;
                    snapshot.vbus_millivolts =
                        board_adc_read_vbus_millivolts();
                    snapshot.encoder_count = encoder_count;
                    snapshot.maintenance_output_percent =
                        motor_test_service_percent(&motor_test);
                    snapshot.motor_channel_d1 =
                        board_motor_get_channel() ==
                        BOARD_MOTOR_CHANNEL_D1;
                    snapshot.oled_contrast =
                        local_ui_get_contrast(&local_ui);
                    snapshot.control_mode =
                        control_pipeline_get_mode(&control_pipeline);
                    snapshot.estimator_mode =
                        control_profile.runtime.estimator_mode;
                    snapshot.balance_controller =
                        control_profile.runtime.balance_controller;

                    if (control_trace_capture.valid) {
                        const control_trace_record_t *record =
                            &control_trace_capture.latest;

                        snapshot.control_allowed =
                            record->control_allowed;
                        snapshot.control_mode =
                            record->control_mode;
                        snapshot.estimator_mode =
                            record->estimator_mode;
                        snapshot.balance_controller =
                            record->balance_controller;
                        snapshot.fault_flags =
                            record->state_safety_flags;
                        snapshot.sample_age_us =
                            record->sensor.sample_age_us;
                        snapshot.theta_mrad = (int32_t)(
                            record->state.pendulum_angle_rad * 1000.0F);
                        snapshot.theta_rate_mrad_s = (int32_t)(
                            record->state.pendulum_rate_rad_s * 1000.0F);
                        snapshot.phi_mrad = (int32_t)(
                            record->state.arm_angle_rad * 1000.0F);
                        snapshot.phi_rate_mrad_s = (int32_t)(
                            record->state.arm_rate_rad_s * 1000.0F);
                        snapshot.control_milli = (int32_t)(
                            record->control.u * 1000.0F);
                        snapshot.actuator_tenths_percent = (int32_t)(
                            record->actuator.pwm_percent * 10.0F);
                    }

                    local_ui_render(
                        &local_ui,
                        &snapshot,
                        &local_ui_frame);
                    for (row = 0U; row < LOCAL_UI_ROWS; row++) {
                        ssd1315_write_line(
                            &oled,
                            row,
                            local_ui_frame.lines[row]);
                    }
                    ui_divider = 0U;
                }
            }

            if (control_cycle_due) {
                oled_work_us =
                    board_time_micros() - oled_work_us;
            }

            if (led_divider >= LED_TOGGLE_TICKS) {
                led_divider = 0U;
                board_led_toggle();
            }

            if (control_cycle_due) {
                uint32_t tick_work_us =
                    board_time_micros() - tick_work_start_us;

                perf_ticks++;
                perf_control_total_us += control_path_us;
                perf_work_total_us += tick_work_us;
                perf_telemetry_total_us += telemetry_work_us;
                perf_oled_total_us += oled_work_us;

                if (telemetry_work_us > perf_telemetry_max_us) {
                    perf_telemetry_max_us = telemetry_work_us;
                }
                if (oled_work_us > perf_oled_max_us) {
                    perf_oled_max_us = oled_work_us;
                }

                if (control_path_us > perf_control_max_us) {
                    perf_control_max_us = control_path_us;
                }
                if (tick_work_us > perf_work_max_us) {
                    perf_work_max_us = tick_work_us;
                }

                if (perf_ticks >= PERF_REPORT_TICKS) {
                    perf_report_control_avg_us =
                        perf_control_total_us / perf_ticks;
                    perf_report_control_max_us = perf_control_max_us;
                    perf_report_work_avg_us =
                        perf_work_total_us / perf_ticks;
                    perf_report_work_max_us = perf_work_max_us;
                    perf_report_telemetry_avg_us =
                        perf_telemetry_total_us / perf_ticks;
                    perf_report_telemetry_max_us =
                        perf_telemetry_max_us;
                    perf_report_oled_avg_us =
                        perf_oled_total_us / perf_ticks;
                    perf_report_oled_max_us =
                        perf_oled_max_us;
                    perf_report_missed =
                        control_missed_cycles - perf_missed_base;
                    perf_missed_base = control_missed_cycles;
                    perf_ticks = 0U;
                    perf_control_total_us = 0U;
                    perf_control_max_us = 0U;
                    perf_work_total_us = 0U;
                    perf_work_max_us = 0U;
                    perf_telemetry_total_us = 0U;
                    perf_telemetry_max_us = 0U;
                    perf_oled_total_us = 0U;
                    perf_oled_max_us = 0U;
                    perf_report_due = true;
                }
            }
        }

        if (perf_report_due) {
            printf(
                "[PERF] ctrl_avg_us=%lu ctrl_max_us=%lu "
                "work_avg_us=%lu work_max_us=%lu "
                "cpu_sched_pct=%lu.%lu missed=%lu\n",
                (unsigned long)perf_report_control_avg_us,
                (unsigned long)perf_report_control_max_us,
                (unsigned long)perf_report_work_avg_us,
                (unsigned long)perf_report_work_max_us,
                (unsigned long)(perf_report_work_avg_us / 10U),
                (unsigned long)(perf_report_work_avg_us % 10U),
                (unsigned long)perf_report_missed);
            printf(
                "[PERF_SUB] telem_avg_us=%lu telem_max_us=%lu "
                "oled_fg_avg_us=%lu oled_fg_max_us=%lu\n",
                (unsigned long)perf_report_telemetry_avg_us,
                (unsigned long)perf_report_telemetry_max_us,
                (unsigned long)perf_report_oled_avg_us,
                (unsigned long)perf_report_oled_max_us);
            perf_report_due = false;
        }

        /*
         * OLED transport is blocking software SPI. Service only after the
         * real-time backlog has been drained, and keep each slice short so a
         * newly arriving 1 ms tick is delayed by substantially less than one
         * control period.
         */
        if (oled_ready) {
            ssd1315_service(
                &oled,
                OLED_BACKGROUND_FLUSH_BYTES);
        }

        /*
         * Formatting is intentionally outside the 1 kHz scheduled workload.
         * The UART driver only enqueues the resulting bytes and DMA performs
         * the physical transmission.
         */
        if (telemetry_report.pending) {
            telemetry_report.pending = false;
            telemetry_write_report(&telemetry_report);
        }

        /*
         * CLI writes are blocking.  Handle them only after the real-time tick
         * backlog above has been drained.  A newly requested motor output is
         * applied immediately by motor_test_service_start().
         */
        while (board_uart_try_read_char(&received_character)) {
            command_service_feed_char(
                &command_service,
                received_character);
        }
    }
}
