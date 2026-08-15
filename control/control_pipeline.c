#include "control_pipeline.h"

#include "actuator_mapper.h"
#include "controller_dispatch.h"

void control_pipeline_init(
    control_pipeline_t *pipeline)
{
    if (pipeline == 0) {
        return;
    }

    pipeline->initialized = false;
    pipeline->control_config_present = false;

    control_runtime_config_init_defaults(
        &pipeline->runtime_config);

    state_safety_limits_init_unconfigured(
        &pipeline->state_safety_limits);

    output_safety_limits_init_unconfigured(
        &pipeline->output_safety_limits);

    sensor_acquisition_init(
        &pipeline->sensor_acquisition);

    estimator_dispatch_init(
        &pipeline->estimator_dispatch);

    control_state_machine_init(
        &pipeline->state_machine);

    lqi_controller_init(
        &pipeline->lqi_controller);

    motor_output_init(
        &pipeline->motor_output);

    control_trace_init(
        &pipeline->trace);

    pipeline->status = (control_pipeline_status_t){0};
    pipeline->status.control_mode = CONTROL_MODE_DISABLED;

    pipeline->initialized = true;
}

bool control_pipeline_set_control_config(
    control_pipeline_t *pipeline,
    const control_config_t *config)
{
    if ((pipeline == 0) ||
        (config == 0)) {
        return false;
    }

    pipeline->control_config = *config;
    pipeline->control_config_present = true;

    /*
     * Reinitialize estimator history when numerical control configuration
     * changes so stale derivatives cannot cross configuration boundaries.
     */
    estimator_dispatch_init(
        &pipeline->estimator_dispatch);

    pipeline->status.valid = false;

    return true;
}

bool control_pipeline_set_runtime_config(
    control_pipeline_t *pipeline,
    const control_runtime_config_t *config)
{
    if ((pipeline == 0) ||
        (config == 0) ||
        (control_runtime_config_validate(config) !=
         CONTROL_RUNTIME_CONFIG_OK)) {
        return false;
    }

    pipeline->runtime_config = *config;

    estimator_dispatch_init(
        &pipeline->estimator_dispatch);

    lqi_controller_init(
        &pipeline->lqi_controller);

    pipeline->status.valid = false;

    if (!pipeline->runtime_config.motor_output_enabled) {
        motor_output_safe_off(
            &pipeline->motor_output);
    }

    return true;
}

bool control_pipeline_set_state_safety_limits(
    control_pipeline_t *pipeline,
    const state_safety_limits_t *limits)
{
    if ((pipeline == 0) ||
        (limits == 0)) {
        return false;
    }

    pipeline->state_safety_limits = *limits;
    pipeline->status.valid = false;
    return true;
}

bool control_pipeline_set_output_safety_limits(
    control_pipeline_t *pipeline,
    const output_safety_limits_t *limits)
{
    if ((pipeline == 0) ||
        (limits == 0)) {
        return false;
    }

    pipeline->output_safety_limits = *limits;
    return true;
}

void control_pipeline_set_sensor_source(
    control_pipeline_t *pipeline,
    sensor_acquisition_read_fn read,
    void *context)
{
    if (pipeline == 0) {
        return;
    }

    sensor_acquisition_set_source(
        &pipeline->sensor_acquisition,
        read,
        context);

    pipeline->status.valid = false;
}

void control_pipeline_set_motor_output(
    control_pipeline_t *pipeline,
    motor_output_apply_fn apply,
    motor_output_safe_off_fn safe_off,
    void *context)
{
    if (pipeline == 0) {
        return;
    }

    motor_output_set_sink(
        &pipeline->motor_output,
        apply,
        safe_off,
        context);

    motor_output_safe_off(
        &pipeline->motor_output);
}

void control_pipeline_set_trace_sink(
    control_pipeline_t *pipeline,
    control_trace_sink_fn sink,
    void *context)
{
    if (pipeline == 0) {
        return;
    }

    control_trace_set_sink(
        &pipeline->trace,
        sink,
        context);
}

bool control_pipeline_request_mode(
    control_pipeline_t *pipeline,
    control_mode_t mode)
{
    if ((pipeline == 0) ||
        !pipeline->initialized) {
        return false;
    }

    return control_state_machine_request_mode(
        &pipeline->state_machine,
        mode);
}

