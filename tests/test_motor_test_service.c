#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "motor_test_service.h"

typedef struct {
    int8_t last_percent;
    uint32_t write_count;
} output_capture_t;

typedef struct {
    uint32_t now_ms;
    int32_t encoder_count;
} test_inputs_t;

static uint32_t read_time_ms(void *context)
{
    return ((test_inputs_t *)context)->now_ms;
}

static int32_t read_encoder(void *context)
{
    return ((test_inputs_t *)context)->encoder_count;
}

static uint32_t fake_now_ms(void *context)
{
    return *(uint32_t *)context;
}

static void capture_output(int8_t signed_percent, void *context)
{
    output_capture_t *capture = context;

    capture->last_percent = signed_percent;
    capture->write_count++;
}

static void init_service(
    motor_test_service_t *service,
    output_capture_t *capture,
    test_inputs_t *inputs)
{
    motor_test_service_init_with_encoder(
        service, capture_output, capture, read_time_ms, inputs,
        read_encoder, inputs);
}

static void test_requires_arm_and_enforces_limits(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t now_ms = 0U;

    motor_test_service_init(
        &service, capture_output, &capture, fake_now_ms, &now_ms);

    assert(motor_test_service_start(&service, 10, 100U) ==
           MOTOR_TEST_NOT_ARMED);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start(&service, 21, 100U) ==
           MOTOR_TEST_INVALID_PERCENT);
    assert(motor_test_service_start(&service, 10, 49U) ==
           MOTOR_TEST_INVALID_DURATION);
    assert(!motor_test_service_is_active(&service));
    assert(capture.last_percent == 0);
}

static void test_timeout_stops_and_disarms(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t now_ms = 0U;
    uint32_t tick;

    motor_test_service_init(
        &service, capture_output, &capture, fake_now_ms, &now_ms);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start(&service, -12, 50U) == MOTOR_TEST_OK);
    assert(capture.last_percent == -12);

    for (tick = 0U; tick < 49U; tick++) {
        now_ms++;
        assert(!motor_test_service_update_1ms(&service));
        assert(capture.last_percent == -12);
    }

    assert(motor_test_service_is_active(&service));
    now_ms++;
    assert(motor_test_service_update_1ms(&service));
    assert(!motor_test_service_is_active(&service));
    assert(!motor_test_service_is_armed(&service));
    assert(capture.last_percent == 0);
}

static void test_stop_is_immediate_and_disarms(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t now_ms = 0U;

    motor_test_service_init(
        &service, capture_output, &capture, fake_now_ms, &now_ms);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start(&service, 5, 2000U) == MOTOR_TEST_OK);
    assert(capture.last_percent == 5);

    motor_test_service_stop(&service);

    assert(capture.last_percent == 0);
    assert(!motor_test_service_is_active(&service));
    assert(!motor_test_service_is_armed(&service));
    assert(motor_test_service_remaining_ms(&service) == 0U);
}

static void test_accepts_ten_second_maximum_and_rejects_above_it(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t now_ms = 100U;

    motor_test_service_init(
        &service, capture_output, &capture, fake_now_ms, &now_ms);
    assert(motor_test_service_arm(&service));

    assert(motor_test_service_start(&service, 20, 10000U) == MOTOR_TEST_OK);
    assert(capture.last_percent == 20);
    assert(motor_test_service_remaining_ms(&service) == 10000U);

    motor_test_service_stop(&service);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start(&service, 20, 10001U) ==
           MOTOR_TEST_INVALID_DURATION);
}

