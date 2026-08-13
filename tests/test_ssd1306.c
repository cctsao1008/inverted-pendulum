#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "ssd1306.h"

typedef struct {
    unsigned int reset_count;
    unsigned int command_count;
    size_t data_bytes;
    uint8_t last_command;
} bus_probe_t;

static void probe_reset(void *context)
{
    bus_probe_t *probe = (bus_probe_t *)context;
    probe->reset_count++;
}

static void probe_command(uint8_t command, void *context)
{
    bus_probe_t *probe = (bus_probe_t *)context;
    probe->command_count++;
    probe->last_command = command;
}

static void probe_data(
    const uint8_t *data,
    size_t length,
    void *context)
{
    bus_probe_t *probe = (bus_probe_t *)context;

    assert(data != NULL);
    probe->data_bytes += length;
}

int main(void)
{
    ssd1306_t display;
    bus_probe_t probe = {0U, 0U, 0U, 0U};
    unsigned int guard = 0U;

    assert(ssd1306_init(
        &display,
        probe_reset,
        probe_command,
        probe_data,
        &probe,
        0x7FU));
    assert(probe.reset_count == 1U);
    assert(!ssd1306_is_idle(&display));

    while (!ssd1306_is_idle(&display) && (guard < 64U)) {
        ssd1306_service(&display, 32U);
        guard++;
    }

    assert(ssd1306_is_idle(&display));
    assert(probe.data_bytes == SSD1306_BUFFER_SIZE);

    ssd1306_write_line(&display, 2U, "HELLO 123");
    assert((display.dirty_pages & (1U << 2)) != 0U);

    ssd1306_service(&display, 32U);
    assert(probe.data_bytes == SSD1306_BUFFER_SIZE + 32U);

    ssd1306_set_contrast(&display, 0x80U);
    assert(probe.last_command == 0x80U);

    return 0;
}
