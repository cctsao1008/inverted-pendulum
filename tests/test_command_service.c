#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "command_service.h"

#define OUTPUT_CAPACITY 2048U

typedef struct {
    char text[OUTPUT_CAPACITY];
    size_t length;
} output_capture_t;

typedef struct {
    int8_t last_percent;
    int32_t encoder_count;
    char channel[3];
} motor_capture_t;

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

int main(void)
{
    test_telemetry_command_and_button_share_state();
    test_parameter_set_changes_active_value();
    test_invalid_rate_is_rejected();
    test_transport_status_records_architecture_decision();
    test_overlong_line_is_discarded();
    test_motor_command_requires_arm_and_runs_bounded_test();
    test_motor_channel_selection_stops_and_disarms();
    test_motor_pattern_command_requires_arm_and_starts_both();
    test_motor_identify_command_requires_arm_and_starts_pulse();
    test_motor_characterize_command_requires_direction_and_arm();
    test_motor_response_command_is_parameterized_and_bounded();
    test_motor_brake_response_is_bounded_and_d2_only();

    printf("PASS: command service tests\n");
    return 0;
}