static void test_right_and_left_patterns_run_for_five_seconds(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t now_ms = 100U;

    motor_test_service_init(
        &service, capture_output, &capture, fake_now_ms, &now_ms);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_pattern(
               &service, MOTOR_PATTERN_RIGHT) == MOTOR_TEST_OK);
    assert(capture.last_percent == MOTOR_PATTERN_PERCENT);
    assert(motor_test_service_pattern(&service) == MOTOR_PATTERN_RIGHT);

    now_ms += MOTOR_PATTERN_DIRECTION_MS - 1U;
    assert(!motor_test_service_update_1ms(&service));
    assert(capture.last_percent == MOTOR_PATTERN_PERCENT);
    now_ms++;
    assert(motor_test_service_update_1ms(&service));
    assert(capture.last_percent == 0);
    assert(!motor_test_service_is_armed(&service));

    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_pattern(
               &service, MOTOR_PATTERN_LEFT) == MOTOR_TEST_OK);
    assert(capture.last_percent == -MOTOR_PATTERN_PERCENT);
}

static void test_both_pattern_inserts_pause_between_directions(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t now_ms = 0U;

    motor_test_service_init(
        &service, capture_output, &capture, fake_now_ms, &now_ms);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_pattern(
               &service, MOTOR_PATTERN_BOTH) == MOTOR_TEST_OK);
    assert(capture.last_percent == MOTOR_PATTERN_PERCENT);

    now_ms = MOTOR_PATTERN_DIRECTION_MS;
    assert(!motor_test_service_update_1ms(&service));
    assert(capture.last_percent == 0);
    assert(motor_test_service_is_active(&service));

    now_ms += MOTOR_PATTERN_PAUSE_MS;
    assert(!motor_test_service_update_1ms(&service));
    assert(capture.last_percent == -MOTOR_PATTERN_PERCENT);

    now_ms += MOTOR_PATTERN_DIRECTION_MS;
    assert(motor_test_service_update_1ms(&service));
    assert(capture.last_percent == 0);
    assert(!motor_test_service_is_active(&service));
    assert(!motor_test_service_is_armed(&service));
}

static void test_arm_expires_without_output(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t now_ms = 0U;
    uint32_t tick;

    motor_test_service_init(
        &service, capture_output, &capture, fake_now_ms, &now_ms);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_arm_remaining_ms(&service) == 30000U);

    for (tick = 0U; tick < MOTOR_TEST_ARM_WINDOW_MS; tick++) {
        now_ms++;
        assert(!motor_test_service_update_1ms(&service));
    }

    assert(!motor_test_service_is_armed(&service));
    assert(!motor_test_service_is_active(&service));
    assert(capture.last_percent == 0);
}

static void test_historical_update_calls_do_not_consume_new_test(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t now_ms = 1000U;
    uint32_t update;

    motor_test_service_init(
        &service, capture_output, &capture, fake_now_ms, &now_ms);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start(&service, 15, 1000U) == MOTOR_TEST_OK);

    for (update = 0U; update < 2000U; update++) {
        assert(!motor_test_service_update_1ms(&service));
    }

    assert(motor_test_service_is_active(&service));
    assert(motor_test_service_remaining_ms(&service) == 1000U);
    assert(capture.last_percent == 15);

    now_ms = 1999U;
    assert(!motor_test_service_update_1ms(&service));
    assert(motor_test_service_remaining_ms(&service) == 1U);

    now_ms = 2000U;
    assert(motor_test_service_update_1ms(&service));
    assert(capture.last_percent == 0);
}

static void advance_identify(
    motor_test_service_t *service,
    test_inputs_t *inputs,
    uint32_t duration_ms,
    int32_t encoder_step)
{
    uint32_t tick;

    for (tick = 0U; tick < duration_ms; tick++) {
        inputs->now_ms++;
        inputs->encoder_count += encoder_step;
        (void)motor_test_service_update_1ms(service);
    }
}

static void test_identify_reports_positive_encoder_sign(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_identify(&service) == MOTOR_TEST_OK);
    assert(capture.last_percent == MOTOR_IDENTIFY_FIRST_PERCENT);

    advance_identify(&service, &inputs, MOTOR_IDENTIFY_DRIVE_MS, 1);
    assert(capture.last_percent == 0);
    advance_identify(&service, &inputs, MOTOR_IDENTIFY_SETTLE_MS, 0);

    assert(!motor_test_service_is_active(&service));
    assert(!motor_test_service_is_armed(&service));
    assert(motor_test_service_identify_state(&service) ==
           MOTOR_IDENTIFY_DONE);
    assert(motor_test_service_identify_delta(&service) == 250);
    assert(motor_test_service_identify_sign(&service) == 1);
    assert(motor_test_service_identify_peak_velocity_counts_s(&service) ==
           1000);
}

