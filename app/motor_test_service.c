#include "motor_test_service.h"

#include <stddef.h>

static bool deadline_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static int32_t encoder_delta_16(int32_t current, int32_t previous)
{
    return (int32_t)(int16_t)((uint16_t)current - (uint16_t)previous);
}

static void force_safe_output(motor_test_service_t *service)
{
    service->output(0, service->output_context);
    service->deadline_ms = 0U;
    service->signed_percent = 0;
    service->pattern = MOTOR_PATTERN_NONE;
    service->pattern_phase = 0U;
    if ((service->identify_state == MOTOR_IDENTIFY_DRIVE) ||
        (service->identify_state == MOTOR_IDENTIFY_SETTLE)) {
        service->identify_state = MOTOR_IDENTIFY_IDLE;
    }
    if ((service->characterize_state == MOTOR_CHARACTERIZE_RAMP_UP) ||
        (service->characterize_state == MOTOR_CHARACTERIZE_CONFIRM) ||
        (service->characterize_state == MOTOR_CHARACTERIZE_RAMP_DOWN)) {
        service->characterize_state = MOTOR_CHARACTERIZE_IDLE;
    }
    if ((service->response_state == MOTOR_RESPONSE_DRIVE) ||
        (service->response_state == MOTOR_RESPONSE_COAST)) {
        service->response_state = MOTOR_RESPONSE_IDLE;
    }
    if ((service->brake_state == MOTOR_BRAKE_DRIVE) ||
        (service->brake_state == MOTOR_BRAKE_NEUTRAL) ||
        (service->brake_state == MOTOR_BRAKE_APPLY) ||
        (service->brake_state == MOTOR_BRAKE_SETTLE)) {
        service->brake_state = MOTOR_BRAKE_IDLE;
    }
    service->active = false;
}

void motor_test_service_init_with_encoder(
    motor_test_service_t *service,
    motor_test_output_fn output,
    void *output_context,
    motor_test_now_ms_fn now_ms,
    void *time_context,
    motor_test_encoder_fn read_encoder,
    void *encoder_context)
{
    service->output = output;
    service->output_context = output_context;
    service->now_ms = now_ms;
    service->time_context = time_context;
    service->read_encoder = read_encoder;
    service->encoder_context = encoder_context;
    service->armed = false;
    service->arm_deadline_ms = 0U;
    service->identify_state = MOTOR_IDENTIFY_IDLE;
    service->identify_start_count = 0;
    service->identify_accumulated_delta = 0;
    service->identify_peak_velocity_counts_s = 0;
    service->identify_last_count = 0;
    service->characterize_state = MOTOR_CHARACTERIZE_IDLE;
    service->characterize_direction = 0;
    service->characterize_breakaway_percent = 0;
    service->characterize_minimum_sustain_percent = 0;
    service->characterize_dropout_percent = 0;
    service->characterize_encoder_sign = 0;
    service->characterize_candidate_encoder_sign = 0;
    service->characterize_motion_windows = 0U;
    service->characterize_wrong_direction_windows = 0U;
    service->characterize_window_deadline_ms = 0U;
    service->characterize_timeout_deadline_ms = 0U;
    service->characterize_window_start_count = 0;
    service->characterize_peak_velocity_counts_s = 0;
    service->characterize_last_velocity_counts_s = 0;
    service->characterize_breakaway_velocity_counts_s = 0;
    service->response_state = MOTOR_RESPONSE_IDLE;
    service->response_direction = 0;
    service->response_requested_percent = 0;
    service->response_requested_drive_ms = 0U;
    service->response_start_ms = 0U;
    service->response_cutoff_ms = 0U;
    service->response_window_deadline_ms = 0U;
    service->response_start_count = 0;
    service->response_window_start_count = 0;
    service->response_previous_count = 0;
    service->response_cutoff_count = 0;
    service->response_drive_delta = 0;
    service->response_coast_delta = 0;
    service->response_cutoff_velocity_counts_s = 0;
    service->response_peak_velocity_counts_s = 0;
    service->response_stopped_windows = 0U;
    service->brake_state = MOTOR_BRAKE_IDLE;
    service->brake_direction = 0;
    service->brake_drive_percent = 0;
    service->brake_percent = 0;
    service->brake_motion_sign = 0;
    service->brake_drive_ms = 0U;
    service->brake_start_ms = 0U;
    service->brake_cutoff_ms = 0U;
    service->brake_apply_ms = 0U;
    service->brake_release_ms = 0U;
    service->brake_finish_ms = 0U;
    service->brake_drive_start_position = 0;
    service->brake_apply_start_position = 0;
    service->brake_settle_start_position = 0;
    service->brake_drive_delta = 0;
    service->brake_neutral_delta = 0;
    service->brake_delta = 0;
    service->brake_settle_delta = 0;
    service->brake_cutoff_velocity_counts_s = 0;
    service->brake_entry_velocity_counts_s = 0;
    service->brake_release_velocity_counts_s = 0;
    service->brake_final_velocity_counts_s = 0;
    service->brake_peak_velocity_counts_s = 0;
    motion_observer_init(&service->brake_observer, 0);
    service->identify_attempt = 0U;
    force_safe_output(service);
}

void motor_test_service_init(
    motor_test_service_t *service,
    motor_test_output_fn output,
    void *output_context,
    motor_test_now_ms_fn now_ms,
    void *time_context)
{
    motor_test_service_init_with_encoder(
        service, output, output_context, now_ms, time_context, NULL, NULL);
}

