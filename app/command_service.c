#include "command_service.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMMAND_MAX_ARGUMENTS 6U
#define COMMAND_RESPONSE_CAPACITY 256U
#define COMMAND_SCRIPT_BRAKE_SWEEP_REPEATS 3U
#define COMMAND_SCRIPT_BRAKE_SWEEP_WAIT_MS 5000U

static bool deadline_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static void write_response(
    command_service_t *service,
    const char *format,
    ...)
{
    char response[COMMAND_RESPONSE_CAPACITY];
    va_list arguments;

    va_start(arguments, format);
    (void)vsnprintf(
        response,
        sizeof(response),
        format,
        arguments);
    va_end(arguments);

    service->write(response, service->write_context);
}

static size_t split_arguments(
    char *line,
    char **arguments,
    size_t capacity)
{
    size_t count = 0U;
    char *cursor = line;

    while ((*cursor != '\0') && (count < capacity)) {
        while ((*cursor == ' ') || (*cursor == '\t')) {
            cursor++;
        }

        if (*cursor == '\0') {
            break;
        }

        arguments[count++] = cursor;

        while ((*cursor != '\0') &&
               (*cursor != ' ') &&
               (*cursor != '\t')) {
            cursor++;
        }

        if (*cursor != '\0') {
            *cursor++ = '\0';
        }
    }

    return count;
}

static bool parse_int32(const char *text, int32_t *value)
{
    char *end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);

    if ((errno != 0) ||
        (end == text) ||
        (*end != '\0') ||
        (parsed < INT32_MIN) ||
        (parsed > INT32_MAX)) {
        return false;
    }

    *value = (int32_t)parsed;
    return true;
}

static bool is_script_allowed_motor_line(
    size_t count,
    char **arguments)
{
    if ((count == 6U) &&
        (strcmp(arguments[0], "motor") == 0) &&
        (strcmp(arguments[1], "brake-response") == 0)) {
        return true;
    }

    return false;
}

static bool is_script_allowed_wait_line(
    size_t count,
    char **arguments)
{
    int32_t wait_ms;

    if ((count == 2U) &&
        (strcmp(arguments[0], "wait") == 0) &&
        parse_int32(arguments[1], &wait_ms) &&
        (wait_ms >= 0) &&
        (wait_ms <= (int32_t)COMMAND_SCRIPT_MAX_RUN_MS)) {
        return true;
    }

    return false;
}

static bool is_script_allowed_line(const char *line)
{
    char scratch[COMMAND_SERVICE_LINE_CAPACITY];
    char *arguments[COMMAND_MAX_ARGUMENTS];
    size_t count;

    (void)snprintf(scratch, sizeof(scratch), "%s", line);
    count = split_arguments(scratch, arguments, COMMAND_MAX_ARGUMENTS);

    return is_script_allowed_motor_line(count, arguments) ||
           is_script_allowed_wait_line(count, arguments);
}

static void script_clear_buffer(command_service_t *service)
{
    service->script_recording = false;
    service->script_line_count = 0U;
    service->script_cursor = 0U;
}

static void script_abort(
    command_service_t *service,
    const char *reason)
{
    uint8_t line_number = service->script_cursor;

    if (service->script_motor_active && motor_test_service_is_active(service->motor)) {
        motor_test_service_disarm(service->motor);
    }

    service->script_running = false;
    service->script_waiting = false;
    service->script_motor_active = false;
    service->script_cursor = 0U;
    service->script_deadline_ms = 0U;
    service->script_wait_deadline_ms = 0U;

    write_response(
        service,
        "[SCRIPT] abort line=%u reason=%s armed=0 output_pct=0\n",
        (unsigned int)(line_number + 1U),
        reason);
}

static bool script_load_brake_sweep(
    command_service_t *service,
    int32_t drive_percent,
    int32_t drive_ms,
    int32_t brake_percent)
{
    uint8_t line = 0U;
    uint8_t repeat;

    if ((drive_percent < MOTOR_RESPONSE_MIN_PERCENT) ||
        (drive_percent > MOTOR_RESPONSE_MAX_PERCENT) ||
        (drive_ms < (int32_t)MOTOR_RESPONSE_MIN_DRIVE_MS) ||
        (drive_ms > (int32_t)MOTOR_RESPONSE_MAX_DRIVE_MS) ||
        (brake_percent < MOTOR_BRAKE_MIN_PERCENT) ||
        (brake_percent > MOTOR_BRAKE_MAX_PERCENT)) {
        return false;
    }

    script_clear_buffer(service);

    for (repeat = 0U;
         repeat < COMMAND_SCRIPT_BRAKE_SWEEP_REPEATS;
         repeat++) {
        (void)snprintf(
            service->script_lines[line++],
            COMMAND_SERVICE_LINE_CAPACITY,
            "motor brake-response right %ld %ld %ld",
            (long)drive_percent,
            (long)drive_ms,
            (long)brake_percent);
        (void)snprintf(
            service->script_lines[line++],
            COMMAND_SERVICE_LINE_CAPACITY,
            "wait %u",
            (unsigned int)COMMAND_SCRIPT_BRAKE_SWEEP_WAIT_MS);
        (void)snprintf(
            service->script_lines[line++],
            COMMAND_SERVICE_LINE_CAPACITY,
            "motor brake-response left %ld %ld %ld",
            (long)drive_percent,
            (long)drive_ms,
            (long)brake_percent);

        if (repeat != (COMMAND_SCRIPT_BRAKE_SWEEP_REPEATS - 1U)) {
            (void)snprintf(
                service->script_lines[line++],
                COMMAND_SERVICE_LINE_CAPACITY,
                "wait %u",
                (unsigned int)COMMAND_SCRIPT_BRAKE_SWEEP_WAIT_MS);
        }
    }

    service->script_line_count = line;
    return true;
}