static void test_identify_retries_then_reports_no_motion(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_identify(&service) == MOTOR_TEST_OK);

    advance_identify(
        &service, &inputs,
        MOTOR_IDENTIFY_DRIVE_MS + MOTOR_IDENTIFY_SETTLE_MS, 0);
    assert(motor_test_service_is_active(&service));
    assert(capture.last_percent == MOTOR_IDENTIFY_RETRY_PERCENT);

    advance_identify(
        &service, &inputs,
        MOTOR_IDENTIFY_DRIVE_MS + MOTOR_IDENTIFY_SETTLE_MS, 0);
    assert(!motor_test_service_is_active(&service));
    assert(motor_test_service_identify_state(&service) ==
           MOTOR_IDENTIFY_NO_MOTION);
    assert(capture.last_percent == 0);
}

static void test_identify_encoder_delta_is_wrap_safe(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0U, 32766};

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_identify(&service) == MOTOR_TEST_OK);

    inputs.now_ms++;
    inputs.encoder_count = -32768;
    assert(!motor_test_service_update_1ms(&service));
    assert(motor_test_service_identify_delta(&service) == 2);
}

static bool advance_characterize_window(
    motor_test_service_t *service,
    test_inputs_t *inputs,
    int32_t encoder_delta)
{
    inputs->now_ms += MOTOR_CHARACTERIZE_WINDOW_MS;
    inputs->encoder_count += encoder_delta;
    return motor_test_service_update_1ms(service);
}

static void test_characterize_finds_breakaway_and_dropout(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};
    uint32_t window;

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_characterize(&service, 1) ==
           MOTOR_TEST_OK);
    assert(capture.last_percent == 5);

    while (capture.last_percent < 15) {
        for (window = 0U; window < 4U; window++) {
            assert(!advance_characterize_window(&service, &inputs, 0));
        }
    }

    /* Encoder polarity is intentionally opposite to the PWM sign. */
    assert(!advance_characterize_window(&service, &inputs, -3));
    assert(!advance_characterize_window(&service, &inputs, -3));
    assert(motor_test_service_characterize_state(&service) ==
           MOTOR_CHARACTERIZE_CONFIRM);
    assert(motor_test_service_characterize_breakaway_percent(&service) ==
           15);
    assert(motor_test_service_characterize_encoder_sign(&service) == -1);

    for (window = 0U; window < 4U; window++) {
        assert(!advance_characterize_window(&service, &inputs, -3));
    }
    assert(capture.last_percent == 14);

    for (window = 0U; window < 6U; window++) {
        assert(!advance_characterize_window(&service, &inputs, -3));
    }
    assert(capture.last_percent == 13);
    for (window = 0U; window < 6U; window++) {
        assert(!advance_characterize_window(&service, &inputs, -3));
    }
    assert(capture.last_percent == 12);
    for (window = 0U; window < 5U; window++) {
        assert(!advance_characterize_window(&service, &inputs, 0));
    }
    assert(advance_characterize_window(&service, &inputs, 0));

    assert(motor_test_service_characterize_state(&service) ==
           MOTOR_CHARACTERIZE_DONE);
    assert(motor_test_service_characterize_minimum_sustain_percent(
               &service) == 13);
    assert(motor_test_service_characterize_dropout_percent(&service) == 12);
    assert(capture.last_percent == 0);
    assert(!motor_test_service_is_active(&service));
    assert(!motor_test_service_is_armed(&service));
}

static void test_characterize_fails_closed_at_thirty_percent(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_characterize(&service, -1) ==
           MOTOR_TEST_OK);

    while (motor_test_service_is_active(&service)) {
        (void)advance_characterize_window(&service, &inputs, 0);
    }

    assert(motor_test_service_characterize_state(&service) ==
           MOTOR_CHARACTERIZE_NO_MOTION);
    assert(capture.last_percent == 0);
    assert(!motor_test_service_is_armed(&service));
}

