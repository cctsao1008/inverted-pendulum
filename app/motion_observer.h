#ifndef MOTION_OBSERVER_H
#define MOTION_OBSERVER_H

#include <stdbool.h>
#include <stdint.h>

#define MOTION_OBSERVER_HISTORY_LENGTH 64U
#define MOTION_OBSERVER_FAST_WINDOW_MS 10U
#define MOTION_OBSERVER_SLOW_WINDOW_MS 40U

typedef struct {
    int32_t previous_raw_count;
    int64_t position_counts;
    int64_t history[MOTION_OBSERVER_HISTORY_LENGTH];
    uint32_t sample_index;
    uint32_t sample_count;
    int32_t delta_1ms_counts;
    int32_t fast_velocity_counts_s;
    int32_t slow_velocity_counts_s;
    bool fast_velocity_valid;
    bool slow_velocity_valid;
} motion_observer_t;

void motion_observer_init(motion_observer_t *observer, int32_t raw_count);
void motion_observer_update_1ms(
    motion_observer_t *observer,
    int32_t raw_count);

#endif