static const char *motor_channel_io(const char *channel)
{
    return (strcmp(channel, "d1") == 0)
        ? "PB0/TIM3_CH3+PB14/PB15"
        : "PB1/TIM3_CH4+PB13/PB12";
}

static const char *motor_pattern_name(motor_pattern_t pattern)
{
    if (pattern == MOTOR_PATTERN_RIGHT) {
        return "right";
    }
    if (pattern == MOTOR_PATTERN_LEFT) {
        return "left";
    }
    if (pattern == MOTOR_PATTERN_BOTH) {
        return "both";
    }
    return "none";
}

static void execute_status(command_service_t *service)
{
    write_response(
        service,
        "[OK] motor=%s armed=%u arm_remaining_ms=%u output_pct=%d "
        "remaining_ms=%u pattern=%s "
        "channel=%s io=%s vbus_mV=%lu "
        "transport=text telemetry=%s rate_hz=%u param_dirty=%u "
        "persistence=unavailable\n",
        motor_test_service_is_active(service->motor)
            ? "test-active"
            : "stopped",
        motor_test_service_is_armed(service->motor) ? 1U : 0U,
        (unsigned int)motor_test_service_arm_remaining_ms(service->motor),
        (int)motor_test_service_percent(service->motor),
        (unsigned int)motor_test_service_remaining_ms(service->motor),
        motor_pattern_name(motor_test_service_pattern(service->motor)),
        service->get_motor_channel(service->motor_channel_context),
        motor_channel_io(
            service->get_motor_channel(service->motor_channel_context)),
        (unsigned long)service->read_vbus_mv(service->vbus_context),
        telemetry_toggle_is_enabled(service->telemetry)
            ? "on"
            : "off",
        (unsigned int)service->parameters->telemetry_rate_hz,
        runtime_parameters_is_dirty(service->parameters) ? 1U : 0U);
}