bool motor_test_service_arm(motor_test_service_t *service)
{
    if (service->active) {
        return false;
    }

    service->armed = true;
    service->arm_deadline_ms =
        service->now_ms(service->time_context) + MOTOR_TEST_ARM_WINDOW_MS;
    return true;
}

void motor_test_service_stop(motor_test_service_t *service)
{
    force_safe_output(service);
    service->armed = false;
    service->arm_deadline_ms = 0U;
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
    service->identify_state = MOTOR_IDENTIFY_IDLE;
    service->pattern = MOTOR_PATTERN_NONE;
    service->pattern_phase = 0U;
    service->deadline_ms =
        service->now_ms(service->time_context) + duration_ms;
    service->active = true;

    /*
     * Apply the requested output at the command boundary.  Deferring this to
     * a later catch-up tick lets old scheduler backlog consume a newly
     * started test before the PWM has had its requested wall-clock duration.
     */
    service->output(service->signed_percent, service->output_context);

    return MOTOR_TEST_OK;
}

motor_test_result_t motor_test_service_start_pattern(
    motor_test_service_t *service,
    motor_pattern_t pattern)
{
    int8_t initial_percent;

    if (!service->armed) {
        return MOTOR_TEST_NOT_ARMED;
    }

    if (service->active) {
        return MOTOR_TEST_ALREADY_ACTIVE;
    }

    if ((pattern != MOTOR_PATTERN_RIGHT) &&
        (pattern != MOTOR_PATTERN_LEFT) &&
        (pattern != MOTOR_PATTERN_BOTH)) {
        return MOTOR_TEST_INVALID_DURATION;
    }

    initial_percent = (pattern == MOTOR_PATTERN_LEFT)
        ? -MOTOR_PATTERN_PERCENT
        : MOTOR_PATTERN_PERCENT;
    service->signed_percent = initial_percent;
    service->identify_state = MOTOR_IDENTIFY_IDLE;
    service->pattern = pattern;
    service->pattern_phase = 0U;
    service->deadline_ms = service->now_ms(service->time_context) +
        MOTOR_PATTERN_DIRECTION_MS;
    service->active = true;
    service->output(initial_percent, service->output_context);

    return MOTOR_TEST_OK;
}

motor_test_result_t motor_test_service_start_identify(
    motor_test_service_t *service)
{
    int32_t count;

    if (!service->armed) {
        return MOTOR_TEST_NOT_ARMED;
    }
    if (service->active) {
        return MOTOR_TEST_ALREADY_ACTIVE;
    }
    if (service->read_encoder == NULL) {
        return MOTOR_TEST_INVALID_DURATION;
    }

    count = service->read_encoder(service->encoder_context);
    service->identify_start_count = count;
    service->identify_last_count = count;
    service->identify_accumulated_delta = 0;
    service->identify_peak_velocity_counts_s = 0;
    service->identify_attempt = 1U;
    service->identify_state = MOTOR_IDENTIFY_DRIVE;
    service->signed_percent = MOTOR_IDENTIFY_FIRST_PERCENT;
    service->pattern = MOTOR_PATTERN_NONE;
    service->pattern_phase = 0U;
    service->deadline_ms = service->now_ms(service->time_context) +
        MOTOR_IDENTIFY_DRIVE_MS;
    service->active = true;
    service->output(service->signed_percent, service->output_context);
    return MOTOR_TEST_OK;
}

static void characterize_reset_window(
    motor_test_service_t *service,
    uint32_t now_ms)
{
    service->characterize_window_start_count =
        service->read_encoder(service->encoder_context);
    service->characterize_window_deadline_ms =
        now_ms + MOTOR_CHARACTERIZE_WINDOW_MS;
    service->characterize_motion_windows = 0U;
    service->characterize_wrong_direction_windows = 0U;
}

static void characterize_finish(
    motor_test_service_t *service,
    motor_characterize_state_t final_state)
{
    service->output(0, service->output_context);
    service->signed_percent = 0;
    service->characterize_state = final_state;
    service->active = false;
    service->armed = false;
    service->arm_deadline_ms = 0U;
    service->deadline_ms = 0U;
}

motor_test_result_t motor_test_service_start_characterize(
    motor_test_service_t *service,
    int8_t direction)
{
    uint32_t now_ms;

    if (!service->armed) {
        return MOTOR_TEST_NOT_ARMED;
    }
    if (service->active) {
        return MOTOR_TEST_ALREADY_ACTIVE;
    }
    if ((service->read_encoder == NULL) ||
        ((direction != 1) && (direction != -1))) {
        return MOTOR_TEST_INVALID_DURATION;
    }

    now_ms = service->now_ms(service->time_context);
    service->identify_state = MOTOR_IDENTIFY_IDLE;
    service->pattern = MOTOR_PATTERN_NONE;
    service->pattern_phase = 0U;
    service->characterize_state = MOTOR_CHARACTERIZE_RAMP_UP;
    service->characterize_direction = direction;
    service->characterize_breakaway_percent = 0;
    service->characterize_minimum_sustain_percent = 0;
    service->characterize_dropout_percent = 0;
    service->characterize_encoder_sign = 0;
    service->characterize_candidate_encoder_sign = 0;
    service->characterize_peak_velocity_counts_s = 0;
    service->characterize_last_velocity_counts_s = 0;
    service->characterize_breakaway_velocity_counts_s = 0;
    service->signed_percent =
        (int8_t)(direction * MOTOR_CHARACTERIZE_START_PERCENT);
    service->deadline_ms = now_ms + MOTOR_CHARACTERIZE_RISE_STEP_MS;
    service->characterize_timeout_deadline_ms =
        now_ms + MOTOR_CHARACTERIZE_TIMEOUT_MS;
    characterize_reset_window(service, now_ms);
    service->active = true;
    service->output(service->signed_percent, service->output_context);
    return MOTOR_TEST_OK;
}

