#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "control_config.h"
#include "safety_manager.h"

static control_config_t make_valid_config(void)
{
    control_config_t config;

    control_config_init_invalid(&config);

    config.sample_period_s = 0.001F;
    config.capture_angle_rad = 0.10F;
    config.escape_angle_rad = 0.35F;
    config.max_normalized_output = 0.80F;

    config.balance_gain[0] = 1.0F;
    config.balance_gain[1] = 1.0F;
    config.balance_gain[2] = 1.0F;
    config.balance_gain[3] = 1.0F;
    config.balance_gains_configured = true;

    return config;
}

static safety_limits_t make_limits(void)
{
    safety_limits_t limits;

    limits.capture_required_samples = 3U;
    limits.sample_timeout_us = 5000U;

    return limits;
}

static safety_input_t make_input(void)
{
    safety_input_t input;

    input.arm_request = false;
    input.clear_fault_request = false;
    input.sample_valid = true;
    input.pendulum_angle_rad = 0.0F;
    input.sample_age_us = 0U;

    return input;
}

static void enter_balance(
    safety_manager_t *manager,
    const control_config_t *config,
    const safety_limits_t *limits,
    safety_input_t *input)
{
    input->arm_request = true;
    input->pendulum_angle_rad = 0.05F;

    assert(!safety_manager_step(
        manager, config, limits, input));
    assert(manager->state == SAFETY_STATE_CAPTURE);

    assert(!safety_manager_step(
        manager, config, limits, input));
    assert(manager->state == SAFETY_STATE_CAPTURE);

    assert(safety_manager_step(
        manager, config, limits, input));
    assert(manager->state == SAFETY_STATE_BALANCE);
}

static void test_initial_state_is_disabled(void)
{
    safety_manager_t manager;

    safety_manager_init(&manager);

    assert(manager.state == SAFETY_STATE_DISABLED);
    assert(manager.fault_flags == SAFETY_FAULT_NONE);
    assert(manager.capture_sample_count == 0U);
}

static void test_capture_requires_consecutive_samples(void)
{
    safety_manager_t manager;
    control_config_t config = make_valid_config();
    safety_limits_t limits = make_limits();
    safety_input_t input = make_input();

    safety_manager_init(&manager);
    input.arm_request = true;
    input.pendulum_angle_rad = 0.05F;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_CAPTURE);
    assert(manager.capture_sample_count == 1U);

    input.pendulum_angle_rad = 0.20F;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_READY);
    assert(manager.capture_sample_count == 0U);
}

static void test_balance_enables_output(void)
{
    safety_manager_t manager;
    control_config_t config = make_valid_config();
    safety_limits_t limits = make_limits();
    safety_input_t input = make_input();

    safety_manager_init(&manager);
    enter_balance(
        &manager, &config, &limits, &input);
}

static void test_disarm_immediately_disables_output(void)
{
    safety_manager_t manager;
    control_config_t config = make_valid_config();
    safety_limits_t limits = make_limits();
    safety_input_t input = make_input();

    safety_manager_init(&manager);
    enter_balance(
        &manager, &config, &limits, &input);

    input.arm_request = false;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_DISABLED);
    assert(manager.capture_sample_count == 0U);
}

static void test_sample_timeout_latches_fault(void)
{
    safety_manager_t manager;
    control_config_t config = make_valid_config();
    safety_limits_t limits = make_limits();
    safety_input_t input = make_input();

    safety_manager_init(&manager);

    input.arm_request = true;
    input.sample_age_us =
        limits.sample_timeout_us + 1U;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_FAULT);
    assert((manager.fault_flags &
            SAFETY_FAULT_SAMPLE_TIMEOUT) != 0U);
}

static void test_invalid_sample_latches_fault(void)
{
    safety_manager_t manager;
    control_config_t config = make_valid_config();
    safety_limits_t limits = make_limits();
    safety_input_t input = make_input();

    safety_manager_init(&manager);

    input.arm_request = true;
    input.sample_valid = false;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_FAULT);
    assert((manager.fault_flags &
            SAFETY_FAULT_SAMPLE_INVALID) != 0U);
}

static void test_escape_angle_latches_fault(void)
{
    safety_manager_t manager;
    control_config_t config = make_valid_config();
    safety_limits_t limits = make_limits();
    safety_input_t input = make_input();

    safety_manager_init(&manager);
    enter_balance(
        &manager, &config, &limits, &input);

    input.pendulum_angle_rad =
        config.escape_angle_rad + 0.01F;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_FAULT);
    assert((manager.fault_flags &
            SAFETY_FAULT_ESCAPE_ANGLE) != 0U);
}

static void test_fault_requires_disarm_and_clear(void)
{
    safety_manager_t manager;
    control_config_t config = make_valid_config();
    safety_limits_t limits = make_limits();
    safety_input_t input = make_input();

    safety_manager_init(&manager);

    input.arm_request = true;
    input.sample_valid = false;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_FAULT);

    input.clear_fault_request = true;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_FAULT);

    input.arm_request = false;

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_DISABLED);
    assert(manager.fault_flags == SAFETY_FAULT_NONE);
}

static void test_invalid_config_latches_fault(void)
{
    safety_manager_t manager;
    control_config_t config;
    safety_limits_t limits = make_limits();
    safety_input_t input = make_input();

    safety_manager_init(&manager);
    control_config_init_invalid(&config);

    assert(!safety_manager_step(
        &manager, &config, &limits, &input));
    assert(manager.state == SAFETY_STATE_FAULT);
    assert((manager.fault_flags &
            SAFETY_FAULT_CONFIG) != 0U);
}

int main(void)
{
    test_initial_state_is_disabled();
    test_capture_requires_consecutive_samples();
    test_balance_enables_output();
    test_disarm_immediately_disables_output();
    test_sample_timeout_latches_fault();
    test_invalid_sample_latches_fault();
    test_escape_angle_latches_fault();
    test_fault_requires_disarm_and_clear();
    test_invalid_config_latches_fault();

    printf("PASS: safety manager tests\n");
    return 0;
}
