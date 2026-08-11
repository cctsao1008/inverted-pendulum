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
    output_capture_t *capture)
{
    memset(capture, 0, sizeof(*capture));
    runtime_parameters_init_defaults(parameters);
    telemetry_toggle_init(telemetry, false, 0U);
    command_service_init(
        service,
        parameters,
        telemetry,
        capture_write,
        capture);
}

static void test_telemetry_command_and_button_share_state(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &capture);

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
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &capture);

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
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &capture);

    feed_text(&service, "telem rate 100\n");

    assert(parameters.telemetry_rate_hz == 10U);
    assert(strstr(capture.text, "[ERR]") != NULL);
}

static void test_transport_status_records_architecture_decision(void)
{
    command_service_t service;
    runtime_parameters_t parameters;
    telemetry_toggle_t telemetry;
    output_capture_t capture;

    setup(&service, &parameters, &telemetry, &capture);

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
    output_capture_t capture;
    uint32_t index;

    setup(&service, &parameters, &telemetry, &capture);

    for (index = 0U;
         index < COMMAND_SERVICE_LINE_CAPACITY + 10U;
         index++) {
        command_service_feed_char(&service, 'x');
    }
    command_service_feed_char(&service, '\n');
    feed_text(&service, "status\n");

    assert(strstr(capture.text, "command line too long") != NULL);
    assert(strstr(capture.text, "motor=disabled") != NULL);
}

int main(void)
{
    test_telemetry_command_and_button_share_state();
    test_parameter_set_changes_active_value();
    test_invalid_rate_is_rejected();
    test_transport_status_records_architecture_decision();
    test_overlong_line_is_discarded();

    printf("PASS: command service tests\n");
    return 0;
}