motor_test_result_t motor_test_service_start_response(
    motor_test_service_t *service,
    int8_t direction,
    int32_t percent,
    uint32_t drive_ms)
{
    uint32_t now_ms;
    int32_t count;

    if (!service->armed) {
        return MOTOR_TEST_NOT_ARMED;
    }
    if (service->active) {
        return MOTOR_TEST_ALREADY_ACTIVE;
    }
    if ((service->read_encoder == NULL) ||
        ((direction != 1) && (direction != -1))) {
        return MOTOR_TEST_INVALID_DURATION;
    }
    if ((percent < MOTOR_RESPONSE_MIN_PERCENT) ||
        (percent > MOTOR_RESPONSE_MAX_PERCENT)) {
        return MOTOR_TEST_INVALID_PERCENT;
    }
    if ((drive_ms < MOTOR_RESPONSE_MIN_DRIVE_MS) ||
        (drive_ms > MOTOR_RESPONSE_MAX_DRIVE_MS)) {
        return MOTOR_TEST_INVALID_DURATION;
    }

    now_ms = service->now_ms(service->time_context);
    count = service->read_encoder(service->encoder_context);
    service->identify_state = MOTOR_IDENTIFY_IDLE;
    service->characterize_state = MOTOR_CHARACTERIZE_IDLE;
    service->pattern = MOTOR_PATTERN_NONE;
    service->pattern_phase = 0U;
    service->response_state = MOTOR_RESPONSE_DRIVE;
    service->response_direction = direction;
    service->response_requested_percent = (int8_t)percent;
    service->response_requested_drive_ms = drive_ms;
    service->response_start_ms = now_ms;
    service->response_cutoff_ms = 0U;
    service->response_start_count = count;
    service->response_window_start_count = count;
    service->response_previous_count = count;
    service->response_cutoff_count = count;
    service->response_drive_delta = 0;
    service->response_coast_delta = 0;
    service->response_cutoff_velocity_counts_s = 0;
    service->response_peak_velocity_counts_s = 0;
    service->response_stopped_windows = 0U;
    service->response_window_deadline_ms = now_ms + MOTOR_RESPONSE_WINDOW_MS;
    service->deadline_ms = now_ms + drive_ms;
    service->signed_percent = (int8_t)(direction * percent);
    service->active = true;
    service->output(service->signed_percent, service->output_context);
    return MOTOR_TEST_OK;
}

static bool response_finish(
    motor_test_service_t *service,
    motor_response_state_t final_state)
{
    service->output(0, service->output_context);
    service->signed_percent = 0;
    service->response_state = final_state;
    service->active = false;
    service->armed = false;
    service->arm_deadline_ms = 0U;
    service->deadline_ms = 0U;
    return true;
}

static bool brake_finish(motor_test_service_t *service, motor_brake_state_t state,
                         uint32_t now_ms)
{
    service->output(0, service->output_context);
    service->signed_percent = 0;
    service->brake_state = state;
    service->brake_finish_ms = now_ms;
    service->active = false;
    service->armed = false;
    service->arm_deadline_ms = 0U;
    service->deadline_ms = 0U;
    return true;
}

motor_test_result_t motor_test_service_start_brake_response(
    motor_test_service_t *service, int8_t direction, int32_t drive_percent,
    uint32_t drive_ms, int32_t brake_percent)
{
    uint32_t now_ms;
    int32_t count;
    if (!service->armed) return MOTOR_TEST_NOT_ARMED;
    if (service->active) return MOTOR_TEST_ALREADY_ACTIVE;
    if ((service->read_encoder == NULL) ||
        ((direction != 1) && (direction != -1))) return MOTOR_TEST_INVALID_DURATION;
    if ((drive_percent < MOTOR_RESPONSE_MIN_PERCENT) ||
        (drive_percent > MOTOR_RESPONSE_MAX_PERCENT) ||
        (brake_percent < MOTOR_BRAKE_MIN_PERCENT) ||
        (brake_percent > MOTOR_BRAKE_MAX_PERCENT)) return MOTOR_TEST_INVALID_PERCENT;
    if ((drive_ms < MOTOR_RESPONSE_MIN_DRIVE_MS) ||
        (drive_ms > MOTOR_RESPONSE_MAX_DRIVE_MS)) return MOTOR_TEST_INVALID_DURATION;
    now_ms = service->now_ms(service->time_context);
    count = service->read_encoder(service->encoder_context);
    service->identify_state = MOTOR_IDENTIFY_IDLE;
    service->characterize_state = MOTOR_CHARACTERIZE_IDLE;
    service->response_state = MOTOR_RESPONSE_IDLE;
    service->brake_state = MOTOR_BRAKE_DRIVE;
    service->brake_direction = direction;
    service->brake_drive_percent = (int8_t)drive_percent;
    service->brake_percent = (int8_t)brake_percent;
    service->brake_motion_sign = 0;
    service->brake_drive_ms = drive_ms;
    service->brake_start_ms = now_ms;
    service->brake_cutoff_ms = 0U;
    service->brake_apply_ms = 0U;
    service->brake_release_ms = 0U;
    service->brake_finish_ms = 0U;
    motion_observer_init(&service->brake_observer, count);
    service->brake_drive_start_position = 0;
    service->brake_apply_start_position = 0;
    service->brake_settle_start_position = 0;
    service->brake_drive_delta = 0;
    service->brake_neutral_delta = 0;
    service->brake_delta = 0;
    service->brake_settle_delta = 0;
    service->brake_cutoff_velocity_counts_s = 0;
    service->brake_entry_velocity_counts_s = 0;
    service->brake_release_velocity_counts_s = 0;
    service->brake_final_velocity_counts_s = 0;
    service->brake_peak_velocity_counts_s = 0;
    service->deadline_ms = now_ms + drive_ms;
    service->signed_percent = (int8_t)(direction * drive_percent);
    service->active = true;
    service->output(service->signed_percent, service->output_context);
    return MOTOR_TEST_OK;
}

