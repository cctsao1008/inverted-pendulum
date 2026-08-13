#ifndef LOCAL_UI_H
#define LOCAL_UI_H

#include <stdbool.h>
#include <stdint.h>

#include "control_types.h"

#define LOCAL_UI_ROWS        8U
#define LOCAL_UI_COLUMNS    21U
#define LOCAL_UI_LINE_BYTES (LOCAL_UI_COLUMNS + 1U)

typedef enum {
    LOCAL_UI_PAGE_STATUS = 0,
    LOCAL_UI_PAGE_SENSOR,
    LOCAL_UI_PAGE_SAFETY,
    LOCAL_UI_PAGE_CONTROL,
    LOCAL_UI_PAGE_MAINTENANCE,
    LOCAL_UI_PAGE_COUNT
} local_ui_page_t;

typedef struct {
    char lines[LOCAL_UI_ROWS][LOCAL_UI_LINE_BYTES];
} local_ui_frame_t;

typedef struct {
    bool control_trace_valid;
    bool control_runtime_ready;
    bool control_allowed;
    bool telemetry_enabled;

    control_mode_t control_mode;
    state_estimator_mode_t estimator_mode;
    balance_controller_mode_t balance_controller;

    uint32_t fault_flags;
    uint32_t sample_age_us;
    uint32_t missed_cycles;
    uint32_t vbus_millivolts;

    int32_t theta_mrad;
    int32_t theta_rate_mrad_s;
    int32_t phi_mrad;
    int32_t phi_rate_mrad_s;
    int32_t encoder_count;
    int32_t control_milli;
    int32_t actuator_tenths_percent;
    int8_t maintenance_output_percent;
    bool motor_channel_d1;

    uint8_t oled_contrast;
} local_ui_snapshot_t;

typedef struct {
    local_ui_page_t page;
    uint8_t contrast;
} local_ui_t;

void local_ui_init(local_ui_t *ui);
void local_ui_next_page(local_ui_t *ui);
void local_ui_previous_page(local_ui_t *ui);
uint8_t local_ui_increase_contrast(local_ui_t *ui);
uint8_t local_ui_decrease_contrast(local_ui_t *ui);
uint8_t local_ui_get_contrast(const local_ui_t *ui);

void local_ui_render(
    const local_ui_t *ui,
    const local_ui_snapshot_t *snapshot,
    local_ui_frame_t *frame);

#endif
