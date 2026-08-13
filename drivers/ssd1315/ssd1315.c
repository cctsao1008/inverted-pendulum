#include "ssd1315.h"

#include <stddef.h>
#include <string.h>

/* SSD1315 command set used by the 128x64 Forest module. */
#define SSD1315_CMD_LOWER_COLUMN             0x00U
#define SSD1315_CMD_HIGHER_COLUMN            0x10U
#define SSD1315_CMD_MEMORY_MODE              0x20U
#define SSD1315_CMD_SET_COLUMN_ADDRESS       0x21U
#define SSD1315_CMD_SET_PAGE_ADDRESS         0x22U
#define SSD1315_CMD_FADE_BLINKING            0x23U
#define SSD1315_CMD_DEACTIVATE_SCROLL        0x2EU
#define SSD1315_CMD_START_LINE               0x40U
#define SSD1315_CMD_CONTRAST                 0x81U
#define SSD1315_CMD_CHARGE_PUMP              0x8DU
#define SSD1315_CMD_SEGMENT_REMAP            0xA1U
#define SSD1315_CMD_RESUME_RAM               0xA4U
#define SSD1315_CMD_NORMAL_DISPLAY           0xA6U
#define SSD1315_CMD_MULTIPLEX                0xA8U
#define SSD1315_CMD_INTERNAL_IREF            0xADU
#define SSD1315_CMD_DISPLAY_OFF              0xAEU
#define SSD1315_CMD_DISPLAY_ON               0xAFU
#define SSD1315_CMD_PAGE_BASE                0xB0U
#define SSD1315_CMD_COM_SCAN_DEC             0xC8U
#define SSD1315_CMD_DISPLAY_OFFSET           0xD3U
#define SSD1315_CMD_DISPLAY_CLOCK            0xD5U
#define SSD1315_CMD_ZOOM                     0xD6U
#define SSD1315_CMD_PRECHARGE                0xD9U
#define SSD1315_CMD_COM_PINS                 0xDAU
#define SSD1315_CMD_VCOM_DESELECT            0xDBU

/*
 * SSD1315 basic profile, cross-checked against libdriver/ssd1315:
 * page addressing, 128x64 multiplex, internal IREF enabled at 19uA,
 * alternative COM pins, VCOMH=0.77*VCC and 7.5V charge-pump mode.
 */
#define SSD1315_IREF_INTERNAL_19UA           0x10U
#define SSD1315_COM_PINS_ALTERNATIVE        0x12U
#define SSD1315_VCOM_0P77                    0x20U
#define SSD1315_CHARGE_PUMP_7P5V_ON         0x14U
#define SSD1315_CLOCK_DEFAULT                0x80U
#define SSD1315_PRECHARGE_DEFAULT            0xF1U

static const uint8_t digit_font[10][5] = {
    {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU},
    {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U},
    {0x42U, 0x61U, 0x51U, 0x49U, 0x46U},
    {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U},
    {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U},
    {0x27U, 0x45U, 0x45U, 0x45U, 0x39U},
    {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U},
    {0x01U, 0x71U, 0x09U, 0x05U, 0x03U},
    {0x36U, 0x49U, 0x49U, 0x49U, 0x36U},
    {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU}
};

static const uint8_t upper_font[26][5] = {
    {0x7EU, 0x11U, 0x11U, 0x11U, 0x7EU},
    {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U},
    {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U},
    {0x7FU, 0x41U, 0x41U, 0x22U, 0x1CU},
    {0x7FU, 0x49U, 0x49U, 0x49U, 0x41U},
    {0x7FU, 0x09U, 0x09U, 0x09U, 0x01U},
    {0x3EU, 0x41U, 0x49U, 0x49U, 0x7AU},
    {0x7FU, 0x08U, 0x08U, 0x08U, 0x7FU},
    {0x00U, 0x41U, 0x7FU, 0x41U, 0x00U},
    {0x20U, 0x40U, 0x41U, 0x3FU, 0x01U},
    {0x7FU, 0x08U, 0x14U, 0x22U, 0x41U},
    {0x7FU, 0x40U, 0x40U, 0x40U, 0x40U},
    {0x7FU, 0x02U, 0x0CU, 0x02U, 0x7FU},
    {0x7FU, 0x04U, 0x08U, 0x10U, 0x7FU},
    {0x3EU, 0x41U, 0x41U, 0x41U, 0x3EU},
    {0x7FU, 0x09U, 0x09U, 0x09U, 0x06U},
    {0x3EU, 0x41U, 0x51U, 0x21U, 0x5EU},
    {0x7FU, 0x09U, 0x19U, 0x29U, 0x46U},
    {0x46U, 0x49U, 0x49U, 0x49U, 0x31U},
    {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U},
    {0x3FU, 0x40U, 0x40U, 0x40U, 0x3FU},
    {0x1FU, 0x20U, 0x40U, 0x20U, 0x1FU},
    {0x3FU, 0x40U, 0x38U, 0x40U, 0x3FU},
    {0x63U, 0x14U, 0x08U, 0x14U, 0x63U},
    {0x07U, 0x08U, 0x70U, 0x08U, 0x07U},
    {0x61U, 0x51U, 0x49U, 0x45U, 0x43U}
};

