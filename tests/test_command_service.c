#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "command_service.h"

#define OUTPUT_CAPACITY 4096U

typedef struct {
    char text[OUTPUT_CAPACITY];
    size_t length;
} output_capture_t;

typedef struct {
    int8_t last_percent;
    int32_t encoder_count;
    char channel[3];
} motor_capture_t;

typedef struct {
    unsigned int call_count;
    command_oled_operation_t last_operation;
    uint8_t last_value;
    uint8_t contrast;
    uint8_t iref;
    uint8_t vcom;
    bool available;
} oled_capture_t;

static oled_capture_t g_oled;

static bool capture_oled(
    command_oled_operation_t operation,
    uint8_t value,
    void *context)
{
    oled_capture_t *capture = context;

    if (!capture->available) {
        return false;
    }

    capture->call_count++;
    capture->last_operation = operation;
    capture->last_value = value;

    switch (operation) {
    case COMMAND_OLED_CONTRAST:
        capture->contrast = value;
        break;
    case COMMAND_OLED_IREF:
        capture->iref = value;
        break;
    case COMMAND_OLED_VCOM:
        capture->vcom = value;
        break;
    default:
        break;
    }

    return true;
}

static void capture_oled_status(
    uint8_t *contrast,
    uint8_t *iref,
    uint8_t *vcom,
    void *context)
{
    oled_capture_t *capture = context;

    *contrast = capture->contrast;
    *iref = capture->iref;
    *vcom = capture->vcom;
}

static uint32_t g_fake_now_ms;

static uint32_t fake_now_ms(void *context)
{
    (void)context;
    return g_fake_now_ms;
}

static void capture_motor_output(int8_t signed_percent, void *context)
{
    motor_capture_t *capture = context;

    capture->last_percent = signed_percent;
}

static int32_t capture_encoder(void *context)
{
    return ((motor_capture_t *)context)->encoder_count;
}

static void capture_write(const char *text, void *context)
{
    output_capture_t *capture = context;
    size_t remaining = OUTPUT_CAPACITY - capture->length - 1U;
    size_t length = strlen(text);

    if (length > remaining) {
        length = remaining;
    }

    memcpy(&capture->text[capture->length], text, length);
    capture->length += length;
    capture->text[capture->length] = '\0';
}

static uint32_t capture_vbus_mv(void *context)
{
    (void)context;
    return 12000U;
}

static const char *capture_get_motor_channel(void *context)
{
    motor_capture_t *capture = context;

    return capture->channel;
}

static bool capture_set_motor_channel(const char *channel, void *context)
{
    motor_capture_t *capture = context;

    if ((strcmp(channel, "d1") != 0) &&
        (strcmp(channel, "d2") != 0)) {
        return false;
    }

    (void)snprintf(capture->channel, sizeof(capture->channel), "%s", channel);
    return true;
}

static void feed_text(
    command_service_t *service,
    const char *text)
{
    while (*text != '\0') {
        command_service_feed_char(service, *text++);
    }
}

static void setup(
    command_service_t *service,
    runtime_parameters_t *parameters,
    telemetry_toggle_t *telemetry,
    motor_test_service_t *motor,
    motor_capture_t *motor_capture,
    output_capture_t *capture)
{
    memset(capture, 0, sizeof(*capture));
    memset(&g_oled, 0, sizeof(g_oled));
    g_oled.available = true;
    g_oled.contrast = 0xEFU;
    g_oled.iref = 0x10U;
    g_oled.vcom = 0x20U;
    g_fake_now_ms = 0U;
    (void)snprintf(
        motor_capture->channel,
        sizeof(motor_capture->channel),
        "%s",
        "d2");
    runtime_parameters_init_defaults(parameters);
    telemetry_toggle_init(telemetry, false, 0U);
    motor_test_service_init_with_encoder(
        motor,
        capture_motor_output,
        motor_capture,
        fake_now_ms,
        NULL,
        capture_encoder,
        motor_capture);
    command_service_init(
        service,
        parameters,
        telemetry,
        motor,
        capture_vbus_mv,
        NULL,
        capture_get_motor_channel,
        capture_set_motor_channel,
        motor_capture,
        capture_oled,
        capture_oled_status,
        &g_oled,
        capture_write,
        capture);
}

static void test_telemetry_command_and_button_share_state(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "telem on\r\n");
    assert(telemetry_toggle_is_enabled(&telemetry));
    assert(strstr(capture.text, "[OK] telemetry=on") != NULL);

    (void)telemetry_toggle_toggle(&telemetry);
    assert(!telemetry_toggle_is_enabled(&telemetry));
}