static bool characterize_update_window(
    motor_test_service_t *service,
    uint32_t now_ms)
{
    int32_t current;
    int32_t delta;
    int32_t directed_delta;
    int32_t velocity;

    if (!deadline_reached(
            now_ms, service->characterize_window_deadline_ms)) {
        return false;
    }

    current = service->read_encoder(service->encoder_context);
    delta = encoder_delta_16(
        current, service->characterize_window_start_count);
    directed_delta = (service->characterize_encoder_sign == 0)
        ? delta
        : delta * service->characterize_encoder_sign;
    velocity = (delta * (int32_t)(1000U / MOTOR_CHARACTERIZE_WINDOW_MS));
    if (velocity < 0) {
        velocity = -velocity;
    }
    service->characterize_last_velocity_counts_s = velocity;
    if (velocity > service->characterize_peak_velocity_counts_s) {
        service->characterize_peak_velocity_counts_s = velocity;
    }
    if (velocity > MOTOR_ENCODER_PLAUSIBLE_MAX_COUNTS_S) {
        characterize_finish(
            service, MOTOR_CHARACTERIZE_ENCODER_IMPLAUSIBLE);
        return true;
    }

    if ((service->characterize_encoder_sign == 0) &&
        ((delta >= MOTOR_CHARACTERIZE_MIN_WINDOW_COUNTS) ||
         (delta <= -MOTOR_CHARACTERIZE_MIN_WINDOW_COUNTS))) {
        int8_t window_sign = (delta > 0) ? 1 : -1;

        if ((service->characterize_motion_windows == 0U) ||
            (window_sign ==
             service->characterize_candidate_encoder_sign)) {
            if (service->characterize_motion_windows < UINT8_MAX) {
                service->characterize_motion_windows++;
            }
        } else {
            service->characterize_motion_windows = 1U;
        }
        service->characterize_candidate_encoder_sign = window_sign;
        service->characterize_wrong_direction_windows = 0U;
    } else if (directed_delta >= MOTOR_CHARACTERIZE_MIN_WINDOW_COUNTS) {
        if (service->characterize_motion_windows < UINT8_MAX) {
            service->characterize_motion_windows++;
        }
        service->characterize_wrong_direction_windows = 0U;
    } else if (directed_delta <= -MOTOR_CHARACTERIZE_MIN_WINDOW_COUNTS) {
        service->characterize_motion_windows = 0U;
        if (service->characterize_wrong_direction_windows < UINT8_MAX) {
            service->characterize_wrong_direction_windows++;
        }
    } else {
        service->characterize_motion_windows = 0U;
        service->characterize_wrong_direction_windows = 0U;
    }

    service->characterize_window_start_count = current;
    service->characterize_window_deadline_ms =
        now_ms + MOTOR_CHARACTERIZE_WINDOW_MS;
    return false;
}

