#ifndef CONTROL_PIPELINE_H
#define CONTROL_PIPELINE_H

#include <stdbool.h>

#include "control_config.h"
#include "control_runtime.h"
#include "control_state_machine.h"
#include "control_trace.h"
#include "control_types.h"
#include "estimator_dispatch.h"
#include "lqi_controller.h"
#include "motor_output.h"
#include "output_safety.h"
#include "sensor_acquisition.h"
#include "state_safety.h"

typedef struct {
    bool initialized;
    bool control_config_present;

    control_config_t control_config;
    control_runtime_config_t runtime_config;

    state_safety_limits_t state_safety_limits;
    output_safety_limits_t output_safety_limits;

    sensor_acquisition_t sensor_acquisition;
    estimator_dispatch_t estimator_dispatch;
    control_state_machine_t state_machine;
    lqi_controller_t lqi_controller;

    motor_output_t motor_output;
    control_trace_t trace;
} control_pipeline_t;

void control_pipeline_init(
    control_pipeline_t *pipeline);

bool control_pipeline_set_control_config(
    control_pipeline_t *pipeline,
    const control_config_t *config);

bool control_pipeline_set_runtime_config(
    control_pipeline_t *pipeline,
    const control_runtime_config_t *config);

bool control_pipeline_set_state_safety_limits(
    control_pipeline_t *pipeline,
    const state_safety_limits_t *limits);

bool control_pipeline_set_output_safety_limits(
    control_pipeline_t *pipeline,
    const output_safety_limits_t *limits);

void control_pipeline_set_sensor_source(
    control_pipeline_t *pipeline,
    sensor_acquisition_read_fn read,
    void *context);

void control_pipeline_set_motor_output(
    control_pipeline_t *pipeline,
    motor_output_apply_fn apply,
    motor_output_safe_off_fn safe_off,
    void *context);

void control_pipeline_set_trace_sink(
    control_pipeline_t *pipeline,
    control_trace_sink_fn sink,
    void *context);

bool control_pipeline_request_mode(
    control_pipeline_t *pipeline,
    control_mode_t mode);

void control_pipeline_request_fault_clear(
    control_pipeline_t *pipeline);

control_mode_t control_pipeline_get_mode(
    const control_pipeline_t *pipeline);

void control_pipeline_step(
    control_pipeline_t *pipeline);

#endif