static void copy_glyph(uint8_t glyph[5], const uint8_t source[5])
{
    uint8_t index;

    for (index = 0U; index < 5U; index++) {
        glyph[index] = source[index];
    }
}

static void glyph_for_character(char character, uint8_t glyph[5])
{
    static const uint8_t blank[5] = {0U, 0U, 0U, 0U, 0U};
    uint8_t punctuation[5] = {0U, 0U, 0U, 0U, 0U};

    if ((character >= 'a') && (character <= 'z')) {
        character = (char)(character - 'a' + 'A');
    }
    if ((character >= '0') && (character <= '9')) {
        copy_glyph(glyph, digit_font[(uint8_t)(character - '0')]);
        return;
    }
    if ((character >= 'A') && (character <= 'Z')) {
        copy_glyph(glyph, upper_font[(uint8_t)(character - 'A')]);
        return;
    }

    switch (character) {
    case '+':
        punctuation[0] = 0x08U; punctuation[1] = 0x08U;
        punctuation[2] = 0x3EU; punctuation[3] = 0x08U;
        punctuation[4] = 0x08U;
        break;
    case '-':
        punctuation[0] = 0x08U; punctuation[1] = 0x08U;
        punctuation[2] = 0x08U; punctuation[3] = 0x08U;
        punctuation[4] = 0x08U;
        break;
    case '.':
        punctuation[1] = 0x60U; punctuation[2] = 0x60U;
        break;
    case '/':
        punctuation[0] = 0x20U; punctuation[1] = 0x10U;
        punctuation[2] = 0x08U; punctuation[3] = 0x04U;
        punctuation[4] = 0x02U;
        break;
    case ':':
        punctuation[1] = 0x36U; punctuation[2] = 0x36U;
        break;
    case '%':
        punctuation[0] = 0x63U; punctuation[1] = 0x13U;
        punctuation[2] = 0x08U; punctuation[3] = 0x64U;
        punctuation[4] = 0x63U;
        break;
    case '=':
        punctuation[0] = 0x14U; punctuation[1] = 0x14U;
        punctuation[2] = 0x14U; punctuation[3] = 0x14U;
        punctuation[4] = 0x14U;
        break;
    case '_':
        punctuation[0] = 0x40U; punctuation[1] = 0x40U;
        punctuation[2] = 0x40U; punctuation[3] = 0x40U;
        punctuation[4] = 0x40U;
        break;
    case ' ':
        copy_glyph(glyph, blank);
        return;
    default:
        punctuation[0] = 0x02U; punctuation[1] = 0x01U;
        punctuation[2] = 0x51U; punctuation[3] = 0x09U;
        punctuation[4] = 0x06U;
        break;
    }

    copy_glyph(glyph, punctuation);
}

static void write_command_pair(
    ssd1315_t *display,
    uint8_t command,
    uint8_t value)
{
    display->write_command(command, display->context);
    display->write_command(value, display->context);
}

static void write_command_triplet(
    ssd1315_t *display,
    uint8_t command,
    uint8_t value0,
    uint8_t value1)
{
    display->write_command(command, display->context);
    display->write_command(value0, display->context);
    display->write_command(value1, display->context);
}

bool ssd1315_init(
    ssd1315_t *display,
    ssd1315_reset_fn reset,
    ssd1315_write_command_fn write_command,
    ssd1315_write_data_fn write_data,
    void *context,
    uint8_t contrast)
{
    if ((display == NULL) ||
        (reset == NULL) ||
        (write_command == NULL) ||
        (write_data == NULL)) {
        return false;
    }

    memset(display, 0, sizeof(*display));
    display->reset = reset;
    display->write_command = write_command;
    display->write_data = write_data;
    display->context = context;

    display->reset(display->context);

    display->write_command(SSD1315_CMD_DISPLAY_OFF, display->context);
    write_command_triplet(display, SSD1315_CMD_SET_COLUMN_ADDRESS, 0x00U, 0x7FU);
    write_command_triplet(display, SSD1315_CMD_SET_PAGE_ADDRESS, 0x00U, 0x07U);
    display->write_command(SSD1315_CMD_START_LINE, display->context);
    write_command_pair(display, SSD1315_CMD_FADE_BLINKING, 0x00U);
    display->write_command(SSD1315_CMD_DEACTIVATE_SCROLL, display->context);
    write_command_pair(display, SSD1315_CMD_ZOOM, 0x00U);
    write_command_pair(display, SSD1315_CMD_CONTRAST, contrast);
    display->write_command(SSD1315_CMD_SEGMENT_REMAP, display->context);
    display->write_command(SSD1315_CMD_COM_SCAN_DEC, display->context);
    display->write_command(SSD1315_CMD_NORMAL_DISPLAY, display->context);
    write_command_pair(display, SSD1315_CMD_MULTIPLEX, 0x3FU);
    write_command_pair(display, SSD1315_CMD_DISPLAY_OFFSET, 0x00U);
    write_command_pair(display, SSD1315_CMD_DISPLAY_CLOCK, SSD1315_CLOCK_DEFAULT);
    write_command_pair(display, SSD1315_CMD_PRECHARGE, SSD1315_PRECHARGE_DEFAULT);
    write_command_pair(display, SSD1315_CMD_INTERNAL_IREF, SSD1315_IREF_INTERNAL_19UA);
    write_command_pair(display, SSD1315_CMD_COM_PINS, SSD1315_COM_PINS_ALTERNATIVE);
    write_command_pair(display, SSD1315_CMD_VCOM_DESELECT, SSD1315_VCOM_0P77);
    write_command_pair(display, SSD1315_CMD_MEMORY_MODE, 0x02U);
    write_command_pair(display, SSD1315_CMD_CHARGE_PUMP, SSD1315_CHARGE_PUMP_7P5V_ON);
    display->write_command(SSD1315_CMD_RESUME_RAM, display->context);
    display->write_command(SSD1315_CMD_DISPLAY_ON, display->context);

    ssd1315_clear(display);
    display->initialized = true;
    return true;
}