void control_pipeline_request_fault_clear(
    control_pipeline_t *pipeline)
{
    if ((pipeline == 0) ||
        !pipeline->initialized) {
        return;
    }

    control_state_machine_request_fault_clear(
        &pipeline->state_machine);
}

control_mode_t control_pipeline_get_mode(
    const control_pipeline_t *pipeline)
{
    if ((pipeline == 0) ||
        !pipeline->initialized) {
        return CONTROL_MODE_FAULT;
    }

    return control_state_machine_get_mode(
        &pipeline->state_machine);
}

bool control_pipeline_get_status(
    const control_pipeline_t *pipeline,
    control_pipeline_status_t *status)
{
    if ((pipeline == 0) ||
        (status == 0) ||
        !pipeline->initialized ||
        !pipeline->status.valid) {
        return false;
    }

    *status = pipeline->status;
    return true;
}

void control_pipeline_step(
    control_pipeline_t *pipeline)
{
    sensor_data_t sensor;
    control_state_t state = {0};
    state_safety_result_t state_safety;
    control_command_t control;
    actuator_command_t mapped_actuator;
    actuator_command_t safe_actuator;
    control_trace_record_t trace_record;
    const control_config_t *control_config = 0;
    control_mode_t mode;
    bool estimate_ready;
    bool motor_authority;

    if ((pipeline == 0) ||
        !pipeline->initialized) {
        return;
    }

    /* 1. Sensor Acquisition */
    sensor = sensor_acquisition_read(
        &pipeline->sensor_acquisition);

    /* 2. State Estimation */
    if (pipeline->control_config_present) {
        control_config =
            &pipeline->control_config;
    }

    estimate_ready = estimator_dispatch_step(
        &pipeline->estimator_dispatch,
        pipeline->runtime_config.estimator_mode,
        control_config,
        &sensor,
        &state);

    /* 3. State Safety and Validation */
    state_safety = state_safety_check(
        &state,
        &sensor,
        estimate_ready,
        &pipeline->state_safety_limits);

    /* 4. Control State Machine */
    control_state_machine_step(
        &pipeline->state_machine,
        &pipeline->runtime_config,
        &state,
        &state_safety);

    mode = control_state_machine_get_mode(
        &pipeline->state_machine);

    lqi_controller_sync_mode(
        &pipeline->lqi_controller,
        mode);

    /*
     * Keep the latest safety/runtime status independent of optional trace
     * delivery. Admission and safety decisions must not depend on telemetry.
     */
    pipeline->status.sensor = sensor;
    pipeline->status.state = state;
    pipeline->status.control_mode = mode;
    pipeline->status.state_safety_flags = state_safety.fault_flags;
    pipeline->status.control_allowed = state_safety.control_allowed;
    pipeline->status.valid = true;

    /* 5. Selected Controller */
    control = controller_dispatch(
        &pipeline->state_machine,
        &pipeline->runtime_config,
        control_config,
        &pipeline->lqi_controller,
        &state);

    /* 6. Actuator Mapping */
    mapped_actuator = actuator_mapper_step(
        &control,
        &state);

    /* 7. Output Safety and Saturation */
    safe_actuator = output_safety_apply(
        &mapped_actuator,
        &state_safety,
        &pipeline->output_safety_limits);

    /* 8. Motor Output */
    motor_authority =
        pipeline->runtime_config.motor_output_enabled &&
        control_state_machine_mode_is_active(mode) &&
        state_safety.control_allowed;

    if (motor_authority) {
        motor_output_apply(
            &pipeline->motor_output,
            &safe_actuator);
    } else {
        motor_output_safe_off(
            &pipeline->motor_output);
    }

    /* 9. Optional Trace */
    if (pipeline->runtime_config.telemetry_enabled) {
        trace_record.sensor = sensor;
        trace_record.state = state;
        trace_record.control_mode = mode;
        trace_record.estimator_mode =
            pipeline->runtime_config.estimator_mode;
        trace_record.balance_controller =
            pipeline->runtime_config.balance_controller;
        trace_record.lqi_integral_state =
            pipeline->lqi_controller.integral_state;
        trace_record.control = control;
        trace_record.actuator = safe_actuator;
        trace_record.state_safety_flags =
            state_safety.fault_flags;
        trace_record.control_allowed =
            state_safety.control_allowed;

        control_trace_push(
            &pipeline->trace,
            &trace_record);
    }
}
