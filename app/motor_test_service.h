#ifndef MOTOR_TEST_SERVICE_H
#define MOTOR_TEST_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#define MOTOR_TEST_MAX_PERCENT 20
#define MOTOR_TEST_MIN_DURATION_MS 50U
#define MOTOR_TEST_MAX_DURATION_MS 2000U
#define MOTOR_TEST_ARM_WINDOW_MS 5000U

typedef void (*motor_test_output_fn)(
    int8_t signed_percent,
    void *context);

typedef enum {
    MOTOR_TEST_OK = 0,
    MOTOR_TEST_NOT_ARMED,
    MOTOR_TEST_ALREADY_ACTIVE,
    MOTOR_TEST_INVALID_PERCENT,
    MOTOR_TEST_INVALID_DURATION
} motor_test_result_t;

typedef struct {
    motor_test_output_fn output;
    void *output_context;
    uint16_t remaining_ms;
    uint16_t arm_remaining_ms;
    int8_t signed_percent;
    bool armed;
    bool active;
    bool output_applied;
} motor_test_service_t;

void motor_test_service_init(
    motor_test_service_t *service,
    motor_test_output_fn output,
    void *output_context);

bool motor_test_service_arm(motor_test_service_t *service);
void motor_test_service_stop(motor_test_service_t *service);
void motor_test_service_disarm(motor_test_service_t *service);

motor_test_result_t motor_test_service_start(
    motor_test_service_t *service,
    int32_t signed_percent,
    uint32_t duration_ms);

/* Called once per 1 ms control tick. Returns true on automatic timeout. */
bool motor_test_service_update_1ms(motor_test_service_t *service);

bool motor_test_service_is_armed(const motor_test_service_t *service);
bool motor_test_service_is_active(const motor_test_service_t *service);
int8_t motor_test_service_percent(const motor_test_service_t *service);
uint16_t motor_test_service_remaining_ms(const motor_test_service_t *service);
uint16_t motor_test_service_arm_remaining_ms(
    const motor_test_service_t *service);

#endif
