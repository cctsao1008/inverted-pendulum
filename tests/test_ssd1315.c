#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "ssd1315.h"

typedef struct {
    unsigned int reset_count;
    unsigned int command_count;
    unsigned int delay_count;
    uint32_t delay_total_ms;
    size_t data_bytes;
    uint8_t commands[128];
    /* Command index at which each delay landed, for ordering assertions. */
    unsigned int delay_positions[8];
} bus_probe_t;

static void probe_reset(void *context)
{
    bus_probe_t *probe = (bus_probe_t *)context;
    probe->reset_count++;
}

static void probe_delay(uint32_t milliseconds, void *context)
{
    bus_probe_t *probe = (bus_probe_t *)context;

    if (probe->delay_count <
        (sizeof(probe->delay_positions) / sizeof(probe->delay_positions[0]))) {
        probe->delay_positions[probe->delay_count] = probe->command_count;
    }
    probe->delay_count++;
    probe->delay_total_ms += milliseconds;
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

static int find_command(
    const bus_probe_t *probe,
    uint8_t command)
{
    unsigned int index;

    for (index = 0U; index < probe->command_count; index++) {
        if (probe->commands[index] == command) {
            return (int)index;
        }
    }

    return -1;
}

static int find_pair(
    const bus_probe_t *probe,
    uint8_t command,
    uint8_t value)
{
    unsigned int index;

    for (index = 0U; index + 1U < probe->command_count; index++) {
        if ((probe->commands[index] == command) &&
            (probe->commands[index + 1U] == value)) {
            return (int)index;
        }
    }

    return -1;
}

static bool saw_pair(
    const bus_probe_t *probe,
    uint8_t command,
    uint8_t value)
{
    return find_pair(probe, command, value) >= 0;
}

static void test_init_sequence(void)
{
    ssd1315_t display;
    bus_probe_t probe = {0};
    int memory_mode_index;

    assert(ssd1315_init(
        &display,
        probe_reset,
        probe_delay,
        probe_command,
        probe_data,
        &probe,
        0xCFU));
    assert(probe.reset_count == 1U);

    /*
     * Page addressing must be established before anything that depends on it.
     * The range-setting commands of the other addressing modes have no meaning
     * here and must not appear at all.
     */
    memory_mode_index = find_pair(&probe, 0x20U, 0x02U);
    assert(memory_mode_index >= 0);
    assert(find_command(&probe, 0x21U) < 0);
    assert(find_command(&probe, 0x22U) < 0);

    assert(saw_pair(&probe, 0xADU, SSD1315_DEFAULT_IREF));
    assert(saw_pair(&probe, 0x8DU, SSD1315_CHARGE_PUMP_ON));
    assert(saw_pair(&probe, 0xDBU, SSD1315_DEFAULT_VCOM));
    assert(saw_pair(&probe, 0x81U, 0xCFU));

    assert(ssd1315_get_contrast(&display) == 0xCFU);
    assert(ssd1315_get_iref(&display) == SSD1315_DEFAULT_IREF);
    assert(ssd1315_get_vcom(&display) == SSD1315_DEFAULT_VCOM);
}

static void test_init_tolerates_missing_delay(void)
{
    ssd1315_t display;
    bus_probe_t probe = {0};

    /* The delay callback is optional; host builds pass NULL. */
    assert(ssd1315_init(
        &display,
        probe_reset,
        NULL,
        probe_command,
        probe_data,
        &probe,
        0x80U));
    assert(probe.reset_count == 1U);
    assert(probe.delay_count == 0U);
}

static void test_flush_and_dirty_tracking(void)
{
    ssd1315_t display;
    bus_probe_t probe = {0};
    unsigned int guard = 0U;

    assert(ssd1315_init(
        &display,
        probe_reset,
        probe_delay,
        probe_command,
        probe_data,
        &probe,
        0xCFU));
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
}

static void test_bring_up_controls(void)
{
    ssd1315_t display;
    bus_probe_t probe = {0};
    unsigned int reset_after_init;

    assert(ssd1315_init(
        &display,
        probe_reset,
        probe_delay,
        probe_command,
        probe_data,
        &probe,
        0xCFU));
    reset_after_init = probe.reset_count;

    ssd1315_set_contrast(&display, 0x80U);
    assert(saw_pair(&probe, 0x81U, 0x80U));
    assert(ssd1315_get_contrast(&display) == 0x80U);

    ssd1315_set_iref(&display, SSD1315_IREF_INTERNAL_30UA);
    assert(saw_pair(&probe, 0xADU, SSD1315_IREF_INTERNAL_30UA));
    assert(ssd1315_get_iref(&display) == SSD1315_IREF_INTERNAL_30UA);

    ssd1315_set_vcom(&display, SSD1315_VCOM_0P83);
    assert(saw_pair(&probe, 0xDBU, SSD1315_VCOM_0P83));
    assert(ssd1315_get_vcom(&display) == SSD1315_VCOM_0P83);

    ssd1315_send_command(&display, SSD1315_CMD_ENTIRE_ON);
    assert(find_command(&probe, SSD1315_CMD_ENTIRE_ON) >= 0);

    /* A reinit replays reset and re-sends whatever the sweep last selected. */
    probe.command_count = 0U;
    assert(ssd1315_reinit(&display));
    assert(probe.reset_count == reset_after_init + 1U);
    assert(saw_pair(&probe, 0xADU, SSD1315_IREF_INTERNAL_30UA));
    assert(saw_pair(&probe, 0xDBU, SSD1315_VCOM_0P83));
    assert(saw_pair(&probe, 0x81U, 0x80U));
}

static void test_self_test_returns_to_ram(void)
{
    ssd1315_t display;
    bus_probe_t probe = {0};

    assert(ssd1315_init(
        &display,
        probe_reset,
        probe_delay,
        probe_command,
        probe_data,
        &probe,
        0xCFU));

    probe.command_count = 0U;
    ssd1315_self_test(&display);

    assert(probe.commands[0] == SSD1315_CMD_ENTIRE_ON);
    /* Must leave the panel driven by RAM, not stuck in entire-on. */
    assert(probe.commands[probe.command_count - 1U] ==
           SSD1315_CMD_RESUME_RAM);
    assert(find_command(&probe, SSD1315_CMD_DISPLAY_OFF) >= 0);
}

static void test_data_path_pattern(void)
{
    ssd1315_t display;
    bus_probe_t probe = {0};
    size_t before;

    assert(ssd1315_init(
        &display,
        probe_reset,
        probe_delay,
        probe_command,
        probe_data,
        &probe,
        0xCFU));

    before = probe.data_bytes;
    ssd1315_test_pattern(&display);
    assert(probe.data_bytes == before + SSD1315_BUFFER_SIZE);
    assert(probe.delay_total_ms >= 1000U);
    assert(!ssd1315_is_idle(&display));
}

static void test_rejects_missing_callbacks(void)
{
    ssd1315_t display;
    bus_probe_t probe = {0};

    assert(!ssd1315_init(
        &display,
        NULL,
        probe_delay,
        probe_command,
        probe_data,
        &probe,
        0xCFU));
    assert(!ssd1315_init(
        &display,
        probe_reset,
        probe_delay,
        NULL,
        probe_data,
        &probe,
        0xCFU));
}

int main(void)
{
    test_init_sequence();
    test_init_tolerates_missing_delay();
    test_flush_and_dirty_tracking();
    test_bring_up_controls();
    test_self_test_returns_to_ram();
    test_data_path_pattern();
    test_rejects_missing_callbacks();

    return 0;
}
