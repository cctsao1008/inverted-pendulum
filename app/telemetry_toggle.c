#include "telemetry_toggle.h"

void telemetry_toggle_init(
    telemetry_toggle_t *toggle,
    bool initial_pressed,
    uint32_t current_tick)
{
    toggle->enabled = false;
    toggle->stable_pressed = initial_pressed;
    toggle->candidate_pressed = initial_pressed;
    toggle->candidate_since_tick = current_tick;
}

bool telemetry_toggle_update(
    telemetry_toggle_t *toggle,
    bool raw_pressed,
    uint32_t current_tick,
    uint32_t debounce_ticks)
{
    if (raw_pressed != toggle->candidate_pressed) {
        toggle->candidate_pressed = raw_pressed;
        toggle->candidate_since_tick = current_tick;
        return false;
    }

    if ((toggle->candidate_pressed != toggle->stable_pressed) &&
        ((uint32_t)(current_tick - toggle->candidate_since_tick) >=
         debounce_ticks)) {
        toggle->stable_pressed = toggle->candidate_pressed;

        if (toggle->stable_pressed) {
            toggle->enabled = !toggle->enabled;
            return true;
        }
    }

    return false;
}

bool telemetry_toggle_is_enabled(const telemetry_toggle_t *toggle)
{
    return toggle->enabled;
}