bool motor_test_service_update_1ms(motor_test_service_t *service)
{
    uint32_t now_ms = service->now_ms(service->time_context);

    if (service->active &&
        ((service->identify_state == MOTOR_IDENTIFY_DRIVE) ||
         (service->identify_state == MOTOR_IDENTIFY_SETTLE))) {
        int32_t current = service->read_encoder(service->encoder_context);
        int32_t delta = encoder_delta_16(current, service->identify_last_count);
        int32_t velocity = delta * 1000;

        service->identify_accumulated_delta += delta;
        service->identify_last_count = current;
        if (velocity < 0) {
            velocity = -velocity;
        }
        if (velocity > service->identify_peak_velocity_counts_s) {
            service->identify_peak_velocity_counts_s = velocity;
        }
    }

    if (!service->active) {
        if (service->armed &&
            deadline_reached(now_ms, service->arm_deadline_ms)) {
            service->armed = false;
        }
        return false;
    }

    if ((service->response_state == MOTOR_RESPONSE_DRIVE) ||
        (service->response_state == MOTOR_RESPONSE_COAST)) {
        if (deadline_reached(now_ms, service->response_window_deadline_ms)) {
            int32_t current = service->read_encoder(service->encoder_context);
            int32_t delta = encoder_delta_16(
                current, service->response_window_start_count);
            int32_t magnitude = delta;
            int32_t velocity;

            if (magnitude < 0) {
                magnitude = -magnitude;
            }
            velocity = magnitude *
                (int32_t)(1000U / MOTOR_RESPONSE_WINDOW_MS);
            if (velocity > service->response_peak_velocity_counts_s) {
                service->response_peak_velocity_counts_s = velocity;
            }
            if (velocity > MOTOR_ENCODER_PLAUSIBLE_MAX_COUNTS_S) {
                return response_finish(
                    service, MOTOR_RESPONSE_ENCODER_IMPLAUSIBLE);
            }
            service->response_window_start_count = current;
            service->response_previous_count = current;
            service->response_window_deadline_ms =
                now_ms + MOTOR_RESPONSE_WINDOW_MS;

            if (service->response_state == MOTOR_RESPONSE_DRIVE) {
                service->response_drive_delta += delta;
                service->response_cutoff_velocity_counts_s =
                    delta * (int32_t)(1000U / MOTOR_RESPONSE_WINDOW_MS);
            } else {
                service->response_coast_delta += delta;
                if (magnitude <= MOTOR_RESPONSE_STOP_MAX_DELTA_COUNTS) {
                    if (service->response_stopped_windows < UINT8_MAX) {
                        service->response_stopped_windows++;
                    }
                } else {
                    service->response_stopped_windows = 0U;
                }
            }
        }

        if ((service->response_state == MOTOR_RESPONSE_DRIVE) &&
            deadline_reached(now_ms, service->deadline_ms)) {
            int32_t current = service->read_encoder(service->encoder_context);
            service->response_drive_delta += encoder_delta_16(
                current, service->response_previous_count);
            service->response_cutoff_count = current;
            service->response_previous_count = current;
            service->response_cutoff_ms = now_ms;
            service->response_state = MOTOR_RESPONSE_COAST;
            service->signed_percent = 0;
            service->response_stopped_windows = 0U;
            service->deadline_ms = now_ms + MOTOR_RESPONSE_MAX_COAST_MS;
            service->response_window_start_count = current;
            service->response_window_deadline_ms =
                now_ms + MOTOR_RESPONSE_WINDOW_MS;
            service->output(0, service->output_context);
            return false;
        }

        if (service->response_state == MOTOR_RESPONSE_COAST) {
            if (service->response_stopped_windows >=
                MOTOR_RESPONSE_STOP_WINDOWS) {
                if (service->response_drive_delta == 0) {
                    return response_finish(
                        service, MOTOR_RESPONSE_NO_MOTION);
                }
                return response_finish(service, MOTOR_RESPONSE_DONE);
            }
            if (deadline_reached(now_ms, service->deadline_ms)) {
                return response_finish(
                    service, MOTOR_RESPONSE_COAST_TIMEOUT);
            }
        }
        return false;
    }

    if ((service->brake_state == MOTOR_BRAKE_DRIVE) ||
        (service->brake_state == MOTOR_BRAKE_NEUTRAL) ||
        (service->brake_state == MOTOR_BRAKE_APPLY) ||
        (service->brake_state == MOTOR_BRAKE_SETTLE)) {
        int32_t magnitude;
        int32_t directed_velocity;

        motion_observer_update_1ms(
            &service->brake_observer,
            service->read_encoder(service->encoder_context));

        magnitude = service->brake_observer.fast_velocity_counts_s;
        if (magnitude < 0) {
            magnitude = -magnitude;
        }
        if (service->brake_observer.fast_velocity_valid &&
            (magnitude > service->brake_peak_velocity_counts_s)) {
            service->brake_peak_velocity_counts_s = magnitude;
        }
        if (service->brake_observer.fast_velocity_valid &&
            (magnitude > MOTOR_ENCODER_PLAUSIBLE_MAX_COUNTS_S)) {
            return brake_finish(
                service, MOTOR_BRAKE_ENCODER_IMPLAUSIBLE, now_ms);
        }

        if ((service->brake_state == MOTOR_BRAKE_DRIVE) &&
            deadline_reached(now_ms, service->deadline_ms)) {
            service->brake_drive_delta = (int32_t)(
                service->brake_observer.position_counts -
                service->brake_drive_start_position);
            service->brake_cutoff_velocity_counts_s =
                service->brake_observer.fast_velocity_counts_s;
            if ((service->brake_drive_delta == 0) ||
                !service->brake_observer.fast_velocity_valid ||
                (service->brake_cutoff_velocity_counts_s == 0)) {
                return brake_finish(service, MOTOR_BRAKE_NO_MOTION, now_ms);
            }
            service->brake_motion_sign =
                (service->brake_cutoff_velocity_counts_s > 0) ? 1 : -1;
            service->brake_cutoff_ms = now_ms;
            service->brake_state = MOTOR_BRAKE_NEUTRAL;
            service->deadline_ms = now_ms + MOTOR_BRAKE_NEUTRAL_MS;
            service->signed_percent = 0;
            service->output(0, service->output_context);
            return false;
        }

        if ((service->brake_state == MOTOR_BRAKE_NEUTRAL) &&
            deadline_reached(now_ms, service->deadline_ms)) {
            service->brake_neutral_delta = (int32_t)(
                service->brake_observer.position_counts -
                service->brake_drive_start_position -
                service->brake_drive_delta);
            service->brake_entry_velocity_counts_s =
                service->brake_observer.fast_velocity_counts_s;
            service->brake_apply_start_position =
                service->brake_observer.position_counts;
            service->brake_apply_ms = now_ms;
            service->brake_state = MOTOR_BRAKE_APPLY;
            service->deadline_ms = now_ms + MOTOR_BRAKE_MAX_MS;
            service->signed_percent = (int8_t)(
                -service->brake_direction * service->brake_percent);
            service->output(
                service->signed_percent, service->output_context);
            return false;
        }

        if (service->brake_state == MOTOR_BRAKE_APPLY) {
            directed_velocity =
                service->brake_observer.fast_velocity_counts_s *
                service->brake_motion_sign;
            service->brake_delta = (int32_t)(
                service->brake_observer.position_counts -
                service->brake_apply_start_position);

            /*
             * Release before the delayed velocity estimate reaches zero.
             * Settling is evaluated separately; one opposite encoder count
             * can no longer terminate the measurement.
             */
            if (service->brake_observer.fast_velocity_valid &&
                (directed_velocity <=
                 MOTOR_BRAKE_RELEASE_VELOCITY_COUNTS_S)) {
                service->brake_release_velocity_counts_s =
                    service->brake_observer.fast_velocity_counts_s;
                service->brake_release_ms = now_ms;
                service->brake_settle_start_position =
                    service->brake_observer.position_counts;
                service->brake_state = MOTOR_BRAKE_SETTLE;
                service->deadline_ms = now_ms + MOTOR_BRAKE_SETTLE_MS;
                service->signed_percent = 0;
                service->output(0, service->output_context);
                return false;
            }
            if (deadline_reached(now_ms, service->deadline_ms)) {
                return brake_finish(service, MOTOR_BRAKE_TIMEOUT, now_ms);
            }
            return false;
        }

        if (service->brake_state == MOTOR_BRAKE_SETTLE) {
            service->brake_settle_delta = (int32_t)(
                service->brake_observer.position_counts -
                service->brake_settle_start_position);
            if (!deadline_reached(now_ms, service->deadline_ms)) {
                return false;
            }
            service->brake_final_velocity_counts_s =
                service->brake_observer.slow_velocity_counts_s;
            directed_velocity =
                service->brake_final_velocity_counts_s *
                service->brake_motion_sign;
            if ((service->brake_settle_delta *
                 service->brake_motion_sign <=
                 -MOTOR_BRAKE_REVERSAL_MIN_COUNTS) ||
                (service->brake_observer.slow_velocity_valid &&
                 (directed_velocity <
                  -MOTOR_BRAKE_SETTLED_VELOCITY_COUNTS_S))) {
                return brake_finish(
                    service, MOTOR_BRAKE_REVERSAL, now_ms);
            }
            return brake_finish(service, MOTOR_BRAKE_DONE, now_ms);
        }
        return false;
    }

    if ((service->characterize_state == MOTOR_CHARACTERIZE_RAMP_UP) ||
        (service->characterize_state == MOTOR_CHARACTERIZE_CONFIRM) ||
        (service->characterize_state == MOTOR_CHARACTERIZE_RAMP_DOWN)) {
        int8_t magnitude;

        if (deadline_reached(
                now_ms, service->characterize_timeout_deadline_ms)) {
            characterize_finish(service, MOTOR_CHARACTERIZE_TIMEOUT);
            return true;
        }
        if (characterize_update_window(service, now_ms)) {
            return true;
        }
        if (service->characterize_wrong_direction_windows >=
            MOTOR_CHARACTERIZE_REQUIRED_WINDOWS) {
            characterize_finish(
                service, MOTOR_CHARACTERIZE_WRONG_DIRECTION);
            return true;
        }

        magnitude = service->signed_percent;
        if (magnitude < 0) {
            magnitude = (int8_t)-magnitude;
        }

        if ((service->characterize_state == MOTOR_CHARACTERIZE_RAMP_UP) &&
            (service->characterize_motion_windows >=
             MOTOR_CHARACTERIZE_REQUIRED_WINDOWS)) {
            service->characterize_breakaway_percent = magnitude;
            service->characterize_minimum_sustain_percent = magnitude;
            service->characterize_encoder_sign =
                service->characterize_candidate_encoder_sign;
            service->characterize_breakaway_velocity_counts_s =
                service->characterize_last_velocity_counts_s;
            service->characterize_state = MOTOR_CHARACTERIZE_CONFIRM;
            service->deadline_ms = now_ms + MOTOR_CHARACTERIZE_CONFIRM_MS;
            characterize_reset_window(service, now_ms);
            return false;
        }

        if (!deadline_reached(now_ms, service->deadline_ms)) {
            return false;
        }

        if (service->characterize_state == MOTOR_CHARACTERIZE_RAMP_UP) {
            int8_t next = (int8_t)(magnitude +
                MOTOR_CHARACTERIZE_RAMP_STEP_PERCENT);

            if (magnitude >= MOTOR_CHARACTERIZE_MAX_PERCENT) {
                characterize_finish(
                    service, MOTOR_CHARACTERIZE_NO_MOTION);
                return true;
            }
            if (next > MOTOR_CHARACTERIZE_MAX_PERCENT) {
                next = MOTOR_CHARACTERIZE_MAX_PERCENT;
            }
            service->signed_percent =
                (int8_t)(service->characterize_direction * next);
            service->deadline_ms = now_ms + MOTOR_CHARACTERIZE_RISE_STEP_MS;
            characterize_reset_window(service, now_ms);
            service->output(
                service->signed_percent, service->output_context);
            return false;
        }

        if (service->characterize_state == MOTOR_CHARACTERIZE_CONFIRM) {
            if (service->characterize_motion_windows <
                MOTOR_CHARACTERIZE_REQUIRED_WINDOWS) {
                characterize_finish(
                    service, MOTOR_CHARACTERIZE_NO_MOTION);
                return true;
            }
            magnitude--;
            service->characterize_state = MOTOR_CHARACTERIZE_RAMP_DOWN;
            service->signed_percent =
                (int8_t)(service->characterize_direction * magnitude);
            service->deadline_ms = now_ms + MOTOR_CHARACTERIZE_FALL_STEP_MS;
            characterize_reset_window(service, now_ms);
            service->output(
                service->signed_percent, service->output_context);
            return false;
        }

        if (service->characterize_motion_windows >=
            MOTOR_CHARACTERIZE_REQUIRED_WINDOWS) {
            service->characterize_minimum_sustain_percent = magnitude;
            if (magnitude > 1) {
                magnitude--;
                service->signed_percent =
                    (int8_t)(service->characterize_direction * magnitude);
                service->deadline_ms =
                    now_ms + MOTOR_CHARACTERIZE_FALL_STEP_MS;
                characterize_reset_window(service, now_ms);
                service->output(
                    service->signed_percent, service->output_context);
                return false;
            }
            service->characterize_dropout_percent = 0;
            characterize_finish(service, MOTOR_CHARACTERIZE_DONE);
            return true;
        }

        service->characterize_dropout_percent = magnitude;
        characterize_finish(service, MOTOR_CHARACTERIZE_DONE);
        return true;
    }

    if (!deadline_reached(now_ms, service->deadline_ms)) {
        return false;
    }

    if (service->identify_state == MOTOR_IDENTIFY_DRIVE) {
        service->output(0, service->output_context);
        service->signed_percent = 0;
        service->identify_state = MOTOR_IDENTIFY_SETTLE;
        service->deadline_ms = now_ms + MOTOR_IDENTIFY_SETTLE_MS;
        return false;
    }

    if (service->identify_state == MOTOR_IDENTIFY_SETTLE) {
        int32_t magnitude = service->identify_accumulated_delta;

        if (magnitude < 0) {
            magnitude = -magnitude;
        }
        if (magnitude >= MOTOR_IDENTIFY_MIN_DELTA_COUNTS) {
            service->identify_state = MOTOR_IDENTIFY_DONE;
            service->active = false;
            service->armed = false;
            service->arm_deadline_ms = 0U;
            service->deadline_ms = 0U;
            return true;
        }
        if (service->identify_attempt == 1U) {
            int32_t count = service->read_encoder(service->encoder_context);

            service->identify_start_count = count;
            service->identify_last_count = count;
            service->identify_accumulated_delta = 0;
            service->identify_peak_velocity_counts_s = 0;
            service->identify_attempt = 2U;
            service->identify_state = MOTOR_IDENTIFY_DRIVE;
            service->signed_percent = MOTOR_IDENTIFY_RETRY_PERCENT;
            service->deadline_ms = now_ms + MOTOR_IDENTIFY_DRIVE_MS;
            service->output(service->signed_percent, service->output_context);
            return false;
        }

        service->identify_state = MOTOR_IDENTIFY_NO_MOTION;
        service->active = false;
        service->armed = false;
        service->arm_deadline_ms = 0U;
        service->deadline_ms = 0U;
        return true;
    }

    if ((service->pattern == MOTOR_PATTERN_BOTH) &&
        (service->pattern_phase == 0U)) {
        service->output(0, service->output_context);
        service->signed_percent = 0;
        service->pattern_phase = 1U;
        service->deadline_ms = now_ms + MOTOR_PATTERN_PAUSE_MS;
        return false;
    }

    if ((service->pattern == MOTOR_PATTERN_BOTH) &&
        (service->pattern_phase == 1U)) {
        service->signed_percent = -MOTOR_PATTERN_PERCENT;
        service->pattern_phase = 2U;
        service->deadline_ms = now_ms + MOTOR_PATTERN_DIRECTION_MS;
        service->output(service->signed_percent, service->output_context);
        return false;
    }

    motor_test_service_stop(service);
    return true;
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

motor_pattern_t motor_test_service_pattern(
    const motor_test_service_t *service)
{
    return service->pattern;
}

motor_identify_state_t motor_test_service_identify_state(
    const motor_test_service_t *service)
{
    return service->identify_state;
}

int32_t motor_test_service_identify_delta(
    const motor_test_service_t *service)
{
    return service->identify_accumulated_delta;
}

int8_t motor_test_service_identify_sign(
    const motor_test_service_t *service)
{
    if (service->identify_accumulated_delta > 0) {
        return 1;
    }
    if (service->identify_accumulated_delta < 0) {
        return -1;
    }
    return 0;
}

int32_t motor_test_service_identify_peak_velocity_counts_s(
    const motor_test_service_t *service)
{
    return service->identify_peak_velocity_counts_s;
}

motor_characterize_state_t motor_test_service_characterize_state(
    const motor_test_service_t *service)
{
    return service->characterize_state;
}

int8_t motor_test_service_characterize_direction(
    const motor_test_service_t *service)
{
    return service->characterize_direction;
}

int8_t motor_test_service_characterize_breakaway_percent(
    const motor_test_service_t *service)
{
    return service->characterize_breakaway_percent;
}

int8_t motor_test_service_characterize_minimum_sustain_percent(
    const motor_test_service_t *service)
{
    return service->characterize_minimum_sustain_percent;
}

int8_t motor_test_service_characterize_dropout_percent(
    const motor_test_service_t *service)
{
    return service->characterize_dropout_percent;
}

int8_t motor_test_service_characterize_encoder_sign(
    const motor_test_service_t *service)
{
    return service->characterize_encoder_sign;
}

int32_t motor_test_service_characterize_peak_velocity_counts_s(
    const motor_test_service_t *service)
{
    return service->characterize_peak_velocity_counts_s;
}

int32_t motor_test_service_characterize_breakaway_velocity_counts_s(
    const motor_test_service_t *service)
{
    return service->characterize_breakaway_velocity_counts_s;
}

motor_response_state_t motor_test_service_response_state(
    const motor_test_service_t *service)
{
    return service->response_state;
}

int8_t motor_test_service_response_direction(const motor_test_service_t *service)
{
    return service->response_direction;
}

int8_t motor_test_service_response_percent(const motor_test_service_t *service)
{
    return service->response_requested_percent;
}

uint32_t motor_test_service_response_drive_ms(const motor_test_service_t *service)
{
    return service->response_requested_drive_ms;
}

uint32_t motor_test_service_response_stop_ms(const motor_test_service_t *service)
{
    uint32_t now_ms = service->now_ms(service->time_context);
    return (service->response_cutoff_ms == 0U)
        ? 0U
        : now_ms - service->response_cutoff_ms;
}

int32_t motor_test_service_response_drive_delta(const motor_test_service_t *service)
{
    return service->response_drive_delta;
}

int32_t motor_test_service_response_coast_delta(const motor_test_service_t *service)
{
    return service->response_coast_delta;
}

int32_t motor_test_service_response_cutoff_velocity_counts_s(
    const motor_test_service_t *service)
{
    return service->response_cutoff_velocity_counts_s;
}

int32_t motor_test_service_response_peak_velocity_counts_s(
    const motor_test_service_t *service)
{
    return service->response_peak_velocity_counts_s;
}

motor_brake_state_t motor_test_service_brake_state(const motor_test_service_t *s) { return s->brake_state; }
int8_t motor_test_service_brake_direction(const motor_test_service_t *s) { return s->brake_direction; }
int8_t motor_test_service_brake_drive_percent(const motor_test_service_t *s) { return s->brake_drive_percent; }
int8_t motor_test_service_brake_percent(const motor_test_service_t *s) { return s->brake_percent; }
uint32_t motor_test_service_brake_drive_ms(const motor_test_service_t *s) { return s->brake_drive_ms; }
uint32_t motor_test_service_brake_time_ms(const motor_test_service_t *s) { return s->brake_apply_ms ? s->brake_release_ms - s->brake_apply_ms : 0U; }
int32_t motor_test_service_brake_drive_delta(const motor_test_service_t *s) { return s->brake_drive_delta; }
int32_t motor_test_service_brake_neutral_delta(const motor_test_service_t *s) { return s->brake_neutral_delta; }
int32_t motor_test_service_brake_delta(const motor_test_service_t *s) { return s->brake_delta; }
int32_t motor_test_service_brake_settle_delta(const motor_test_service_t *s) { return s->brake_settle_delta; }
int32_t motor_test_service_brake_cutoff_velocity_counts_s(const motor_test_service_t *s) { return s->brake_cutoff_velocity_counts_s; }
int32_t motor_test_service_brake_entry_velocity_counts_s(const motor_test_service_t *s) { return s->brake_entry_velocity_counts_s; }
int32_t motor_test_service_brake_release_velocity_counts_s(const motor_test_service_t *s) { return s->brake_release_velocity_counts_s; }
int32_t motor_test_service_brake_final_velocity_counts_s(const motor_test_service_t *s) { return s->brake_final_velocity_counts_s; }
int32_t motor_test_service_brake_peak_velocity_counts_s(const motor_test_service_t *s) { return s->brake_peak_velocity_counts_s; }

uint16_t motor_test_service_remaining_ms(const motor_test_service_t *service)
{
    uint32_t now_ms;
    uint32_t remaining_ms;

    if (!service->active) {
        return 0U;
    }

    now_ms = service->now_ms(service->time_context);
    if (deadline_reached(now_ms, service->deadline_ms)) {
        return 0U;
    }

    remaining_ms = service->deadline_ms - now_ms;
    return (remaining_ms > UINT16_MAX)
        ? UINT16_MAX
        : (uint16_t)remaining_ms;
}

uint16_t motor_test_service_arm_remaining_ms(
    const motor_test_service_t *service)
{
    uint32_t now_ms;
    uint32_t remaining_ms;

    if (!service->armed || service->active) {
        return 0U;
    }

    now_ms = service->now_ms(service->time_context);
    if (deadline_reached(now_ms, service->arm_deadline_ms)) {
        return 0U;
    }

    remaining_ms = service->arm_deadline_ms - now_ms;
    return (remaining_ms > UINT16_MAX)
        ? UINT16_MAX
        : (uint16_t)remaining_ms;
}
