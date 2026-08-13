#ifndef CONTROL_TRACE_H
#define CONTROL_TRACE_H

#include <stdbool.h>
#include <stdint.h>

#include "control_types.h"

typedef struct {
    sensor_data_t sensor;
    control_state_t state;

    control_mode_t control_mode;
    state_estimator_mode_t estimator_mode;
    balance_controller_mode_t balance_controller;

    float lqi_integral_state;

    control_command_t control;
    actuator_command_t actuator;

    uint32_t state_safety_flags;
    bool control_allowed;
} control_trace_record_t;

typedef void (*control_trace_sink_fn)(
    const control_trace_record_t *record,
    void *context);

typedef struct {
    control_trace_sink_fn sink;
    void *context;
} control_trace_t;

void control_trace_init(
    control_trace_t *trace);

void control_trace_set_sink(
    control_trace_t *trace,
    control_trace_sink_fn sink,
    void *context);

void control_trace_push(
    const control_trace_t *trace,
    const control_trace_record_t *record);

#endif
