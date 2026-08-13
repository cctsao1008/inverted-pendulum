#include <assert.h>

#include "key_service.h"

int main(void)
{
    key_service_t service;
    key_service_events_t events;
    const uint32_t key0 = 1UL << 0;

    key_service_init(&service, 5U, 0U, 0U);

    events = key_service_update(&service, key0, 1U);
    assert(events.pressed == 0U);

    events = key_service_update(
        &service,
        key0,
        1U + KEY_SERVICE_DEBOUNCE_TICKS);
    assert((events.pressed & key0) != 0U);

    events = key_service_update(
        &service,
        key0,
        1U + KEY_SERVICE_DEBOUNCE_TICKS +
            KEY_SERVICE_REPEAT_DELAY_TICKS);
    assert((events.repeat & key0) != 0U);

    events = key_service_update(
        &service,
        key0,
        1U + KEY_SERVICE_DEBOUNCE_TICKS +
            KEY_SERVICE_LONG_PRESS_TICKS);
    assert((events.long_pressed & key0) != 0U);

    events = key_service_update(&service, 0U, 900U);
    assert(events.released == 0U);

    events = key_service_update(
        &service,
        0U,
        900U + KEY_SERVICE_DEBOUNCE_TICKS);
    assert((events.released & key0) != 0U);

    return 0;
}
