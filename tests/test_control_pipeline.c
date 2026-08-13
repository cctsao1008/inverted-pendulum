#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "control_pipeline.h"

typedef struct {
    unsigned int apply_count;
    unsigned int safe_off_count;
    actuator_command_t last_command;
} motor_probe_t;

typedef struct {
    uint32_t timestamp_us;
    bool valid;
} sensor_probe_t;

static void test_motor_apply(
    const actuator_command_t *command,
    void *context)
{
    motor_probe_t *probe =
        (motor_probe_t *)context;

    assert(command != NULL);
    assert(probe != NULL);

    probe->apply_count++;
    probe->last_command = *command;
}

static void test_motor_safe_off(void *context)
{
    motor_probe_t *probe =
        (motor_probe_t *)context;

    assert(probe != NULL);

    probe->safe_off_count++;
}

static bool test_sensor_read(
    sensor_data_t *sample,
    void *context)
{
    sensor_probe_t *probe =
        (sensor_probe_t *)context;

    assert(sample != NULL);
    assert(probe != NULL);

    probe->timestamp_us += 1000U;

    *sample = (sensor_data_t){0};
    sample->timestamp_us = probe->timestamp_us;
    sample->sample_age_us = 0U;
    sample->pendulum_angle_rad = 0.01F;
    sample->arm_angle_rad = 0.02F;
    sample->valid = probe->valid;

    return true;
}

static control_config_t make_control_config(void)
{
    control_config_t config;

    control_config_init_invalid(&config);
    config.sample_period_s = 0.001F;
    config.capture_angle_rad = 0.10F;
    config.escape_angle_rad = 0.35F;
    config.max_normalized_output = 0.80F;
    config.rate_filter_alpha = 1.0F;
    config.balance_gain[0] = 1.0F;
    config.balance_gain[1] = 0.0F;
    config.balance_gain[2] = 0.0F;
    config.balance_gain[3] = 0.0F;
    config.balance_gains_configured = true;

    return config;
}

static state_safety_limits_t make_state_limits(void)
{
    state_safety_limits_t limits;

    state_safety_limits_init_unconfigured(&limits);
    limits.configured = true;
    limits.max_sample_age_us = 5000U;
    limits.max_abs_pendulum_angle_rad = 1.0F;
    limits.max_abs_arm_angle_rad = 10.0F;
    limits.max_abs_pendulum_rate_rad_s = 1000.0F;
    limits.max_abs_arm_rate_rad_s = 1000.0F;

    return limits;
}

static output_safety_limits_t make_output_limits(void)
{
    output_safety_limits_t limits;

    output_safety_limits_init_unconfigured(&limits);
    limits.configured = true;
    limits.max_pwm_percent = 100.0F;

    return limits;
}

static void configure_pipeline(
    control_pipeline_t *pipeline,
    sensor_probe_t *sensor,
    motor_probe_t *motor,
    bool set_control_config,
    bool motor_output_enabled)
{
    control_config_t control_config =
        make_control_config();
    state_safety_limits_t state_limits =
        make_state_limits();
    output_safety_limits_t output_limits =
        make_output_limits();
    control_runtime_config_t runtime;

    control_pipeline_init(pipeline);

    if (set_control_config) {
        assert(control_pipeline_set_control_config(
            pipeline,
            &control_config));
    }

    assert(control_pipeline_set_state_safety_limits(
        pipeline,
        &state_limits));
    assert(control_pipeline_set_output_safety_limits(
        pipeline,
        &output_limits));

    control_pipeline_set_sensor_source(
        pipeline,
        test_sensor_read,
        sensor);

    control_pipeline_set_motor_output(
        pipeline,
        test_motor_apply,
        test_motor_safe_off,
        motor);

    control_runtime_config_init_defaults(&runtime);
    runtime.motor_output_enabled = motor_output_enabled;
    assert(control_pipeline_set_runtime_config(
        pipeline,
        &runtime));
}

