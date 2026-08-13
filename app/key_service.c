#include "key_service.h"

#include <stddef.h>

static uint32_t key_bit(uint8_t key)
{
    return 1UL << key;
}

void key_service_init(
    key_service_t *service,
    uint8_t key_count,
    uint32_t initial_pressed_mask,
    uint32_t current_tick)
{
    uint8_t key;
    uint32_t valid_mask;

    if (service == NULL) {
        return;
    }

    if (key_count > KEY_SERVICE_MAX_KEYS) {
        key_count = KEY_SERVICE_MAX_KEYS;
    }

    valid_mask = (key_count == 32U)
        ? UINT32_MAX
        : ((1UL << key_count) - 1UL);

    service->key_count = key_count;
    service->stable_mask = initial_pressed_mask & valid_mask;
    service->candidate_mask = service->stable_mask;
    service->long_sent_mask = 0U;

    for (key = 0U; key < KEY_SERVICE_MAX_KEYS; key++) {
        service->candidate_since_tick[key] = current_tick;
        service->pressed_since_tick[key] = current_tick;
        service->last_repeat_tick[key] = current_tick;
    }
}

key_service_events_t key_service_update(
    key_service_t *service,
    uint32_t raw_pressed_mask,
    uint32_t current_tick)
{
    key_service_events_t events = {0U, 0U, 0U, 0U};
    uint8_t key;

    if (service == NULL) {
        return events;
    }

    for (key = 0U; key < service->key_count; key++) {
        uint32_t bit = key_bit(key);
        bool raw_pressed = (raw_pressed_mask & bit) != 0U;
        bool candidate_pressed =
            (service->candidate_mask & bit) != 0U;
        bool stable_pressed =
            (service->stable_mask & bit) != 0U;

        if (raw_pressed != candidate_pressed) {
            if (raw_pressed) {
                service->candidate_mask |= bit;
            } else {
                service->candidate_mask &= ~bit;
            }
            service->candidate_since_tick[key] = current_tick;
            continue;
        }

        if (candidate_pressed != stable_pressed) {
            if ((uint32_t)(current_tick -
                    service->candidate_since_tick[key]) <
                KEY_SERVICE_DEBOUNCE_TICKS) {
                continue;
            }

            if (candidate_pressed) {
                service->stable_mask |= bit;
                service->pressed_since_tick[key] = current_tick;
                service->last_repeat_tick[key] = current_tick;
                service->long_sent_mask &= ~bit;
                events.pressed |= bit;
            } else {
                service->stable_mask &= ~bit;
                service->long_sent_mask &= ~bit;
                events.released |= bit;
            }
            continue;
        }

        if (!stable_pressed) {
            continue;
        }

        if ((service->long_sent_mask & bit) == 0U) {
            if ((uint32_t)(current_tick -
                    service->pressed_since_tick[key]) >=
                KEY_SERVICE_LONG_PRESS_TICKS) {
                service->long_sent_mask |= bit;
                events.long_pressed |= bit;
            }
        }

        if ((uint32_t)(current_tick -
                service->pressed_since_tick[key]) >=
            KEY_SERVICE_REPEAT_DELAY_TICKS) {
            if ((uint32_t)(current_tick -
                    service->last_repeat_tick[key]) >=
                KEY_SERVICE_REPEAT_TICKS) {
                service->last_repeat_tick[key] = current_tick;
                events.repeat |= bit;
            }
        }
    }

    return events;
}
