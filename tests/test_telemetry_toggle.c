#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "telemetry_toggle.h"

#define DEBOUNCE_TICKS 50U

static bool update(
    telemetry_toggle_t *toggle,
    bool pressed,
    uint32_t tick)
{
    return telemetry_toggle_update(
        toggle,
        pressed,
        tick,
        DEBOUNCE_TICKS);
}

static void test_initial_state_is_disabled(void)
{
    telemetry_toggle_t toggle;

    telemetry_toggle_init(&toggle, false, 0U);

    assert(!telemetry_toggle_is_enabled(&toggle));
    assert(!update(&toggle, false, 100U));
}

static void test_bounce_does_not_toggle(void)
{
    telemetry_toggle_t toggle;

    telemetry_toggle_init(&toggle, false, 0U);

    assert(!update(&toggle, true, 10U));
    assert(!update(&toggle, true, 40U));
    assert(!update(&toggle, false, 45U));
    assert(!update(&toggle, true, 50U));
    assert(!update(&toggle, true, 99U));
    assert(!telemetry_toggle_is_enabled(&toggle));
}

static void test_press_edge_toggles_after_debounce(void)
{
    telemetry_toggle_t toggle;

    telemetry_toggle_init(&toggle, false, 0U);

    assert(!update(&toggle, true, 10U));
    assert(!update(&toggle, true, 59U));
    assert(update(&toggle, true, 60U));
    assert(telemetry_toggle_is_enabled(&toggle));
    assert(!update(&toggle, true, 200U));
}

static void test_release_does_not_toggle_and_next_press_disables(void)
{
    telemetry_toggle_t toggle;

    telemetry_toggle_init(&toggle, false, 0U);

    assert(!update(&toggle, true, 10U));
    assert(update(&toggle, true, 60U));

    assert(!update(&toggle, false, 70U));
    assert(!update(&toggle, false, 120U));
    assert(telemetry_toggle_is_enabled(&toggle));

    assert(!update(&toggle, true, 130U));
    assert(update(&toggle, true, 180U));
    assert(!telemetry_toggle_is_enabled(&toggle));
}

static void test_held_at_boot_requires_release_and_new_press(void)
{
    telemetry_toggle_t toggle;

    telemetry_toggle_init(&toggle, true, 0U);

    assert(!update(&toggle, true, 100U));
    assert(!telemetry_toggle_is_enabled(&toggle));

    assert(!update(&toggle, false, 110U));
    assert(!update(&toggle, false, 160U));
    assert(!update(&toggle, true, 170U));
    assert(update(&toggle, true, 220U));
    assert(telemetry_toggle_is_enabled(&toggle));
}

static void test_tick_wraparound_is_supported(void)
{
    telemetry_toggle_t toggle;

    telemetry_toggle_init(&toggle, false, UINT32_MAX - 20U);

    assert(!update(&toggle, true, UINT32_MAX - 10U));
    assert(!update(&toggle, true, 38U));
    assert(update(&toggle, true, 39U));
    assert(telemetry_toggle_is_enabled(&toggle));
}

static void test_command_style_set_and_toggle_share_state(void)
{
    telemetry_toggle_t toggle;

    telemetry_toggle_init(&toggle, false, 0U);

    assert(telemetry_toggle_set_enabled(&toggle, true));
    assert(telemetry_toggle_is_enabled(&toggle));
    assert(!telemetry_toggle_set_enabled(&toggle, true));

    assert(!telemetry_toggle_toggle(&toggle));
    assert(!telemetry_toggle_is_enabled(&toggle));
}

int main(void)
{
    test_initial_state_is_disabled();
    test_bounce_does_not_toggle();
    test_press_edge_toggles_after_debounce();
    test_release_does_not_toggle_and_next_press_disables();
    test_held_at_boot_requires_release_and_new_press();
    test_tick_wraparound_is_supported();
    test_command_style_set_and_toggle_share_state();

    printf("PASS: telemetry toggle tests\n");
    return 0;
}
