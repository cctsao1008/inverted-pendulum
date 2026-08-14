#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "runtime_parameters.h"

static void test_defaults_match_calibration_baseline(void)
{
    runtime_parameters_t parameters;

    runtime_parameters_init_defaults(&parameters);

    assert(parameters.pendulum_upright_adc == 2928U);
    assert(parameters.pendulum_direction == 1);
    assert(parameters.telemetry_rate_hz == 10U);
    assert(parameters.control_gate_max_sample_age_us == 2000U);
    assert(parameters.control_gate_max_entry_theta_mrad == 250);
    assert(!parameters.control_enable_request);
    assert(!runtime_parameters_is_dirty(&parameters));
}

static void test_valid_values_can_be_set(void)
{
    runtime_parameters_t parameters;
    int32_t value = 0;

    runtime_parameters_init_defaults(&parameters);

    assert(runtime_parameters_set_text(
        &parameters,
        "sensor.pendulum.upright_adc",
        "3008") == RUNTIME_PARAMETER_OK);

    assert(runtime_parameters_get(
        &parameters,
        "sensor.pendulum.upright_adc",
        &value) == RUNTIME_PARAMETER_OK);
    assert(value == 3008);
    assert(runtime_parameters_is_dirty(&parameters));

    runtime_parameters_init_defaults(&parameters);
    assert(runtime_parameters_set_text(
        &parameters,
        "control.gate.max_sample_age_us",
        "2500") == RUNTIME_PARAMETER_OK);
    assert(parameters.control_gate_max_sample_age_us == 2500U);

    assert(runtime_parameters_set_text(
        &parameters,
        "control.gate.max_entry_theta_mrad",
        "300") == RUNTIME_PARAMETER_OK);
    assert(parameters.control_gate_max_entry_theta_mrad == 300);

    assert(runtime_parameters_set_text(
        &parameters,
        "control.enable_request",
        "1") == RUNTIME_PARAMETER_OK);
    assert(parameters.control_enable_request);
}

static void test_invalid_values_do_not_change_parameter(void)
{
    runtime_parameters_t parameters;

    runtime_parameters_init_defaults(&parameters);

    assert(runtime_parameters_set_text(
        &parameters,
        "telem.rate_hz",
        "21") == RUNTIME_PARAMETER_OUT_OF_RANGE);
    assert(parameters.telemetry_rate_hz == 10U);

    assert(runtime_parameters_set_text(
        &parameters,
        "sensor.pendulum.direction",
        "0") == RUNTIME_PARAMETER_OUT_OF_RANGE);
    assert(parameters.pendulum_direction == 1);

    assert(runtime_parameters_set_text(
        &parameters,
        "sensor.pendulum.upright_adc",
        "3x") == RUNTIME_PARAMETER_INVALID_FORMAT);
    assert(parameters.pendulum_upright_adc == 2928U);

    assert(runtime_parameters_set_text(
        &parameters,
        "control.gate.max_sample_age_us",
        "499") == RUNTIME_PARAMETER_OUT_OF_RANGE);
    assert(parameters.control_gate_max_sample_age_us == 2000U);

    assert(runtime_parameters_set_text(
        &parameters,
        "control.gate.max_entry_theta_mrad",
        "1001") == RUNTIME_PARAMETER_OUT_OF_RANGE);
    assert(parameters.control_gate_max_entry_theta_mrad == 250);

    assert(runtime_parameters_set_text(
        &parameters,
        "control.enable_request",
        "2") == RUNTIME_PARAMETER_OUT_OF_RANGE);
    assert(!parameters.control_enable_request);
}

static void test_unknown_parameter_is_rejected(void)
{
    runtime_parameters_t parameters;

    runtime_parameters_init_defaults(&parameters);

    assert(runtime_parameters_set_text(
        &parameters,
        "unknown",
        "1") == RUNTIME_PARAMETER_NOT_FOUND);
}

static void test_setting_same_value_does_not_mark_dirty(void)
{
    runtime_parameters_t parameters;

    runtime_parameters_init_defaults(&parameters);

    assert(runtime_parameters_set_text(
        &parameters,
        "telem.rate_hz",
        "10") == RUNTIME_PARAMETER_OK);
    assert(!runtime_parameters_is_dirty(&parameters));
}

int main(void)
{
    test_defaults_match_calibration_baseline();
    test_valid_values_can_be_set();
    test_invalid_values_do_not_change_parameter();
    test_unknown_parameter_is_rejected();
    test_setting_same_value_does_not_mark_dirty();

    printf("PASS: runtime parameter tests\n");
    return 0;
}
