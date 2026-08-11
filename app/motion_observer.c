#include "motion_observer.h"

#include <stddef.h>

static int32_t encoder_delta_16(int32_t current, int32_t previous)
{
    return (int32_t)(int16_t)((uint16_t)current - (uint16_t)previous);
}

void motion_observer_init(motion_observer_t *observer, int32_t raw_count)
{
    uint32_t index;

    observer->previous_raw_count = raw_count;
    observer->position_counts = 0;
    for (index = 0U; index < MOTION_OBSERVER_HISTORY_LENGTH; index++) {
        observer->history[index] = 0;
    }
    observer->sample_index = 0U;
    observer->sample_count = 0U;
    observer->delta_1ms_counts = 0;
    observer->fast_velocity_counts_s = 0;
    observer->slow_velocity_counts_s = 0;
    observer->fast_velocity_valid = false;
    observer->slow_velocity_valid = false;
}

void motion_observer_update_1ms(
    motion_observer_t *observer,
    int32_t raw_count)
{
    uint32_t fast_index;
    uint32_t slow_index;

    observer->delta_1ms_counts = encoder_delta_16(
        raw_count, observer->previous_raw_count);
    observer->previous_raw_count = raw_count;
    observer->position_counts += observer->delta_1ms_counts;
    observer->sample_index =
        (observer->sample_index + 1U) % MOTION_OBSERVER_HISTORY_LENGTH;
    observer->history[observer->sample_index] = observer->position_counts;
    if (observer->sample_count < UINT32_MAX) {
        observer->sample_count++;
    }

    if (observer->sample_count >= MOTION_OBSERVER_FAST_WINDOW_MS) {
        fast_index =
            (observer->sample_index + MOTION_OBSERVER_HISTORY_LENGTH -
             MOTION_OBSERVER_FAST_WINDOW_MS) %
            MOTION_OBSERVER_HISTORY_LENGTH;
        observer->fast_velocity_counts_s = (int32_t)(
            (observer->position_counts - observer->history[fast_index]) *
            (1000 / (int32_t)MOTION_OBSERVER_FAST_WINDOW_MS));
        observer->fast_velocity_valid = true;
    }

    if (observer->sample_count >= MOTION_OBSERVER_SLOW_WINDOW_MS) {
        slow_index =
            (observer->sample_index + MOTION_OBSERVER_HISTORY_LENGTH -
             MOTION_OBSERVER_SLOW_WINDOW_MS) %
            MOTION_OBSERVER_HISTORY_LENGTH;
        observer->slow_velocity_counts_s = (int32_t)(
            (observer->position_counts - observer->history[slow_index]) *
            (1000 / (int32_t)MOTION_OBSERVER_SLOW_WINDOW_MS));
        observer->slow_velocity_valid = true;
    }
}
