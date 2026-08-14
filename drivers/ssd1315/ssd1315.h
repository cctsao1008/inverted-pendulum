#ifndef SSD1315_H
#define SSD1315_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SSD1315_WIDTH       128U
#define SSD1315_HEIGHT       64U
#define SSD1315_PAGE_COUNT    8U
#define SSD1315_LINE_COLUMNS 21U
#define SSD1315_BUFFER_SIZE  (SSD1315_WIDTH * SSD1315_PAGE_COUNT)

/*
 * Commands that decide whether the panel lights at all, exported so the boot
 * banner and the maintenance CLI can name the same values the init sequence
 * programs.  This bus is write-only: nothing can be read back from the panel,
 * so the log and the CLI are the only record of what it was told.
 */
#define SSD1315_CMD_CONTRAST        0x81U
#define SSD1315_CMD_CHARGE_PUMP     0x8DU
#define SSD1315_CMD_RESUME_RAM      0xA4U
#define SSD1315_CMD_ENTIRE_ON       0xA5U
#define SSD1315_CMD_NORMAL_DISPLAY  0xA6U
#define SSD1315_CMD_INVERT_DISPLAY  0xA7U
#define SSD1315_CMD_INTERNAL_IREF   0xADU
#define SSD1315_CMD_DISPLAY_OFF     0xAEU
#define SSD1315_CMD_DISPLAY_ON      0xAFU
#define SSD1315_CMD_VCOM_DESELECT   0xDBU

#define SSD1315_IREF_EXTERNAL       0x00U
#define SSD1315_IREF_INTERNAL_19UA  0x10U
#define SSD1315_IREF_INTERNAL_30UA  0x30U

#define SSD1315_VCOM_0P65           0x00U
#define SSD1315_VCOM_0P77           0x20U
#define SSD1315_VCOM_0P83           0x30U

#define SSD1315_CHARGE_PUMP_ON      0x14U
#define SSD1315_CHARGE_PUMP_OFF     0x10U

/*
 * Profile defaults.  These remain unproven against the physically populated
 * module: the panel offers no readback, so the only way to confirm them is to
 * sweep them on the assembled unit with the `oled` maintenance command and
 * record which combination lights.
 */
#define SSD1315_DEFAULT_IREF        SSD1315_IREF_INTERNAL_19UA
#define SSD1315_DEFAULT_VCOM        SSD1315_VCOM_0P77

typedef void (*ssd1315_reset_fn)(void *context);
typedef void (*ssd1315_delay_ms_fn)(
    uint32_t milliseconds,
    void *context);
typedef void (*ssd1315_write_command_fn)(
    uint8_t command,
    void *context);
typedef void (*ssd1315_write_data_fn)(
    const uint8_t *data,
    size_t length,
    void *context);

typedef struct {
    ssd1315_reset_fn reset;
    ssd1315_delay_ms_fn delay_ms;
    ssd1315_write_command_fn write_command;
    ssd1315_write_data_fn write_data;
    void *context;

    uint8_t buffer[SSD1315_BUFFER_SIZE];
    uint8_t contrast;
    uint8_t iref;
    uint8_t vcom;
    uint8_t dirty_pages;
    uint8_t flush_page;
    uint8_t flush_column;
    bool flush_active;
    bool initialized;
} ssd1315_t;

bool ssd1315_init(
    ssd1315_t *display,
    ssd1315_reset_fn reset,
    ssd1315_delay_ms_fn delay_ms,
    ssd1315_write_command_fn write_command,
    ssd1315_write_data_fn write_data,
    void *context,
    uint8_t contrast);

/* Replay the reset and init sequence with the currently programmed profile. */
bool ssd1315_reinit(ssd1315_t *display);

/*
 * Drive every segment on, blank the panel, then return to RAM-driven display.
 * With no readback available this proves nothing on its own; it only makes the
 * command path observable to a human watching the panel.
 */
void ssd1315_self_test(ssd1315_t *display);

/* Exercise command plus D/C/data/addressing with a visible checkerboard. */
void ssd1315_test_pattern(ssd1315_t *display);

void ssd1315_clear(ssd1315_t *display);
void ssd1315_write_line(
    ssd1315_t *display,
    uint8_t row,
    const char *text);
void ssd1315_set_contrast(
    ssd1315_t *display,
    uint8_t contrast);
void ssd1315_set_iref(
    ssd1315_t *display,
    uint8_t iref);
void ssd1315_set_vcom(
    ssd1315_t *display,
    uint8_t vcom);

uint8_t ssd1315_get_contrast(const ssd1315_t *display);
uint8_t ssd1315_get_iref(const ssd1315_t *display);
uint8_t ssd1315_get_vcom(const ssd1315_t *display);

/* Escape hatches for bring-up; the maintenance CLI drives the panel here. */
void ssd1315_send_command(
    ssd1315_t *display,
    uint8_t command);
void ssd1315_send_command_pair(
    ssd1315_t *display,
    uint8_t command,
    uint8_t value);

bool ssd1315_is_idle(const ssd1315_t *display);
void ssd1315_service(
    ssd1315_t *display,
    size_t max_data_bytes);

#endif
