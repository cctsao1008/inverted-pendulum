#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "ssd1315.h"

typedef struct {
    unsigned int reset_count;
    unsigned int command_count;
    size_t data_bytes;
    uint8_t commands[128];
} bus_probe_t;

static void probe_reset(void *context)
{
    bus_probe_t *probe = (bus_probe_t *)context;
    probe->reset_count++;
}

static void probe_command(uint8_t command, void *context)
{
    bus_probe_t *probe = (bus_probe_t *)context;

    if (probe->command_count < sizeof(probe->commands)) {
        probe->commands[probe->command_count] = command;
    }
    probe->command_count++;
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

static bool saw_pair(
    const bus_probe_t *probe,
    uint8_t command,
    uint8_t value)
{
    unsigned int index;

    for (index = 0U; index + 1U < probe->command_count; index++) {
        if ((probe->commands[index] == command) &&
            (probe->commands[index + 1U] == value)) {
            return true;
        }
    }

    return false;
}

int main(void)
{
    ssd1315_t display;
    bus_probe_t probe = {0};
    unsigned int guard = 0U;

    assert(ssd1315_init(
        &display,
        probe_reset,
        probe_command,
        probe_data,
        &probe,
        0xCFU));
    assert(probe.reset_count == 1U);
    assert(saw_pair(&probe, 0xADU, 0x10U));
    assert(saw_pair(&probe, 0x8DU, 0x14U));
    assert(saw_pair(&probe, 0xDBU, 0x20U));
    assert(!ssd1315_is_idle(&display));

    while (!ssd1315_is_idle(&display) && (guard < 64U)) {
        ssd1315_service(&display, 32U);
        guard++;
    }

    assert(ssd1315_is_idle(&display));
    assert(probe.data_bytes == SSD1315_BUFFER_SIZE);

    ssd1315_write_line(&display, 2U, "HELLO 123");
    assert((display.dirty_pages & (1U << 2)) != 0U);

    ssd1315_service(&display, 32U);
    assert(probe.data_bytes == SSD1315_BUFFER_SIZE + 32U);

    ssd1315_set_contrast(&display, 0x80U);
    assert(saw_pair(&probe, 0x81U, 0x80U));

    return 0;
}