static void execute_motor(
    command_service_t *service,
    size_t count,
    char **arguments)
{
    int32_t percent;
    int32_t duration_ms;
    motor_test_result_t result;

    if ((count == 2U) &&
        (strcmp(arguments[1], "status") == 0)) {
        write_response(
            service,
            "[OK] motor=%s armed=%u arm_remaining_ms=%u output_pct=%d "
            "remaining_ms=%u pattern=%s "
            "channel=%s io=%s vbus_mV=%lu limit_pct=%u duration_ms=%u..%u\n",
            motor_test_service_is_active(service->motor)
                ? "test-active"
                : "stopped",
            motor_test_service_is_armed(service->motor) ? 1U : 0U,
            (unsigned int)
                motor_test_service_arm_remaining_ms(service->motor),
            (int)motor_test_service_percent(service->motor),
            (unsigned int)motor_test_service_remaining_ms(service->motor),
            motor_pattern_name(motor_test_service_pattern(service->motor)),
            service->get_motor_channel(service->motor_channel_context),
            motor_channel_io(
                service->get_motor_channel(service->motor_channel_context)),
            (unsigned long)service->read_vbus_mv(service->vbus_context),
            (unsigned int)MOTOR_TEST_MAX_PERCENT,
            (unsigned int)MOTOR_TEST_MIN_DURATION_MS,
            (unsigned int)MOTOR_TEST_MAX_DURATION_MS);
        return;
    }

    if ((count == 3U) &&
        (strcmp(arguments[1], "channel") == 0)) {
        /* A channel change always stops and disarms before touching GPIO. */
        motor_test_service_disarm(service->motor);

        if (service->set_motor_channel(
                arguments[2],
                service->motor_channel_context)) {
            write_response(
                service,
                "[OK] motor channel=%s stopped=1 armed=0; %s\n",
                service->get_motor_channel(
                    service->motor_channel_context),
                motor_channel_io(arguments[2]));
        } else {
            write_response(
                service,
                "[ERR] motor channel must be d1 or d2; motor stopped and disarmed\n");
        }
        return;
    }

    if ((count == 2U) &&
        (strcmp(arguments[1], "arm") == 0)) {
        if (motor_test_service_arm(service->motor)) {
            write_response(
                service,
                "[OK] motor armed for one bounded test; expires in %u ms; "
                "use motor test <signed_pct> <duration_ms> or motor pattern "
                "right|left|both, motor identify, or motor characterize "
                "right|left, motor response right|left <pct> <ms>, or motor "
                "brake-response right|left <drive_pct> <ms> <brake_pct>\n",
                (unsigned int)MOTOR_TEST_ARM_WINDOW_MS);
        } else {
            write_response(service, "[ERR] motor test already active\n");
        }
        return;
    }

    if ((count == 2U) &&
        ((strcmp(arguments[1], "stop") == 0) ||
         (strcmp(arguments[1], "disarm") == 0))) {
        motor_test_service_disarm(service->motor);
        write_response(
            service,
            "[OK] motor stopped output_pct=0 armed=0\n");
        return;
    }

    if ((count == 2U) &&
        (strcmp(arguments[1], "identify") == 0)) {
        result = motor_test_service_start_identify(service->motor);
        if (result == MOTOR_TEST_OK) {
            write_response(
                service,
                "[OK] motor identify active; channel=%s "
                "pulse=+%u%%/%lums retry=+%u%%/%lums "
                "min_delta=%d; output_applied=1\n",
                service->get_motor_channel(
                    service->motor_channel_context),
                (unsigned int)MOTOR_IDENTIFY_FIRST_PERCENT,
                (unsigned long)MOTOR_IDENTIFY_DRIVE_MS,
                (unsigned int)MOTOR_IDENTIFY_RETRY_PERCENT,
                (unsigned long)MOTOR_IDENTIFY_DRIVE_MS,
                (int)MOTOR_IDENTIFY_MIN_DELTA_COUNTS);
            return;
        }

        motor_test_service_disarm(service->motor);
        write_response(
            service,
            (result == MOTOR_TEST_NOT_ARMED)
                ? "[ERR] motor is not armed\n"
                : "[ERR] motor operation already active\n");
        return;
    }

    if ((count == 3U) &&
        (strcmp(arguments[1], "characterize") == 0)) {
        int8_t direction = 0;

        if (strcmp(arguments[2], "right") == 0) {
            direction = 1;
        } else if (strcmp(arguments[2], "left") == 0) {
            direction = -1;
        }

        if (direction == 0) {
            motor_test_service_disarm(service->motor);
            write_response(
                service,
                "[ERR] motor characterize direction must be right or left; "
                "motor stopped and disarmed\n");
            return;
        }

        result = motor_test_service_start_characterize(
            service->motor, direction);
        if (result == MOTOR_TEST_OK) {
            write_response(
                service,
                "[OK] motor characterize direction=%s active; "
                "ramp=%u..%u%% step=%u%%/%lums fall=1%%/%lums "
                "window=%lums timeout=%lums; output_applied=1\n",
                arguments[2],
                (unsigned int)MOTOR_CHARACTERIZE_START_PERCENT,
                (unsigned int)MOTOR_CHARACTERIZE_MAX_PERCENT,
                (unsigned int)MOTOR_CHARACTERIZE_RAMP_STEP_PERCENT,
                (unsigned long)MOTOR_CHARACTERIZE_RISE_STEP_MS,
                (unsigned long)MOTOR_CHARACTERIZE_FALL_STEP_MS,
                (unsigned long)MOTOR_CHARACTERIZE_WINDOW_MS,
                (unsigned long)MOTOR_CHARACTERIZE_TIMEOUT_MS);
            return;
        }

        motor_test_service_disarm(service->motor);
        write_response(
            service,
            (result == MOTOR_TEST_NOT_ARMED)
                ? "[ERR] motor is not armed\n"
                : "[ERR] motor operation already active\n");
        return;
    }

    if ((count == 5U) &&
        (strcmp(arguments[1], "response") == 0)) {
        int8_t direction = 0;

        if (strcmp(arguments[2], "right") == 0) {
            direction = 1;
        } else if (strcmp(arguments[2], "left") == 0) {
            direction = -1;
        }
        if ((direction == 0) ||
            !parse_int32(arguments[3], &percent) ||
            !parse_int32(arguments[4], &duration_ms) ||
            (duration_ms < 0)) {
            motor_test_service_disarm(service->motor);
            write_response(
                service,
                "[ERR] usage: motor response right|left <%u..%u> "
                "<%u..%u ms>; motor stopped and disarmed\n",
                (unsigned int)MOTOR_RESPONSE_MIN_PERCENT,
                (unsigned int)MOTOR_RESPONSE_MAX_PERCENT,
                (unsigned int)MOTOR_RESPONSE_MIN_DRIVE_MS,
                (unsigned int)MOTOR_RESPONSE_MAX_DRIVE_MS);
            return;
        }

        result = motor_test_service_start_response(
            service->motor, direction, percent, (uint32_t)duration_ms);
        if (result == MOTOR_TEST_OK) {
            write_response(
                service,
                "[OK] motor response direction=%s output_pct=%ld "
                "drive_ms=%ld coast_timeout_ms=%lu window_ms=%lu "
                "output_applied=1; auto-stop and auto-disarm enabled\n",
                arguments[2],
                (long)percent,
                (long)duration_ms,
                (unsigned long)MOTOR_RESPONSE_MAX_COAST_MS,
                (unsigned long)MOTOR_RESPONSE_WINDOW_MS);
            return;
        }

        motor_test_service_disarm(service->motor);
        if (result == MOTOR_TEST_NOT_ARMED) {
            write_response(service, "[ERR] motor is not armed\n");
        } else if (result == MOTOR_TEST_ALREADY_ACTIVE) {
            write_response(service, "[ERR] motor operation already active\n");
        } else if (result == MOTOR_TEST_INVALID_PERCENT) {
            write_response(
                service, "[ERR] response percent must be %u..%u\n",
                (unsigned int)MOTOR_RESPONSE_MIN_PERCENT,
                (unsigned int)MOTOR_RESPONSE_MAX_PERCENT);
        } else {
            write_response(
                service, "[ERR] response drive_ms must be %u..%u\n",
                (unsigned int)MOTOR_RESPONSE_MIN_DRIVE_MS,
                (unsigned int)MOTOR_RESPONSE_MAX_DRIVE_MS);
        }
        return;
    }

    if ((count == 6U) && (strcmp(arguments[1], "brake-response") == 0)) {
        int8_t direction = 0;
        int32_t brake_percent;
        if (strcmp(service->get_motor_channel(service->motor_channel_context),
                   "d2") != 0) {
            motor_test_service_disarm(service->motor);
            write_response(service, "[ERR] brake-response requires characterized channel d2; motor stopped and disarmed\n");
            return;
        }
        if (strcmp(arguments[2], "right") == 0) direction = 1;
        else if (strcmp(arguments[2], "left") == 0) direction = -1;
        if ((direction == 0) || !parse_int32(arguments[3], &percent) ||
            !parse_int32(arguments[4], &duration_ms) ||
            !parse_int32(arguments[5], &brake_percent) || (duration_ms < 0)) {
            motor_test_service_disarm(service->motor);
            write_response(service, "[ERR] usage: motor brake-response right|left <30..80> <1000..10000 ms> <10..20>; motor stopped and disarmed\n");
            return;
        }
        result = motor_test_service_start_brake_response(service->motor,
            direction, percent, (uint32_t)duration_ms, brake_percent);
        if (result == MOTOR_TEST_OK) {
            write_response(service, "[OK] motor brake-response direction=%s drive_pct=%ld drive_ms=%ld brake_pct=%ld neutral_ms=%u brake_timeout_ms=%u settle_ms=%u output_applied=1; velocity-release and auto-disarm enabled\n",
                arguments[2], (long)percent, (long)duration_ms,
                (long)brake_percent, (unsigned int)MOTOR_BRAKE_NEUTRAL_MS,
                (unsigned int)MOTOR_BRAKE_MAX_MS,
                (unsigned int)MOTOR_BRAKE_SETTLE_MS);
        } else {
            motor_test_service_disarm(service->motor);
            if (result == MOTOR_TEST_NOT_ARMED) write_response(service, "[ERR] motor is not armed\n");
            else if (result == MOTOR_TEST_ALREADY_ACTIVE) write_response(service, "[ERR] motor operation already active\n");
            else if (result == MOTOR_TEST_INVALID_PERCENT) write_response(service, "[ERR] drive percent must be 30..80 and brake percent 10..20\n");
            else write_response(service, "[ERR] drive_ms must be 1000..10000\n");
        }
        return;
    }

    if ((count == 3U) &&
        (strcmp(arguments[1], "pattern") == 0)) {
        motor_pattern_t pattern = MOTOR_PATTERN_NONE;

        if (strcmp(arguments[2], "right") == 0) {
            pattern = MOTOR_PATTERN_RIGHT;
        } else if (strcmp(arguments[2], "left") == 0) {
            pattern = MOTOR_PATTERN_LEFT;
        } else if (strcmp(arguments[2], "both") == 0) {
            pattern = MOTOR_PATTERN_BOTH;
        }

        if (pattern == MOTOR_PATTERN_NONE) {
            motor_test_service_disarm(service->motor);
            write_response(
                service,
                "[ERR] motor pattern must be right, left, or both; "
                "motor stopped and disarmed\n");
            return;
        }

        result = motor_test_service_start_pattern(service->motor, pattern);
        if (result == MOTOR_TEST_OK) {
            write_response(
                service,
                "[OK] motor pattern=%s active; channel=%s "
                "right=+%u%%/5000ms left=-%u%%/5000ms pause=1000ms "
                "output_applied=1; auto-stop and auto-disarm enabled\n",
                motor_pattern_name(pattern),
                service->get_motor_channel(
                    service->motor_channel_context),
                (unsigned int)MOTOR_PATTERN_PERCENT,
                (unsigned int)MOTOR_PATTERN_PERCENT);
            return;
        }

        motor_test_service_disarm(service->motor);
        write_response(
            service,
            (result == MOTOR_TEST_NOT_ARMED)
                ? "[ERR] motor is not armed\n"
                : "[ERR] motor test already active\n");
        return;
    }

    if ((count == 4U) &&
        (strcmp(arguments[1], "test") == 0)) {
        if (!parse_int32(arguments[2], &percent) ||
            !parse_int32(arguments[3], &duration_ms) ||
            (duration_ms < 0)) {
            motor_test_service_disarm(service->motor);
            write_response(
                service,
                "[ERR] motor test requires signed integer percent and "
                "duration_ms\n");
            return;
        }

        result = motor_test_service_start(
            service->motor,
            percent,
            (uint32_t)duration_ms);

        if (result == MOTOR_TEST_OK) {
            write_response(
                service,
                "[OK] motor test active output_pct=%ld duration_ms=%ld; "
                "channel=%s output_applied=1 vbus_mV=%lu; "
                "auto-stop and auto-disarm enabled\n",
                (long)percent,
                (long)duration_ms,
                service->get_motor_channel(
                    service->motor_channel_context),
                (unsigned long)
                    service->read_vbus_mv(service->vbus_context));
            return;
        }

        /* Any rejected test request fails closed and clears the arm latch. */
        motor_test_service_disarm(service->motor);

        if (result == MOTOR_TEST_NOT_ARMED) {
            write_response(service, "[ERR] motor is not armed\n");
        } else if (result == MOTOR_TEST_ALREADY_ACTIVE) {
            write_response(service, "[ERR] motor test already active\n");
        } else if (result == MOTOR_TEST_INVALID_PERCENT) {
            write_response(
                service,
                "[ERR] signed percent must be -%u..-1 or 1..%u\n",
                (unsigned int)MOTOR_TEST_MAX_PERCENT,
                (unsigned int)MOTOR_TEST_MAX_PERCENT);
        } else {
            write_response(
                service,
                "[ERR] duration_ms must be %u..%u\n",
                (unsigned int)MOTOR_TEST_MIN_DURATION_MS,
                (unsigned int)MOTOR_TEST_MAX_DURATION_MS);
        }
        return;
    }

    write_response(
        service,
        "[ERR] usage: motor status|channel d1|d2|arm|identify|characterize "
        "right|left|response right|left <pct> <ms>|brake-response "
        "right|left <drive_pct> <ms> <brake_pct>|test "
        "<signed_pct> <duration_ms>|pattern right|left|both|stop|disarm\n");
}

