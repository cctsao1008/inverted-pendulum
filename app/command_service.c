#include "command_service.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COMMAND_MAX_ARGUMENTS 5U
#define COMMAND_RESPONSE_CAPACITY 160U

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

static void execute_status(command_service_t *service)
{
    write_response(
        service,
        "[OK] motor=disabled transport=text telemetry=%s "
        "rate_hz=%u param_dirty=%u persistence=unavailable\n",
        telemetry_toggle_is_enabled(service->telemetry)
            ? "on"
            : "off",
        (unsigned int)service->parameters->telemetry_rate_hz,
        runtime_parameters_is_dirty(service->parameters) ? 1U : 0U);
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

void command_service_init(
    command_service_t *service,
    runtime_parameters_t *parameters,
    telemetry_toggle_t *telemetry,
    command_service_write_fn write,
    void *write_context)
{
    service->parameters = parameters;
    service->telemetry = telemetry;
    service->write = write;
    service->write_context = write_context;
    service->length = 0U;
    service->discard_until_newline = false;
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
            "[OK] commands: help status telem param transport\n");
    } else if ((count == 1U) &&
               (strcmp(arguments[0], "status") == 0)) {
        execute_status(service);
    } else if (strcmp(arguments[0], "telem") == 0) {
        execute_telemetry(service, count, arguments);
    } else if (strcmp(arguments[0], "param") == 0) {
        execute_parameter(service, count, arguments);
    } else if (strcmp(arguments[0], "transport") == 0) {
        execute_transport(service, count, arguments);
    } else {
        write_response(
            service,
            "[ERR] unknown command: %s\n",
            arguments[0]);
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
            command_service_execute_line(
                service,
                service->line);
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
