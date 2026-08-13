#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SSD1306_WIDTH       128U
#define SSD1306_HEIGHT       64U
#define SSD1306_PAGE_COUNT    8U
#define SSD1306_LINE_COLUMNS 21U
#define SSD1306_BUFFER_SIZE  (SSD1306_WIDTH * SSD1306_PAGE_COUNT)

typedef void (*ssd1306_reset_fn)(void *context);
typedef void (*ssd1306_write_command_fn)(
    uint8_t command,
    void *context);
typedef void (*ssd1306_write_data_fn)(
    const uint8_t *data,
    size_t length,
    void *context);

typedef struct {
    ssd1306_reset_fn reset;
    ssd1306_write_command_fn write_command;
    ssd1306_write_data_fn write_data;
    void *context;

    uint8_t buffer[SSD1306_BUFFER_SIZE];
    uint8_t dirty_pages;
    uint8_t flush_page;
    uint8_t flush_column;
    bool flush_active;
    bool initialized;
} ssd1306_t;

bool ssd1306_init(
    ssd1306_t *display,
    ssd1306_reset_fn reset,
    ssd1306_write_command_fn write_command,
    ssd1306_write_data_fn write_data,
    void *context,
    uint8_t contrast);

void ssd1306_clear(ssd1306_t *display);
void ssd1306_write_line(
    ssd1306_t *display,
    uint8_t row,
    const char *text);
void ssd1306_set_contrast(
    ssd1306_t *display,
    uint8_t contrast);

bool ssd1306_is_idle(const ssd1306_t *display);
void ssd1306_service(
    ssd1306_t *display,
    size_t max_data_bytes);

#endif
