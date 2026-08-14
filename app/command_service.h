#ifndef COMMAND_SERVICE_H
#define COMMAND_SERVICE_H

#include <stdint.h>

#include "motor_test_service.h"
#include "runtime_parameters.h"
#include "telemetry_toggle.h"

#define COMMAND_SERVICE_LINE_CAPACITY 96U
#define COMMAND_SCRIPT_MAX_LINES 32U
#define COMMAND_SCRIPT_MAX_RUN_MS 90000U

typedef void (*command_service_write_fn)(
    const char *text,
    void *context);

typedef uint32_t (*command_service_read_vbus_mv_fn)(void *context);
typedef const char *(*command_service_get_motor_channel_fn)(void *context);
typedef bool (*command_service_set_motor_channel_fn)(
    const char *channel,
    void *context);

/*
 * The OLED link is write-only, so no software check can tell a working panel
 * from a dead one.  These operations exist so a single flashed image can sweep
 * the settings that decide whether the panel lights, one variable at a time,
 * over the maintenance UART.
 */
typedef enum {
    COMMAND_OLED_DISPLAY_ON = 0,
    COMMAND_OLED_DISPLAY_OFF,
    COMMAND_OLED_ENTIRE_ON,
    COMMAND_OLED_RESUME_RAM,
    COMMAND_OLED_INVERT_ON,
    COMMAND_OLED_INVERT_OFF,
    COMMAND_OLED_CONTRAST,
    COMMAND_OLED_IREF,
    COMMAND_OLED_VCOM,
    COMMAND_OLED_CHARGE_PUMP,
    COMMAND_OLED_REINIT,
    COMMAND_OLED_SELF_TEST,
    COMMAND_OLED_PATTERN,
    COMMAND_OLED_RAW
} command_oled_operation_t;

typedef bool (*command_service_oled_fn)(
    command_oled_operation_t operation,
    uint8_t value,
    void *context);
typedef void (*command_service_oled_status_fn)(
    uint8_t *contrast,
    uint8_t *iref,
    uint8_t *vcom,
    void *context);

typedef struct {
    runtime_parameters_t *parameters;
    telemetry_toggle_t *telemetry;
    motor_test_service_t *motor;
    command_service_read_vbus_mv_fn read_vbus_mv;
    void *vbus_context;
    command_service_get_motor_channel_fn get_motor_channel;
    command_service_set_motor_channel_fn set_motor_channel;
    void *motor_channel_context;
    command_service_oled_fn oled;
    command_service_oled_status_fn oled_status;
    void *oled_context;
    command_service_write_fn write;
    void *write_context;
    char line[COMMAND_SERVICE_LINE_CAPACITY];
    char script_lines[COMMAND_SCRIPT_MAX_LINES][COMMAND_SERVICE_LINE_CAPACITY];
    uint8_t length;
    uint8_t script_line_count;
    uint8_t script_cursor;
    uint32_t script_deadline_ms;
    uint32_t script_wait_deadline_ms;
    bool discard_until_newline;
    bool script_recording;
    bool script_running;
    bool script_waiting;
    bool script_motor_active;
} command_service_t;

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
    command_service_oled_fn oled,
    command_service_oled_status_fn oled_status,
    void *oled_context,
    command_service_write_fn write,
    void *write_context);

void command_service_feed_char(
    command_service_t *service,
    char character);

void command_service_execute_line(
    command_service_t *service,
    char *line);

void command_service_update_1ms(command_service_t *service);

#endif