static void test_characterize_rejects_implausible_encoder_velocity(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_characterize(&service, 1) ==
           MOTOR_TEST_OK);
    assert(advance_characterize_window(&service, &inputs, 4000));
    assert(motor_test_service_characterize_state(&service) ==
           MOTOR_CHARACTERIZE_ENCODER_IMPLAUSIBLE);
    assert(capture.last_percent == 0);
}

static void test_response_measures_drive_and_coast(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};
    uint32_t window;

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_response(&service, 1, 50, 1000U) ==
           MOTOR_TEST_OK);
    assert(capture.last_percent == 50);

    for (window = 0U; window < 10U; window++) {
        inputs.now_ms += MOTOR_RESPONSE_WINDOW_MS;
        inputs.encoder_count -= 50;
        assert(!motor_test_service_update_1ms(&service));
    }
    assert(capture.last_percent == 0);
    assert(motor_test_service_response_state(&service) ==
           MOTOR_RESPONSE_COAST);

    inputs.now_ms += MOTOR_RESPONSE_WINDOW_MS;
    inputs.encoder_count -= 20;
    assert(!motor_test_service_update_1ms(&service));
    inputs.now_ms += MOTOR_RESPONSE_WINDOW_MS;
    inputs.encoder_count -= 5;
    assert(!motor_test_service_update_1ms(&service));
    for (window = 0U; window < MOTOR_RESPONSE_STOP_WINDOWS; window++) {
        inputs.now_ms += MOTOR_RESPONSE_WINDOW_MS;
        assert(motor_test_service_update_1ms(&service) ==
               (window + 1U == MOTOR_RESPONSE_STOP_WINDOWS));
    }

    assert(motor_test_service_response_state(&service) == MOTOR_RESPONSE_DONE);
    assert(motor_test_service_response_drive_delta(&service) == -500);
    assert(motor_test_service_response_coast_delta(&service) == -25);
    assert(motor_test_service_response_cutoff_velocity_counts_s(&service) ==
           -500);
    assert(motor_test_service_response_peak_velocity_counts_s(&service) ==
           500);
    assert(motor_test_service_response_stop_ms(&service) == 500U);
    assert(!motor_test_service_is_armed(&service));
}

static void test_response_has_separate_high_output_limits(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_response(&service, -1, 29, 5000U) ==
           MOTOR_TEST_INVALID_PERCENT);
    assert(motor_test_service_start_response(&service, -1, 81, 5000U) ==
           MOTOR_TEST_INVALID_PERCENT);
    assert(motor_test_service_start_response(&service, -1, 70, 10001U) ==
           MOTOR_TEST_INVALID_DURATION);
    assert(motor_test_service_start_response(&service, -1, 70, 5000U) ==
           MOTOR_TEST_OK);
    assert(capture.last_percent == -70);
}