static void execute_telemetry(
    command_service_t *service,
    size_t count,
    char **arguments)
{
    runtime_parameter_result_t result;

    if ((count == 2U) &&
        (strcmp(arguments[1], "status") == 0)) {
        write_response(
            service,
            "[OK] telemetry=%s rate_hz=%u format=text\n",
            telemetry_toggle_is_enabled(service->telemetry)
                ? "on"
                : "off",
            (unsigned int)
                service->parameters->telemetry_rate_hz);
        return;
    }

    if ((count == 2U) &&
        ((strcmp(arguments[1], "on") == 0) ||
         (strcmp(arguments[1], "off") == 0))) {
        bool enabled = (strcmp(arguments[1], "on") == 0);

        (void)telemetry_toggle_set_enabled(
            service->telemetry,
            enabled);
        write_response(
            service,
            "[OK] telemetry=%s rate_hz=%u\n",
            enabled ? "on" : "off",
            (unsigned int)
                service->parameters->telemetry_rate_hz);
        return;
    }

    if ((count == 3U) &&
        (strcmp(arguments[1], "rate") == 0)) {
        result = runtime_parameters_set_text(
            service->parameters,
            "telem.rate_hz",
            arguments[2]);

        if (result == RUNTIME_PARAMETER_OK) {
            write_response(
                service,
                "[OK] telem.rate_hz=%u dirty=1\n",
                (unsigned int)
                    service->parameters->telemetry_rate_hz);
        } else {
            write_response(
                service,
                "[ERR] telem.rate_hz requires integer 1..20\n");
        }
        return;
    }

    write_response(
        service,
        "[ERR] usage: telem on|off|status|rate <1..20>\n");
}

