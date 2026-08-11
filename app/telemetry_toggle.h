#ifndef TELEMETRY_TOGGLE_H
#define TELEMETRY_TOGGLE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool enabled;
    bool stable_pressed;
    bool candidate_pressed;
    uint32_t candidate_since_tick;
} telemetry_toggle_t;

void telemetry_toggle_init(
    telemetry_toggle_t *toggle,
    bool initial_pressed,
    uint32_t current_tick);

bool telemetry_toggle_update(
    telemetry_toggle_t *toggle,
    bool raw_pressed,
    uint32_t current_tick,
    uint32_t debounce_ticks);

bool telemetry_toggle_is_enabled(const telemetry_toggle_t *toggle);

bool telemetry_toggle_set_enabled(
    telemetry_toggle_t *toggle,
    bool enabled);

bool telemetry_toggle_toggle(telemetry_toggle_t *toggle);

#endif