void ssd1315_clear(ssd1315_t *display)
{
    if (display == NULL) {
        return;
    }

    memset(display->buffer, 0, sizeof(display->buffer));
    display->dirty_pages = 0xFFU;
    display->flush_active = false;
    display->flush_page = 0U;
    display->flush_column = 0U;
}

void ssd1315_write_line(
    ssd1315_t *display,
    uint8_t row,
    const char *text)
{
    uint8_t row_buffer[SSD1315_WIDTH] = {0U};
    size_t character_index;
    size_t text_length;
    uint32_t base;

    if ((display == NULL) ||
        (text == NULL) ||
        (row >= SSD1315_PAGE_COUNT)) {
        return;
    }

    text_length = strlen(text);
    if (text_length > SSD1315_LINE_COLUMNS) {
        text_length = SSD1315_LINE_COLUMNS;
    }

    for (character_index = 0U;
         character_index < text_length;
         character_index++) {
        uint8_t glyph[5];
        size_t column;
        size_t offset = character_index * 6U;

        glyph_for_character(text[character_index], glyph);
        for (column = 0U; column < 5U; column++) {
            row_buffer[offset + column] = glyph[column];
        }
    }

    base = (uint32_t)row * SSD1315_WIDTH;
    if (memcmp(&display->buffer[base], row_buffer, SSD1315_WIDTH) != 0) {
        memcpy(&display->buffer[base], row_buffer, SSD1315_WIDTH);
        display->dirty_pages |= (uint8_t)(1U << row);
    }
}

void ssd1315_set_contrast(
    ssd1315_t *display,
    uint8_t contrast)
{
    if ((display == NULL) || !display->initialized) {
        return;
    }

    write_command_pair(display, SSD1315_CMD_CONTRAST, contrast);
}

bool ssd1315_is_idle(const ssd1315_t *display)
{
    return (display != NULL) &&
           (display->dirty_pages == 0U) &&
           !display->flush_active;
}

void ssd1315_service(
    ssd1315_t *display,
    size_t max_data_bytes)
{
    size_t remaining;
    size_t transfer;
    uint32_t base;
    uint8_t page;

    if ((display == NULL) ||
        !display->initialized ||
        (max_data_bytes == 0U)) {
        return;
    }

    if (!display->flush_active) {
        for (page = 0U; page < SSD1315_PAGE_COUNT; page++) {
            if ((display->dirty_pages & (uint8_t)(1U << page)) != 0U) {
                display->flush_page = page;
                display->flush_column = 0U;
                display->flush_active = true;
                display->write_command(
                    (uint8_t)(SSD1315_CMD_PAGE_BASE | page),
                    display->context);
                display->write_command(SSD1315_CMD_LOWER_COLUMN, display->context);
                display->write_command(SSD1315_CMD_HIGHER_COLUMN, display->context);
                break;
            }
        }
    }

    if (!display->flush_active) {
        return;
    }

    remaining = SSD1315_WIDTH - display->flush_column;
    transfer = (remaining < max_data_bytes)
        ? remaining
        : max_data_bytes;
    base = ((uint32_t)display->flush_page * SSD1315_WIDTH) +
           display->flush_column;

    display->write_data(
        &display->buffer[base],
        transfer,
        display->context);

    display->flush_column =
        (uint8_t)(display->flush_column + transfer);

    if (display->flush_column >= SSD1315_WIDTH) {
        display->dirty_pages &=
            (uint8_t)~(uint8_t)(1U << display->flush_page);
        display->flush_active = false;
        display->flush_column = 0U;
    }
}