static void execute_parameter_list(command_service_t *service)
{
    size_t index;

    for (index = 0U;
         index < runtime_parameters_count();
         index++) {
        const runtime_parameter_descriptor_t *descriptor =
            runtime_parameters_descriptor(index);
        int32_t value = 0;

        (void)runtime_parameters_get(
            service->parameters,
            descriptor->name,
            &value);

        write_response(
            service,
            "[PARAM] %s=%ld allowed=%s default=%ld\n",
            descriptor->name,
            (long)value,
            descriptor->allowed_values,
            (long)descriptor->default_value);
    }
}

static void execute_parameter(
    command_service_t *service,
    size_t count,
    char **arguments)
{
    runtime_parameter_result_t result;
    int32_t value = 0;

    if ((count == 2U) &&
        (strcmp(arguments[1], "list") == 0)) {
        execute_parameter_list(service);
        return;
    }

    if ((count == 2U) &&
        (strcmp(arguments[1], "defaults") == 0)) {
        runtime_parameters_init_defaults(service->parameters);
        write_response(
            service,
            "[OK] runtime parameters restored to defaults dirty=0\n");
        return;
    }

    if ((count == 3U) &&
        (strcmp(arguments[1], "get") == 0)) {
        result = runtime_parameters_get(
            service->parameters,
            arguments[2],
            &value);

        if (result == RUNTIME_PARAMETER_OK) {
            write_response(
                service,
                "[OK] %s=%ld\n",
                arguments[2],
                (long)value);
        } else {
            write_response(
                service,
                "[ERR] unknown parameter: %s\n",
                arguments[2]);
        }
        return;
    }

    if ((count == 4U) &&
        (strcmp(arguments[1], "set") == 0)) {
        result = runtime_parameters_set_text(
            service->parameters,
            arguments[2],
            arguments[3]);

        if (result == RUNTIME_PARAMETER_OK) {
            (void)runtime_parameters_get(
                service->parameters,
                arguments[2],
                &value);
            write_response(
                service,
                "[OK] %s=%ld dirty=1\n",
                arguments[2],
                (long)value);
        } else if (result == RUNTIME_PARAMETER_NOT_FOUND) {
            write_response(
                service,
                "[ERR] unknown parameter: %s\n",
                arguments[2]);
        } else {
            write_response(
                service,
                "[ERR] invalid value for %s\n",
                arguments[2]);
        }
        return;
    }

    if ((count == 2U) &&
        ((strcmp(arguments[1], "save") == 0) ||
         (strcmp(arguments[1], "load") == 0))) {
        write_response(
            service,
            "[ERR] persistent parameter storage is not implemented\n");
        return;
    }

    write_response(
        service,
        "[ERR] usage: param list|get <name>|set <name> <value>|"
        "defaults\n");
}

