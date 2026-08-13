#ifndef KEY_SERVICE_H
#define KEY_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#define KEY_SERVICE_MAX_KEYS            8U
#define KEY_SERVICE_DEBOUNCE_TICKS     20U
#define KEY_SERVICE_LONG_PRESS_TICKS  800U
#define KEY_SERVICE_REPEAT_DELAY_TICKS 500U
#define KEY_SERVICE_REPEAT_TICKS      150U

typedef struct {
    uint32_t pressed;
    uint32_t released;
    uint32_t long_pressed;
    uint32_t repeat;
} key_service_events_t;

typedef struct {
    uint32_t stable_mask;
    uint32_t candidate_mask;
    uint32_t candidate_since_tick[KEY_SERVICE_MAX_KEYS];
    uint32_t pressed_since_tick[KEY_SERVICE_MAX_KEYS];
    uint32_t last_repeat_tick[KEY_SERVICE_MAX_KEYS];
    uint32_t long_sent_mask;
    uint8_t key_count;
} key_service_t;

void key_service_init(
    key_service_t *service,
    uint8_t key_count,
    uint32_t initial_pressed_mask,
    uint32_t current_tick);

key_service_events_t key_service_update(
    key_service_t *service,
    uint32_t raw_pressed_mask,
    uint32_t current_tick);

#endif