static void test_parameter_set_changes_active_value(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(
        &service,
        "param set sensor.pendulum.upright_adc 3008\n");

    assert(parameters.pendulum_upright_adc == 3008U);
    assert(runtime_parameters_is_dirty(&parameters));
    assert(strstr(capture.text, "upright_adc=3008 dirty=1") != NULL);
}

static void test_invalid_rate_is_rejected(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "telem rate 100\n");

    assert(parameters.telemetry_rate_hz == 10U);
    assert(strstr(capture.text, "[ERR]") != NULL);
}

static void test_oled_sweep_commands_reach_the_panel(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "oled all\n");
    assert(g_oled.last_operation == COMMAND_OLED_ENTIRE_ON);

    feed_text(&service, "oled ram\n");
    assert(g_oled.last_operation == COMMAND_OLED_RESUME_RAM);

    feed_text(&service, "oled pattern\n");
    assert(g_oled.last_operation == COMMAND_OLED_PATTERN);

    feed_text(&service, "oled invert on\n");
    assert(g_oled.last_operation == COMMAND_OLED_INVERT_ON);

    feed_text(&service, "oled pump off\n");
    assert(g_oled.last_operation == COMMAND_OLED_CHARGE_PUMP);
    assert(g_oled.last_value == 0U);

    /* Hex is the natural way to type the datasheet's register values. */
    feed_text(&service, "oled iref 0x30\n");
    assert(g_oled.last_operation == COMMAND_OLED_IREF);
    assert(g_oled.iref == 0x30U);

    feed_text(&service, "oled vcom 0x30\n");
    assert(g_oled.vcom == 0x30U);

    /* Decimal too, because contrast is naturally read as 0..255. */
    feed_text(&service, "oled contrast 255\n");
    assert(g_oled.last_operation == COMMAND_OLED_CONTRAST);
    assert(g_oled.contrast == 255U);

    feed_text(&service, "oled status\n");
    assert(strstr(capture.text, "iref=0x30") != NULL);
    assert(strstr(capture.text, "vcom=0x30") != NULL);
    assert(strstr(capture.text, "contrast=0xFF") != NULL);
    assert(strstr(capture.text, "readback=unavailable") != NULL);
}

static void test_oled_raw_sends_each_byte_and_rejects_garbage(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;
    unsigned int before;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "oled raw 0xAE 0x8D 0x14 0xAF\n");
    assert(g_oled.call_count == 4U);
    assert(g_oled.last_operation == COMMAND_OLED_RAW);
    assert(g_oled.last_value == 0xAFU);

    before = g_oled.call_count;
    feed_text(&service, "oled raw 0xZZ\n");
    assert(g_oled.call_count == before);
    assert(strstr(capture.text, "[ERR]") != NULL);

    before = g_oled.call_count;
    feed_text(&service, "oled contrast 256\n");
    assert(g_oled.call_count == before);
}

static void test_oled_reports_unavailable_without_a_panel_binding(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    g_oled.available = false;
    feed_text(&service, "oled on\n");

    assert(strstr(capture.text, "oled unavailable") != NULL);
    assert(g_oled.call_count == 0U);
}

static void test_transport_status_records_architecture_decision(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "transport status\n");

    assert(strstr(capture.text, "active=text") != NULL);
    assert(strstr(capture.text, "xrce=roadmap") != NULL);
    assert(strstr(capture.text, "cobs=disabled") != NULL);
}

static void test_overlong_line_is_discarded(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;
    uint32_t index;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    for (index = 0U;
         index < COMMAND_SERVICE_LINE_CAPACITY + 10U;
         index++) {
        command_service_feed_char(&service, 'x');
    }
    command_service_feed_char(&service, '\n');
    feed_text(&service, "status\n");

    assert(strstr(capture.text, "command line too long") != NULL);
    assert(strstr(capture.text, "motor=stopped") != NULL);
}

static void test_motor_command_requires_arm_and_runs_bounded_test(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "motor test 10 100\n");
    assert(motor_capture.last_percent == 0);
    assert(strstr(capture.text, "motor is not armed") != NULL);

    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor test -10 100\n");
    assert(strstr(capture.text, "vbus_mV=12000") != NULL);
    /* The output must be applied at the command boundary, not on a later
     * scheduler tick that might belong to historical catch-up work. */
    assert(motor_capture.last_percent == -10);
    assert(motor_test_service_is_active(&motor));
    g_fake_now_ms++;
    assert(!motor_test_service_update_1ms(&motor));
    assert(motor_capture.last_percent == -10);
    assert(motor_test_service_remaining_ms(&motor) == 99U);

    feed_text(&service, "motor stop\n");
    assert(motor_capture.last_percent == 0);
    assert(!motor_test_service_is_armed(&motor));
    assert(!motor_test_service_is_active(&motor));

    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor test 21 100\n");
    assert(!motor_test_service_is_armed(&motor));
    assert(motor_capture.last_percent == 0);
}