static void execute_transport(
    command_service_t *service,
    size_t count,
    char **arguments)
{
    if ((count == 2U) &&
        (strcmp(arguments[1], "status") == 0)) {
        write_response(
            service,
            "[OK] active=text xrce=roadmap cobs=disabled\n");
        return;
    }

    write_response(
        service,
        "[ERR] only text transport is implemented\n");
}

static void execute_script(
    command_service_t *service,
    size_t count,
    char **arguments)
{
    uint8_t index;

    if ((count == 2U) &&
        (strcmp(arguments[1], "begin") == 0)) {
        if (service->script_running) {
            write_response(service, "[ERR] script is running\n");
            return;
        }
        service->script_recording = true;
        service->script_line_count = 0U;
        write_response(
            service,
            "[OK] script recording max_lines=%u line_bytes=%u\n",
            (unsigned int)COMMAND_SCRIPT_MAX_LINES,
            (unsigned int)COMMAND_SERVICE_LINE_CAPACITY);
        return;
    }

    if ((count == 2U) &&
        (strcmp(arguments[1], "end") == 0)) {
        if (!service->script_recording) {
            write_response(service, "[ERR] script is not recording\n");
            return;
        }
        service->script_recording = false;
        write_response(
            service,
            "[OK] script stored lines=%u ram_only=1\n",
            (unsigned int)service->script_line_count);
        return;
    }

    if ((count == 2U) &&
        (strcmp(arguments[1], "clear") == 0)) {
        if (service->script_running) {
            script_abort(service, "clear");
        }
        script_clear_buffer(service);
        write_response(service, "[OK] script cleared\n");
        return;
    }

    if ((count == 5U) &&
        (strcmp(arguments[1], "load") == 0) &&
        (strcmp(arguments[2], "brake-sweep") == 0)) {
        int32_t drive_percent;
        int32_t drive_ms;
        int32_t brake_percent;

        if (service->script_recording) {
            write_response(service, "[ERR] script is still recording\n");
            return;
        }
        if (service->script_running) {
            write_response(service, "[ERR] script is running\n");
            return;
        }
        if (!parse_int32(arguments[3], &drive_percent) ||
            !parse_int32(arguments[4], &drive_ms)) {
            write_response(
                service,
                "[ERR] usage: script load brake-sweep <30..80> "
                "<1000..10000 ms> <10..20>\n");
            return;
        }
        brake_percent = 10;
        if (!script_load_brake_sweep(
                service,
                drive_percent,
                drive_ms,
                brake_percent)) {
            write_response(
                service,
                "[ERR] brake-sweep requires drive_pct=30..80 "
                "drive_ms=1000..10000 brake_pct=10..20\n");
            return;
        }
        write_response(
            service,
            "[OK] script loaded brake-sweep drive_pct=%ld drive_ms=%ld "
            "brake_pct=%ld repeats=%u wait_ms=%u lines=%u ram_only=1\n",
            (long)drive_percent,
            (long)drive_ms,
            (long)brake_percent,
            (unsigned int)COMMAND_SCRIPT_BRAKE_SWEEP_REPEATS,
            (unsigned int)COMMAND_SCRIPT_BRAKE_SWEEP_WAIT_MS,
            (unsigned int)service->script_line_count);
        return;
    }

    if ((count == 6U) &&
        (strcmp(arguments[1], "load") == 0) &&
        (strcmp(arguments[2], "brake-sweep") == 0)) {
        int32_t drive_percent;
        int32_t drive_ms;
        int32_t brake_percent;

        if (service->script_recording) {
            write_response(service, "[ERR] script is still recording\n");
            return;
        }
        if (service->script_running) {
            write_response(service, "[ERR] script is running\n");
            return;
        }
        if (!parse_int32(arguments[3], &drive_percent) ||
            !parse_int32(arguments[4], &drive_ms) ||
            !parse_int32(arguments[5], &brake_percent)) {
            write_response(
                service,
                "[ERR] usage: script load brake-sweep <30..80> "
                "<1000..10000 ms> <10..20>\n");
            return;
        }
        if (!script_load_brake_sweep(
                service,
                drive_percent,
                drive_ms,
                brake_percent)) {
            write_response(
                service,
                "[ERR] brake-sweep requires drive_pct=30..80 "
                "drive_ms=1000..10000 brake_pct=10..20\n");
            return;
        }
        write_response(
            service,
            "[OK] script loaded brake-sweep drive_pct=%ld drive_ms=%ld "
            "brake_pct=%ld repeats=%u wait_ms=%u lines=%u ram_only=1\n",
            (long)drive_percent,
            (long)drive_ms,
            (long)brake_percent,
            (unsigned int)COMMAND_SCRIPT_BRAKE_SWEEP_REPEATS,
            (unsigned int)COMMAND_SCRIPT_BRAKE_SWEEP_WAIT_MS,
            (unsigned int)service->script_line_count);
        return;
    }

    if ((count == 2U) &&
        (strcmp(arguments[1], "list") == 0)) {
        if (service->script_recording) {
            write_response(service, "[SCRIPT] recording=1\n");
        }
        for (index = 0U; index < service->script_line_count; index++) {
            write_response(
                service,
                "[SCRIPT] %u: %s\n",
                (unsigned int)(index + 1U),
                service->script_lines[index]);
        }
        write_response(
            service,
            "[SCRIPT] lines=%u running=%u\n",
            (unsigned int)service->script_line_count,
            service->script_running ? 1U : 0U);
        return;
    }

    if ((count == 2U) &&
        (strcmp(arguments[1], "abort") == 0)) {
        if (!service->script_running && !service->script_recording) {
            write_response(service, "[OK] script idle\n");
            return;
        }
        if (service->script_running) {
            script_abort(service, "user");
        }
        service->script_recording = false;
        write_response(service, "[OK] script recording stopped\n");
        return;
    }

    if ((count == 2U) &&
        (strcmp(arguments[1], "run") == 0)) {
        if (service->script_recording) {
            write_response(service, "[ERR] script is still recording\n");
            return;
        }
        if (service->script_running) {
            write_response(service, "[ERR] script is already running\n");
            return;
        }
        if (service->script_line_count == 0U) {
            write_response(service, "[ERR] script is empty\n");
            return;
        }
        if (!motor_test_service_is_armed(service->motor)) {
            write_response(service, "[ERR] motor is not armed\n");
            return;
        }
        if (motor_test_service_is_active(service->motor)) {
            write_response(service, "[ERR] motor operation already active\n");
            return;
        }

        service->script_running = true;
        service->script_waiting = false;
        service->script_motor_active = false;
        service->script_cursor = 0U;
        service->script_deadline_ms =
            service->motor->now_ms(service->motor->time_context) +
            COMMAND_SCRIPT_MAX_RUN_MS;
        write_response(
            service,
            "[SCRIPT] run lines=%u timeout_ms=%u\n",
            (unsigned int)service->script_line_count,
            (unsigned int)COMMAND_SCRIPT_MAX_RUN_MS);
        return;
    }

    write_response(
        service,
        "[ERR] usage: script begin|end|list|clear|run|abort|"
        "load brake-sweep <pct> <ms> [brake_pct]\n");
}