static void test_safe_defaults_fail_closed(void)
{
    control_pipeline_t pipeline;
    motor_probe_t probe = {0};

    control_pipeline_init(&pipeline);

    control_pipeline_set_motor_output(
        &pipeline,
        test_motor_apply,
        test_motor_safe_off,
        &probe);

    control_pipeline_step(&pipeline);

    assert(control_pipeline_get_mode(&pipeline) ==
           CONTROL_MODE_DISABLED);
    assert(probe.apply_count == 0U);
    assert(probe.safe_off_count >= 2U);
}

static void test_motor_authority_requires_all_gates(void)
{
    control_pipeline_t pipeline;
    sensor_probe_t sensor = {0U, true};
    motor_probe_t motor = {0};
    control_runtime_config_t runtime;
    unsigned int safe_off_before_disable;

    configure_pipeline(
        &pipeline,
        &sensor,
        &motor,
        true,
        true);

    assert(control_pipeline_request_mode(
        &pipeline,
        CONTROL_MODE_BALANCE));

    /* First sample only initializes estimator history. */
    control_pipeline_step(&pipeline);
    assert(motor.apply_count == 0U);
    assert(control_pipeline_get_mode(&pipeline) ==
           CONTROL_MODE_DISABLED);

    /* Second real sample makes the full path ready. */
    control_pipeline_step(&pipeline);
    assert(control_pipeline_get_mode(&pipeline) ==
           CONTROL_MODE_BALANCE);
    assert(motor.apply_count == 1U);
    assert(motor.last_command.valid);
    assert(motor.last_command.direction ==
           MOTOR_DIRECTION_LEFT);
    assert(motor.last_command.pwm_percent > 0.0F);

    safe_off_before_disable = motor.safe_off_count;

    control_runtime_config_init_defaults(&runtime);
    runtime.motor_output_enabled = false;
    assert(control_pipeline_set_runtime_config(
        &pipeline,
        &runtime));

    assert(motor.safe_off_count > safe_off_before_disable);

    control_pipeline_step(&pipeline);
    assert(motor.apply_count == 1U);
    assert(control_pipeline_get_mode(&pipeline) ==
           CONTROL_MODE_IDLE);
}

static void test_sensor_fault_propagates_to_safe_off(void)
{
    control_pipeline_t pipeline;
    sensor_probe_t sensor = {0U, true};
    motor_probe_t motor = {0};
    unsigned int safe_off_before_fault;

    configure_pipeline(
        &pipeline,
        &sensor,
        &motor,
        true,
        true);

    assert(control_pipeline_request_mode(
        &pipeline,
        CONTROL_MODE_BALANCE));

    control_pipeline_step(&pipeline);
    control_pipeline_step(&pipeline);

    assert(motor.apply_count == 1U);
    assert(control_pipeline_get_mode(&pipeline) ==
           CONTROL_MODE_BALANCE);

    safe_off_before_fault = motor.safe_off_count;
    sensor.valid = false;

    control_pipeline_step(&pipeline);

    assert(motor.apply_count == 1U);
    assert(motor.safe_off_count > safe_off_before_fault);
    assert(control_pipeline_get_mode(&pipeline) ==
           CONTROL_MODE_FAULT);
}

static void test_missing_control_config_never_grants_authority(void)
{
    control_pipeline_t pipeline;
    sensor_probe_t sensor = {0U, true};
    motor_probe_t motor = {0};

    configure_pipeline(
        &pipeline,
        &sensor,
        &motor,
        false,
        true);

    assert(control_pipeline_request_mode(
        &pipeline,
        CONTROL_MODE_BALANCE));

    control_pipeline_step(&pipeline);
    control_pipeline_step(&pipeline);

    assert(motor.apply_count == 0U);
    assert(control_pipeline_get_mode(&pipeline) ==
           CONTROL_MODE_DISABLED);
}

int main(void)
{
    test_safe_defaults_fail_closed();
    test_motor_authority_requires_all_gates();
    test_sensor_fault_propagates_to_safe_off();
    test_missing_control_config_never_grants_authority();

    return 0;
}
