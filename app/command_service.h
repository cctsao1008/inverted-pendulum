#ifndef COMMAND_SERVICE_H
#define COMMAND_SERVICE_H

#include <stdint.h>

#include "runtime_parameters.h"
#include "telemetry_toggle.h"

#define COMMAND_SERVICE_LINE_CAPACITY 96U

typedef void (*command_service_write_fn)(
    const char *text,
    void *context);

typedef struct {
    runtime_parameters_t *parameters;
    telemetry_toggle_t *telemetry;
    command_service_write_fn write;
    void *write_context;
    char line[COMMAND_SERVICE_LINE_CAPACITY];
    uint8_t length;
    bool discard_until_newline;
} command_service_t;

void command_service_init(
    command_service_t *service,
    runtime_parameters_t *parameters,
    telemetry_toggle_t *telemetry,
    command_service_write_fn write,
    void *write_context);

void command_service_feed_char(
    command_service_t *service,
    char character);

void command_service_execute_line(
    command_service_t *service,
    char *line);

#endif