static void test_motor_channel_selection_stops_and_disarms(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor channel d1\n");
    assert(strcmp(motor_capture.channel, "d1") == 0);
    assert(!motor_test_service_is_armed(&motor));
    assert(motor_capture.last_percent == 0);
    assert(strstr(capture.text, "channel=d1") != NULL);
    assert(strstr(capture.text, "PB0/TIM3_CH3") != NULL);

    feed_text(&service, "motor channel invalid\n");
    assert(strcmp(motor_capture.channel, "d1") == 0);
    assert(strstr(capture.text, "must be d1 or d2") != NULL);
}

static void test_motor_pattern_command_requires_arm_and_starts_both(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "motor pattern both\n");
    assert(motor_capture.last_percent == 0);
    assert(strstr(capture.text, "motor is not armed") != NULL);

    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor pattern both\n");
    assert(motor_capture.last_percent == MOTOR_PATTERN_PERCENT);
    assert(motor_test_service_pattern(&motor) == MOTOR_PATTERN_BOTH);
    assert(strstr(capture.text, "pattern=both active") != NULL);
    assert(strstr(capture.text, "right=+15%/5000ms") != NULL);

    feed_text(&service, "motor stop\n");
    assert(motor_capture.last_percent == 0);
    assert(!motor_test_service_is_armed(&motor));
}

static void test_motor_identify_command_requires_arm_and_starts_pulse(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "motor identify\n");
    assert(motor_capture.last_percent == 0);
    assert(strstr(capture.text, "motor is not armed") != NULL);

    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor identify\n");
    assert(motor_capture.last_percent == MOTOR_IDENTIFY_FIRST_PERCENT);
    assert(motor_test_service_identify_state(&motor) ==
           MOTOR_IDENTIFY_DRIVE);
    assert(strstr(capture.text, "motor identify active") != NULL);
    assert(strstr(capture.text, "pulse=+5%/250ms") != NULL);
}

static void test_motor_characterize_command_requires_direction_and_arm(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture = {{0}, 0U};

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "motor characterize right\n");
    assert(strstr(capture.text, "motor is not armed") != NULL);

    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor characterize sideways\n");
    assert(strstr(capture.text, "direction must be right or left") != NULL);
    assert(motor_capture.last_percent == 0);

    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor characterize left\n");
    assert(motor_capture.last_percent ==
           -MOTOR_CHARACTERIZE_START_PERCENT);
    assert(motor_test_service_characterize_state(&motor) ==
           MOTOR_CHARACTERIZE_RAMP_UP);
    assert(strstr(capture.text, "direction=left active") != NULL);
}

static void test_motor_response_command_is_parameterized_and_bounded(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture = {{0}, 0U};

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "motor response right 50 5000\n");
    assert(strstr(capture.text, "motor is not armed") != NULL);

    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor response right 50 5000\n");
    assert(motor_capture.last_percent == 50);
    assert(motor_test_service_response_state(&motor) == MOTOR_RESPONSE_DRIVE);
    assert(strstr(capture.text, "direction=right output_pct=50") != NULL);

    feed_text(&service, "motor stop\n");
    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor response left 70 10000\n");
    assert(motor_capture.last_percent == -70);
}

static void test_motor_brake_response_is_bounded_and_d2_only(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture = {{0}, 0U};
    setup(&service, &parameters, &telemetry, &motor, &motor_capture, &capture);

    feed_text(&service, "motor channel d1\n");
    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor brake-response right 50 5000 10\n");
    assert(strstr(capture.text, "requires characterized channel d2") != NULL);
    assert(motor_capture.last_percent == 0);

    feed_text(&service, "motor channel d2\n");
    feed_text(&service, "motor arm\n");
    feed_text(&service, "motor brake-response left 50 5000 10\n");
    assert(motor_capture.last_percent == -50);
    assert(motor_test_service_brake_state(&motor) == MOTOR_BRAKE_DRIVE);
    assert(strstr(capture.text, "brake_pct=10") != NULL);
}

static void test_script_records_safe_lines_without_running_them(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture = {{0}, 0U};

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service,
              "script begin\n"
              "motor brake-response right 50 5000 10\n"
              "wait 5000\n"
              "motor brake-response left 50 5000 10\n"
              "script end\n"
              "script list\n");

    assert(!motor_test_service_is_active(&motor));
    assert(motor_capture.last_percent == 0);
    assert(strstr(capture.text, "script stored lines=3") != NULL);
    assert(strstr(capture.text, "[SCRIPT] 1: motor brake-response right 50 5000 10") != NULL);
    assert(strstr(capture.text, "[SCRIPT] 2: wait 5000") != NULL);
    assert(strstr(capture.text, "[SCRIPT] 3: motor brake-response left 50 5000 10") != NULL);
}