static void test_brake_response_uses_guard_velocity_release_and_settle(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};
    uint32_t tick;

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_brake_response(
               &service, 1, 50, 1000U, 10) == MOTOR_TEST_OK);
    assert(capture.last_percent == 50);
    for (tick = 0U; tick < 1000U; tick++) {
        inputs.now_ms++;
        inputs.encoder_count -= 4;
        assert(!motor_test_service_update_1ms(&service));
    }
    assert(motor_test_service_brake_state(&service) == MOTOR_BRAKE_NEUTRAL);
    assert(capture.last_percent == 0);
    assert(motor_test_service_brake_drive_delta(&service) == -4000);

    inputs.now_ms++;
    inputs.encoder_count -= 4;
    assert(!motor_test_service_update_1ms(&service));
    assert(motor_test_service_brake_state(&service) == MOTOR_BRAKE_APPLY);
    assert(capture.last_percent == -10);
    assert(motor_test_service_brake_neutral_delta(&service) == -4);

    for (tick = 0U; tick < 20U; tick++) {
        inputs.now_ms++;
        inputs.encoder_count -= 4;
        assert(!motor_test_service_update_1ms(&service));
    }
    while (motor_test_service_brake_state(&service) == MOTOR_BRAKE_APPLY) {
        inputs.now_ms++;
        assert(!motor_test_service_update_1ms(&service));
    }
    assert(motor_test_service_brake_state(&service) == MOTOR_BRAKE_SETTLE);
    assert(capture.last_percent == 0);
    assert(motor_test_service_brake_delta(&service) == -80);

    for (tick = 0U; tick < MOTOR_BRAKE_SETTLE_MS; tick++) {
        inputs.now_ms++;
        assert(motor_test_service_update_1ms(&service) ==
               (tick + 1U == MOTOR_BRAKE_SETTLE_MS));
    }
    assert(motor_test_service_brake_state(&service) == MOTOR_BRAKE_DONE);
    assert(capture.last_percent == 0);
    assert(!motor_test_service_is_armed(&service));
    assert(motor_test_service_brake_settle_delta(&service) == 0);
    assert(motor_test_service_brake_time_ms(&service) >= 20U);
}

static void test_brake_response_limits_are_independent(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};
    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_brake_response(&service, 1, 50, 5000U, 9) == MOTOR_TEST_INVALID_PERCENT);
    assert(motor_test_service_start_brake_response(&service, 1, 50, 5000U, 21) == MOTOR_TEST_INVALID_PERCENT);
    assert(motor_test_service_start_brake_response(&service, 1, 50, 5000U, 10) == MOTOR_TEST_OK);
}

static void test_brake_response_timeout_preserves_partial_distance(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    test_inputs_t inputs = {0};
    uint32_t tick;

    init_service(&service, &capture, &inputs);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start_brake_response(
               &service, -1, 50, 1000U, 10) == MOTOR_TEST_OK);
    for (tick = 0U; tick < 1000U; tick++) {
        inputs.now_ms++;
        inputs.encoder_count += 4;
        assert(!motor_test_service_update_1ms(&service));
    }
    inputs.now_ms++;
    inputs.encoder_count += 4;
    assert(!motor_test_service_update_1ms(&service));
    assert(motor_test_service_brake_state(&service) == MOTOR_BRAKE_APPLY);

    for (tick = 0U; tick < MOTOR_BRAKE_MAX_MS; tick++) {
        inputs.now_ms++;
        inputs.encoder_count += 4;
        assert(motor_test_service_update_1ms(&service) ==
               (tick + 1U == MOTOR_BRAKE_MAX_MS));
    }
    assert(motor_test_service_brake_state(&service) == MOTOR_BRAKE_TIMEOUT);
    assert(motor_test_service_brake_delta(&service) ==
           (int32_t)(4U * MOTOR_BRAKE_MAX_MS));
    assert(capture.last_percent == 0);
    assert(!motor_test_service_is_armed(&service));
}

int main(void)
{
    test_requires_arm_and_enforces_limits();
    test_timeout_stops_and_disarms();
    test_stop_is_immediate_and_disarms();
    test_accepts_ten_second_maximum_and_rejects_above_it();
    test_right_and_left_patterns_run_for_five_seconds();
    test_both_pattern_inserts_pause_between_directions();
    test_arm_expires_without_output();
    test_historical_update_calls_do_not_consume_new_test();
    test_identify_reports_positive_encoder_sign();
    test_identify_retries_then_reports_no_motion();
    test_identify_encoder_delta_is_wrap_safe();
    test_characterize_finds_breakaway_and_dropout();
    test_characterize_fails_closed_at_thirty_percent();
    test_characterize_rejects_implausible_encoder_velocity();
    test_response_measures_drive_and_coast();
    test_response_has_separate_high_output_limits();
    test_brake_response_uses_guard_velocity_release_and_settle();
    test_brake_response_limits_are_independent();
    test_brake_response_timeout_preserves_partial_distance();

    printf("PASS: motor test service tests\n");
    return 0;
}
