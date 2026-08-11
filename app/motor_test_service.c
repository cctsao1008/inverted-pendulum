#include "motor_test_service.h"

static void force_safe_output(motor_test_service_t *service)
{
    service->output(0, service->output_context);
    service->remaining_ms = 0U;
    service->signed_percent = 0;
    service->active = false;
    service->output_applied = false;
}

void motor_test_service_init(
    motor_test_service_t *service,
    motor_test_output_fn output,
    void *output_context)
{
    service->output = output;
    service->output_context = output_context;
    service->armed = false;
    service->arm_remaining_ms = 0U;
    force_safe_output(service);
}

bool motor_test_service_arm(motor_test_service_t *service)
{
    if (service->active) {
        return false;
    }

    service->armed = true;
    service->arm_remaining_ms = MOTOR_TEST_ARM_WINDOW_MS;
    return true;
}

void motor_test_service_stop(motor_test_service_t *service)
{
    force_safe_output(service);
    service->armed = false;
    service->arm_remaining_ms = 0U;
}

void motor_test_service_disarm(motor_test_service_t *service)
{
    motor_test_service_stop(service);
}

motor_test_result_t motor_test_service_start(
    motor_test_service_t *service,
    int32_t signed_percent,
    uint32_t duration_ms)
{
    if (!service->armed) {
        return MOTOR_TEST_NOT_ARMED;
    }

    if (service->active) {
        return MOTOR_TEST_ALREADY_ACTIVE;
    }

    if ((signed_percent == 0) ||
        (signed_percent < -MOTOR_TEST_MAX_PERCENT) ||
        (signed_percent > MOTOR_TEST_MAX_PERCENT)) {
        return MOTOR_TEST_INVALID_PERCENT;
    }

    if ((duration_ms < MOTOR_TEST_MIN_DURATION_MS) ||
        (duration_ms > MOTOR_TEST_MAX_DURATION_MS)) {
        return MOTOR_TEST_INVALID_DURATION;
    }

    service->signed_percent = (int8_t)signed_percent;
    service->remaining_ms = (uint16_t)duration_ms;
    service->active = true;
    service->output_applied = false;

    return MOTOR_TEST_OK;
}

bool motor_test_service_update_1ms(motor_test_service_t *service)
{
    if (!service->active) {
        if (service->armed && (service->arm_remaining_ms > 0U)) {
            service->arm_remaining_ms--;
            if (service->arm_remaining_ms == 0U) {
                service->armed = false;
            }
        }
        return false;
    }

    /* Apply output on the real-time tick, after any blocking CLI reply. */
    if (!service->output_applied) {
        service->output(
            service->signed_percent,
            service->output_context);
        service->output_applied = true;
    }

    if (service->remaining_ms > 0U) {
        service->remaining_ms--;
    }

    if (service->remaining_ms == 0U) {
        motor_test_service_stop(service);
        return true;
    }

    return false;
}

bool motor_test_service_is_armed(const motor_test_service_t *service)
{
    return service->armed;
}

bool motor_test_service_is_active(const motor_test_service_t *service)
{
    return service->active;
}

int8_t motor_test_service_percent(const motor_test_service_t *service)
{
    return service->signed_percent;
}

uint16_t motor_test_service_remaining_ms(const motor_test_service_t *service)
{
    return service->remaining_ms;
}

uint16_t motor_test_service_arm_remaining_ms(
    const motor_test_service_t *service)
{
    return service->arm_remaining_ms;
}