static void test_script_rejects_unsafe_recorded_lines(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture = {{0}, 0U};

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service,
              "script begin\n"
              "motor arm\n"
              "motor brake-response right 50 5000 10\n");

    assert(strstr(capture.text, "script line rejected") != NULL);
    assert(service.script_line_count == 0U);
    assert(!service.script_recording);
}

static void test_script_run_requires_arm_and_sequences_motor_wait_motor(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture = {{0}, 0U};

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service,
              "script begin\n"
              "motor brake-response right 50 5000 10\n"
              "wait 5\n"
              "motor brake-response left 50 5000 10\n"
              "script end\n"
              "script run\n");
    assert(strstr(capture.text, "motor is not armed") != NULL);
    assert(!service.script_running);

    feed_text(&service, "motor arm\n");
    feed_text(&service, "script run\n");
    command_service_update_1ms(&service);
    assert(service.script_running);
    assert(service.script_motor_active);
    assert(motor_test_service_brake_state(&motor) == MOTOR_BRAKE_DRIVE);
    assert(motor_capture.last_percent == 50);

    motor.active = false;
    motor.armed = false;
    motor.brake_state = MOTOR_BRAKE_DONE;
    command_service_update_1ms(&service);
    assert(service.script_waiting);
    assert(motor_capture.last_percent == 0);

    g_fake_now_ms += 5U;
    command_service_update_1ms(&service);
    assert(service.script_motor_active);
    assert(motor_test_service_brake_state(&motor) == MOTOR_BRAKE_DRIVE);
    assert(motor_capture.last_percent == -50);

    motor.active = false;
    motor.armed = false;
    motor.brake_state = MOTOR_BRAKE_DONE;
    command_service_update_1ms(&service);
    assert(!service.script_running);
    assert(strstr(capture.text, "[SCRIPT] complete lines=3 armed=0") != NULL);
}

static void test_script_load_brake_sweep_generates_repeat_template(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture = {{0}, 0U};

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service,
              "script load brake-sweep 40 5000\n"
              "script list\n");

    assert(service.script_line_count == 11U);
    assert(strstr(capture.text, "script loaded brake-sweep drive_pct=40") != NULL);
    assert(strstr(capture.text, "brake_pct=10") != NULL);
    assert(strstr(capture.text, "[SCRIPT] 1: motor brake-response right 40 5000 10") != NULL);
    assert(strstr(capture.text, "[SCRIPT] 2: wait 5000") != NULL);
    assert(strstr(capture.text, "[SCRIPT] 3: motor brake-response left 40 5000 10") != NULL);
    assert(strstr(capture.text, "[SCRIPT] 11: motor brake-response left 40 5000 10") != NULL);
    assert(!motor_test_service_is_active(&motor));
    assert(motor_capture.last_percent == 0);
}

static void test_script_load_brake_sweep_rejects_invalid_values(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    motor_test_service_t motor;
    motor_capture_t motor_capture = {0};
    output_capture_t capture = {{0}, 0U};

    setup(&service, &parameters, &telemetry, &motor,
          &motor_capture, &capture);

    feed_text(&service, "script load brake-sweep 25 5000 10\n");
    assert(service.script_line_count == 0U);
    assert(strstr(capture.text, "brake-sweep requires") != NULL);

    feed_text(&service, "script load brake-sweep 40 5000 15\n");
    assert(service.script_line_count == 11U);
    assert(strstr(capture.text, "brake_pct=15") != NULL);
    assert(strstr(capture.text, "[OK] script loaded") != NULL);
}

int main(void)
{
    test_telemetry_command_and_button_share_state();
    test_parameter_set_changes_active_value();
    test_invalid_rate_is_rejected();
    test_transport_status_records_architecture_decision();
    test_oled_sweep_commands_reach_the_panel();
    test_oled_raw_sends_each_byte_and_rejects_garbage();
    test_oled_reports_unavailable_without_a_panel_binding();
    test_overlong_line_is_discarded();
    test_motor_command_requires_arm_and_runs_bounded_test();
    test_motor_channel_selection_stops_and_disarms();
    test_motor_pattern_command_requires_arm_and_starts_both();
    test_motor_identify_command_requires_arm_and_starts_pulse();
    test_motor_characterize_command_requires_direction_and_arm();
    test_motor_response_command_is_parameterized_and_bounded();
    test_motor_brake_response_is_bounded_and_d2_only();
    test_script_records_safe_lines_without_running_them();
    test_script_rejects_unsafe_recorded_lines();
    test_script_run_requires_arm_and_sequences_motor_wait_motor();
    test_script_load_brake_sweep_generates_repeat_template();
    test_script_load_brake_sweep_rejects_invalid_values();

    printf("PASS: command service tests\n");
    return 0;
}