void command_service_init(
    command_service_t *service,
    runtime_parameters_t *parameters,
    telemetry_toggle_t *telemetry,
    motor_test_service_t *motor,
    command_service_read_vbus_mv_fn read_vbus_mv,
    void *vbus_context,
    command_service_get_motor_channel_fn get_motor_channel,
    command_service_set_motor_channel_fn set_motor_channel,
    void *motor_channel_context,
    command_service_write_fn write,
    void *write_context)
{
    service->parameters = parameters;
    service->telemetry = telemetry;
    service->motor = motor;
    service->read_vbus_mv = read_vbus_mv;
    service->vbus_context = vbus_context;
    service->get_motor_channel = get_motor_channel;
    service->set_motor_channel = set_motor_channel;
    service->motor_channel_context = motor_channel_context;
    service->write = write;
    service->write_context = write_context;
    service->length = 0U;
    service->script_line_count = 0U;
    service->script_cursor = 0U;
    service->script_deadline_ms = 0U;
    service->script_wait_deadline_ms = 0U;
    service->discard_until_newline = false;
    service->script_recording = false;
    service->script_running = false;
    service->script_waiting = false;
    service->script_motor_active = false;
    service->line[0] = '\0';
}

void command_service_execute_line(
    command_service_t *service,
    char *line)
{
    char *arguments[COMMAND_MAX_ARGUMENTS];
    size_t count = split_arguments(
        line,
        arguments,
        COMMAND_MAX_ARGUMENTS);

    if (count == 0U) {
        return;
    }

    if ((count == 1U) &&
        (strcmp(arguments[0], "help") == 0)) {
        write_response(
            service,
            "[OK] commands: help status script telem param transport motor\n");
    } else if ((count == 1U) &&
               (strcmp(arguments[0], "status") == 0)) {
        execute_status(service);
    } else if (strcmp(arguments[0], "script") == 0) {
        execute_script(service, count, arguments);
    } else if (strcmp(arguments[0], "telem") == 0) {
        execute_telemetry(service, count, arguments);
    } else if (strcmp(arguments[0], "param") == 0) {
        execute_parameter(service, count, arguments);
    } else if (strcmp(arguments[0], "transport") == 0) {
        execute_transport(service, count, arguments);
    } else if (strcmp(arguments[0], "motor") == 0) {
        execute_motor(service, count, arguments);
    } else {
        write_response(
            service,
            "[ERR] unknown command: %s\n",
            arguments[0]);
    }
}

