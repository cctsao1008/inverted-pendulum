#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "motor_test_service.h"

typedef struct {
    int8_t last_percent;
    uint32_t write_count;
} output_capture_t;

static void capture_output(int8_t signed_percent, void *context)
{
    output_capture_t *capture = context;

    capture->last_percent = signed_percent;
    capture->write_count++;
}

static void test_requires_arm_and_enforces_limits(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};

    motor_test_service_init(&service, capture_output, &capture);

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
    uint32_t tick;

    motor_test_service_init(&service, capture_output, &capture);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start(&service, -12, 50U) == MOTOR_TEST_OK);
    assert(capture.last_percent == 0);

    for (tick = 0U; tick < 49U; tick++) {
        assert(!motor_test_service_update_1ms(&service));
        assert(capture.last_percent == -12);
    }

    assert(motor_test_service_is_active(&service));
    assert(motor_test_service_update_1ms(&service));
    assert(!motor_test_service_is_active(&service));
    assert(!motor_test_service_is_armed(&service));
    assert(capture.last_percent == 0);
}

static void test_stop_is_immediate_and_disarms(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};

    motor_test_service_init(&service, capture_output, &capture);
    assert(motor_test_service_arm(&service));
    assert(motor_test_service_start(&service, 5, 2000U) == MOTOR_TEST_OK);
    assert(!motor_test_service_update_1ms(&service));
    assert(capture.last_percent == 5);

    motor_test_service_stop(&service);

    assert(capture.last_percent == 0);
    assert(!motor_test_service_is_active(&service));
    assert(!motor_test_service_is_armed(&service));
    assert(motor_test_service_remaining_ms(&service) == 0U);
}

static void test_arm_expires_without_output(void)
{
    motor_test_service_t service;
    output_capture_t capture = {0};
    uint32_t tick;

    motor_test_service_init(&service, capture_output, &capture);
    assert(motor_test_service_arm(&service));

    for (tick = 0U; tick < MOTOR_TEST_ARM_WINDOW_MS; tick++) {
        assert(!motor_test_service_update_1ms(&service));
    }

    assert(!motor_test_service_is_armed(&service));
    assert(!motor_test_service_is_active(&service));
    assert(capture.last_percent == 0);
}

int main(void)
{
    test_requires_arm_and_enforces_limits();
    test_timeout_stops_and_disarms();
    test_stop_is_immediate_and_disarms();
    test_arm_expires_without_output();

    printf("PASS: motor test service tests\n");
    return 0;
}
