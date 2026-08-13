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

typedef void (*ssd1315_reset_fn)(void *context);
typedef void (*ssd1315_write_command_fn)(
    uint8_t command,
    void *context);
typedef void (*ssd1315_write_data_fn)(
    const uint8_t *data,
    size_t length,
    void *context);

typedef struct {
    ssd1315_reset_fn reset;
    ssd1315_write_command_fn write_command;
    ssd1315_write_data_fn write_data;
    void *context;

    uint8_t buffer[SSD1315_BUFFER_SIZE];
    uint8_t dirty_pages;
    uint8_t flush_page;
    uint8_t flush_column;
    bool flush_active;
    bool initialized;
} ssd1315_t;

bool ssd1315_init(
    ssd1315_t *display,
    ssd1315_reset_fn reset,
    ssd1315_write_command_fn write_command,
    ssd1315_write_data_fn write_data,
    void *context,
    uint8_t contrast);

void ssd1315_clear(ssd1315_t *display);
void ssd1315_write_line(
    ssd1315_t *display,
    uint8_t row,
    const char *text);
void ssd1315_set_contrast(
    ssd1315_t *display,
    uint8_t contrast);

bool ssd1315_is_idle(const ssd1315_t *display);
void ssd1315_service(
    ssd1315_t *display,
    size_t max_data_bytes);

#endif