void command_service_update_1ms(command_service_t *service)
{
    uint32_t now_ms;

    if (!service->script_running) {
        return;
    }

    now_ms = service->motor->now_ms(service->motor->time_context);
    if (deadline_reached(now_ms, service->script_deadline_ms)) {
        script_abort(service, "timeout");
        return;
    }

    if (service->script_waiting) {
        if (!deadline_reached(now_ms, service->script_wait_deadline_ms)) {
            return;
        }
        service->script_waiting = false;
        service->script_cursor++;
    }

    if (service->script_motor_active) {
        motor_brake_state_t state;

        if (motor_test_service_is_active(service->motor)) {
            return;
        }

        state = motor_test_service_brake_state(service->motor);
        if (state != MOTOR_BRAKE_DONE) {
            script_abort(service, "motor-failed");
            return;
        }
        write_response(
            service,
            "[SCRIPT] line=%u pass\n",
            (unsigned int)(service->script_cursor + 1U));
        service->script_motor_active = false;
        service->script_cursor++;
    }

    while (service->script_running &&
           !service->script_waiting &&
           !service->script_motor_active) {
        char scratch[COMMAND_SERVICE_LINE_CAPACITY];
        char parsed[COMMAND_SERVICE_LINE_CAPACITY];
        char *arguments[COMMAND_MAX_ARGUMENTS];
        size_t count;

        if (service->script_cursor >= service->script_line_count) {
            service->script_running = false;
            service->script_cursor = 0U;
            service->script_deadline_ms = 0U;
            write_response(
                service,
                "[SCRIPT] complete lines=%u armed=0\n",
                (unsigned int)service->script_line_count);
            return;
        }

        (void)snprintf(
            parsed,
            sizeof(parsed),
            "%s",
            service->script_lines[service->script_cursor]);
        count = split_arguments(parsed, arguments, COMMAND_MAX_ARGUMENTS);

        write_response(
            service,
            "[SCRIPT] line=%u/%u start: %s\n",
            (unsigned int)(service->script_cursor + 1U),
            (unsigned int)service->script_line_count,
            service->script_lines[service->script_cursor]);

        if (is_script_allowed_wait_line(count, arguments)) {
            int32_t wait_ms = 0;

            (void)parse_int32(arguments[1], &wait_ms);
            motor_test_service_stop(service->motor);
            service->script_waiting = true;
            service->script_wait_deadline_ms = now_ms + (uint32_t)wait_ms;
            write_response(
                service,
                "[SCRIPT] line=%u wait_ms=%ld output_pct=0\n",
                (unsigned int)(service->script_cursor + 1U),
                (long)wait_ms);
            if (wait_ms == 0) {
                service->script_waiting = false;
                service->script_cursor++;
                continue;
            }
            return;
        }

        if (is_script_allowed_motor_line(count, arguments)) {
            (void)motor_test_service_arm(service->motor);
            (void)snprintf(
                scratch,
                sizeof(scratch),
                "%s",
                service->script_lines[service->script_cursor]);
            command_service_execute_line(service, scratch);
            if (!motor_test_service_is_active(service->motor)) {
                script_abort(service, "start-failed");
                return;
            }
            service->script_motor_active = true;
            return;
        }

        script_abort(service, "invalid-line");
        return;
    }
}

void command_service_feed_char(
    command_service_t *service,
    char character)
{
    if ((character == '\r') || (character == '\n')) {
        if (service->discard_until_newline) {
            service->discard_until_newline = false;
            service->length = 0U;
            return;
        }

        if (service->length > 0U) {
            service->line[service->length] = '\0';
            if (service->script_recording &&
                (strcmp(service->line, "script end") != 0) &&
                (strcmp(service->line, "script abort") != 0) &&
                (strcmp(service->line, "script clear") != 0)) {
                if (service->script_line_count >= COMMAND_SCRIPT_MAX_LINES) {
                    service->script_recording = false;
                    write_response(
                        service,
                        "[ERR] script full; recording stopped\n");
                } else if (!is_script_allowed_line(service->line)) {
                    service->script_recording = false;
                    write_response(
                        service,
                        "[ERR] script line rejected; allowed: motor brake-response, wait <ms>\n");
                } else {
                    (void)snprintf(
                        service->script_lines[service->script_line_count],
                        COMMAND_SERVICE_LINE_CAPACITY,
                        "%s",
                        service->line);
                    service->script_line_count++;
                    write_response(
                        service,
                        "[SCRIPT] stored line=%u\n",
                        (unsigned int)service->script_line_count);
                }
            } else {
                command_service_execute_line(
                    service,
                    service->line);
            }
            service->length = 0U;
        }
        return;
    }

    if (service->discard_until_newline) {
        return;
    }

    if ((character == '\b') || (character == 0x7F)) {
        if (service->length > 0U) {
            service->length--;
        }
        return;
    }

    if ((character < 0x20) || (character > 0x7E)) {
        return;
    }

    if (service->length >=
        (COMMAND_SERVICE_LINE_CAPACITY - 1U)) {
        service->discard_until_newline = true;
        write_response(
            service,
            "[ERR] command line too long\n");
        return;
    }

    service->line[service->length++] = character;
}
