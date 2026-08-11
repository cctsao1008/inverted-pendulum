#ifndef MOTOR_TEST_SERVICE_H
#define MOTOR_TEST_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "motion_observer.h"

#define MOTOR_TEST_MAX_PERCENT 20
#define MOTOR_TEST_MIN_DURATION_MS 50U
#define MOTOR_TEST_MAX_DURATION_MS 10000U
#define MOTOR_TEST_ARM_WINDOW_MS 30000U
#define MOTOR_PATTERN_PERCENT 15
#define MOTOR_PATTERN_DIRECTION_MS 5000U
#define MOTOR_PATTERN_PAUSE_MS 1000U
#define MOTOR_IDENTIFY_FIRST_PERCENT 5
#define MOTOR_IDENTIFY_RETRY_PERCENT 8
#define MOTOR_IDENTIFY_DRIVE_MS 250U
#define MOTOR_IDENTIFY_SETTLE_MS 250U
#define MOTOR_IDENTIFY_MIN_DELTA_COUNTS 3
#define MOTOR_ENCODER_COUNTS_PER_OUTPUT_REV 1040
#define MOTOR_ENCODER_RATED_COUNTS_S 9516
#define MOTOR_ENCODER_PLAUSIBLE_MAX_COUNTS_S 15000
#define MOTOR_CHARACTERIZE_START_PERCENT 5
#define MOTOR_CHARACTERIZE_RAMP_STEP_PERCENT 2
#define MOTOR_CHARACTERIZE_MAX_PERCENT 30
#define MOTOR_CHARACTERIZE_RISE_STEP_MS 1000U
#define MOTOR_CHARACTERIZE_CONFIRM_MS 1000U
#define MOTOR_CHARACTERIZE_FALL_STEP_MS 1500U
#define MOTOR_CHARACTERIZE_WINDOW_MS 250U
#define MOTOR_CHARACTERIZE_MIN_WINDOW_COUNTS 2
#define MOTOR_CHARACTERIZE_REQUIRED_WINDOWS 2U
#define MOTOR_CHARACTERIZE_TIMEOUT_MS 60000U
#define MOTOR_RESPONSE_MIN_PERCENT 30
#define MOTOR_RESPONSE_MAX_PERCENT 80
#define MOTOR_RESPONSE_MIN_DRIVE_MS 1000U
#define MOTOR_RESPONSE_MAX_DRIVE_MS 10000U
#define MOTOR_RESPONSE_WINDOW_MS 100U
#define MOTOR_RESPONSE_STOP_WINDOWS 3U
#define MOTOR_RESPONSE_STOP_MAX_DELTA_COUNTS 1
#define MOTOR_RESPONSE_MAX_COAST_MS 5000U
#define MOTOR_BRAKE_MIN_PERCENT 10
#define MOTOR_BRAKE_MAX_PERCENT 20
#define MOTOR_BRAKE_NEUTRAL_MS 1U
#define MOTOR_BRAKE_MAX_MS 300U
#define MOTOR_BRAKE_SETTLE_MS 300U
#define MOTOR_BRAKE_RELEASE_VELOCITY_COUNTS_S 600
#define MOTOR_BRAKE_SETTLED_VELOCITY_COUNTS_S 100
#define MOTOR_BRAKE_REVERSAL_MIN_COUNTS 3

typedef void (*motor_test_output_fn)(
    int8_t signed_percent,
    void *context);

typedef uint32_t (*motor_test_now_ms_fn)(void *context);
typedef int32_t (*motor_test_encoder_fn)(void *context);

typedef enum {
    MOTOR_TEST_OK = 0,
    MOTOR_TEST_NOT_ARMED,
    MOTOR_TEST_ALREADY_ACTIVE,
    MOTOR_TEST_INVALID_PERCENT,
    MOTOR_TEST_INVALID_DURATION
} motor_test_result_t;

typedef enum {
    MOTOR_PATTERN_NONE = 0,
    MOTOR_PATTERN_RIGHT,
    MOTOR_PATTERN_LEFT,
    MOTOR_PATTERN_BOTH
} motor_pattern_t;

typedef enum {
    MOTOR_IDENTIFY_IDLE = 0,
    MOTOR_IDENTIFY_DRIVE,
    MOTOR_IDENTIFY_SETTLE,
    MOTOR_IDENTIFY_DONE,
    MOTOR_IDENTIFY_NO_MOTION
} motor_identify_state_t;

typedef enum {
    MOTOR_CHARACTERIZE_IDLE = 0,
    MOTOR_CHARACTERIZE_RAMP_UP,
    MOTOR_CHARACTERIZE_CONFIRM,
    MOTOR_CHARACTERIZE_RAMP_DOWN,
    MOTOR_CHARACTERIZE_DONE,
    MOTOR_CHARACTERIZE_NO_MOTION,
    MOTOR_CHARACTERIZE_WRONG_DIRECTION,
    MOTOR_CHARACTERIZE_ENCODER_IMPLAUSIBLE,
    MOTOR_CHARACTERIZE_TIMEOUT
} motor_characterize_state_t;

typedef enum {
    MOTOR_RESPONSE_IDLE = 0,
    MOTOR_RESPONSE_DRIVE,
    MOTOR_RESPONSE_COAST,
    MOTOR_RESPONSE_DONE,
    MOTOR_RESPONSE_NO_MOTION,
    MOTOR_RESPONSE_ENCODER_IMPLAUSIBLE,
    MOTOR_RESPONSE_COAST_TIMEOUT
} motor_response_state_t;

typedef enum {
    MOTOR_BRAKE_IDLE = 0,
    MOTOR_BRAKE_DRIVE,
    MOTOR_BRAKE_NEUTRAL,
    MOTOR_BRAKE_APPLY,
    MOTOR_BRAKE_SETTLE,
    MOTOR_BRAKE_DONE,
    MOTOR_BRAKE_REVERSAL,
    MOTOR_BRAKE_NO_MOTION,
    MOTOR_BRAKE_ENCODER_IMPLAUSIBLE,
    MOTOR_BRAKE_TIMEOUT
} motor_brake_state_t;

typedef struct {
    motor_test_output_fn output;
    void *output_context;
    motor_test_now_ms_fn now_ms;
    void *time_context;
    motor_test_encoder_fn read_encoder;
    void *encoder_context;
    uint32_t deadline_ms;
    uint32_t arm_deadline_ms;
    int8_t signed_percent;
    motor_pattern_t pattern;
    uint8_t pattern_phase;
    motor_identify_state_t identify_state;
    int32_t identify_start_count;
    int32_t identify_accumulated_delta;
    int32_t identify_peak_velocity_counts_s;
    int32_t identify_last_count;
    motor_characterize_state_t characterize_state;
    int8_t characterize_direction;
    int8_t characterize_breakaway_percent;
    int8_t characterize_minimum_sustain_percent;
    int8_t characterize_dropout_percent;
    int8_t characterize_encoder_sign;
    int8_t characterize_candidate_encoder_sign;
    uint8_t characterize_motion_windows;
    uint8_t characterize_wrong_direction_windows;
    uint32_t characterize_window_deadline_ms;
    uint32_t characterize_timeout_deadline_ms;
    int32_t characterize_window_start_count;
    int32_t characterize_peak_velocity_counts_s;
    int32_t characterize_last_velocity_counts_s;
    int32_t characterize_breakaway_velocity_counts_s;
    motor_response_state_t response_state;
    int8_t response_direction;
    int8_t response_requested_percent;
    uint32_t response_requested_drive_ms;
    uint32_t response_start_ms;
    uint32_t response_cutoff_ms;
    uint32_t response_window_deadline_ms;
    int32_t response_start_count;
    int32_t response_window_start_count;
    int32_t response_previous_count;
    int32_t response_cutoff_count;
    int32_t response_drive_delta;
    int32_t response_coast_delta;
    int32_t response_cutoff_velocity_counts_s;
    int32_t response_peak_velocity_counts_s;
    uint8_t response_stopped_windows;
    motor_brake_state_t brake_state;
    int8_t brake_direction;
    int8_t brake_drive_percent;
    int8_t brake_percent;
    int8_t brake_motion_sign;
    uint32_t brake_drive_ms;
    uint32_t brake_start_ms;
    uint32_t brake_cutoff_ms;
    uint32_t brake_apply_ms;
    uint32_t brake_release_ms;
    uint32_t brake_finish_ms;
    int64_t brake_drive_start_position;
    int64_t brake_apply_start_position;
    int64_t brake_settle_start_position;
    int32_t brake_drive_delta;
    int32_t brake_neutral_delta;
    int32_t brake_delta;
    int32_t brake_settle_delta;
    int32_t brake_cutoff_velocity_counts_s;
    int32_t brake_entry_velocity_counts_s;
    int32_t brake_release_velocity_counts_s;
    int32_t brake_final_velocity_counts_s;
    int32_t brake_peak_velocity_counts_s;
    motion_observer_t brake_observer;
    uint8_t identify_attempt;
    bool armed;
    bool active;
} motor_test_service_t;

void motor_test_service_init(
    motor_test_service_t *service,
    motor_test_output_fn output,
    void *output_context,
    motor_test_now_ms_fn now_ms,
    void *time_context);

void motor_test_service_init_with_encoder(
    motor_test_service_t *service,
    motor_test_output_fn output,
    void *output_context,
    motor_test_now_ms_fn now_ms,
    void *time_context,
    motor_test_encoder_fn read_encoder,
    void *encoder_context);

bool motor_test_service_arm(motor_test_service_t *service);
void motor_test_service_stop(motor_test_service_t *service);
void motor_test_service_disarm(motor_test_service_t *service);

motor_test_result_t motor_test_service_start(
    motor_test_service_t *service,
    int32_t signed_percent,
    uint32_t duration_ms);

motor_test_result_t motor_test_service_start_pattern(
    motor_test_service_t *service,
    motor_pattern_t pattern);

motor_test_result_t motor_test_service_start_identify(
    motor_test_service_t *service);

motor_test_result_t motor_test_service_start_characterize(
    motor_test_service_t *service,
    int8_t direction);

motor_test_result_t motor_test_service_start_response(
    motor_test_service_t *service,
    int8_t direction,
    int32_t percent,
    uint32_t drive_ms);
motor_test_result_t motor_test_service_start_brake_response(
    motor_test_service_t *service, int8_t direction, int32_t drive_percent,
    uint32_t drive_ms, int32_t brake_percent);

/* Called once per 1 ms control tick. Returns true on automatic timeout. */
bool motor_test_service_update_1ms(motor_test_service_t *service);

bool motor_test_service_is_armed(const motor_test_service_t *service);
bool motor_test_service_is_active(const motor_test_service_t *service);
int8_t motor_test_service_percent(const motor_test_service_t *service);
motor_pattern_t motor_test_service_pattern(
    const motor_test_service_t *service);
motor_identify_state_t motor_test_service_identify_state(
    const motor_test_service_t *service);
int32_t motor_test_service_identify_delta(
    const motor_test_service_t *service);
int8_t motor_test_service_identify_sign(
    const motor_test_service_t *service);
int32_t motor_test_service_identify_peak_velocity_counts_s(
    const motor_test_service_t *service);
motor_characterize_state_t motor_test_service_characterize_state(
    const motor_test_service_t *service);
int8_t motor_test_service_characterize_direction(
    const motor_test_service_t *service);
int8_t motor_test_service_characterize_breakaway_percent(
    const motor_test_service_t *service);
int8_t motor_test_service_characterize_minimum_sustain_percent(
    const motor_test_service_t *service);
int8_t motor_test_service_characterize_dropout_percent(
    const motor_test_service_t *service);
int8_t motor_test_service_characterize_encoder_sign(
    const motor_test_service_t *service);
int32_t motor_test_service_characterize_peak_velocity_counts_s(
    const motor_test_service_t *service);
int32_t motor_test_service_characterize_breakaway_velocity_counts_s(
    const motor_test_service_t *service);
motor_response_state_t motor_test_service_response_state(
    const motor_test_service_t *service);
int8_t motor_test_service_response_direction(
    const motor_test_service_t *service);
int8_t motor_test_service_response_percent(
    const motor_test_service_t *service);
uint32_t motor_test_service_response_drive_ms(
    const motor_test_service_t *service);
uint32_t motor_test_service_response_stop_ms(
    const motor_test_service_t *service);
int32_t motor_test_service_response_drive_delta(
    const motor_test_service_t *service);
int32_t motor_test_service_response_coast_delta(
    const motor_test_service_t *service);
int32_t motor_test_service_response_cutoff_velocity_counts_s(
    const motor_test_service_t *service);
int32_t motor_test_service_response_peak_velocity_counts_s(
    const motor_test_service_t *service);
motor_brake_state_t motor_test_service_brake_state(const motor_test_service_t *service);
int8_t motor_test_service_brake_direction(const motor_test_service_t *service);
int8_t motor_test_service_brake_drive_percent(const motor_test_service_t *service);
int8_t motor_test_service_brake_percent(const motor_test_service_t *service);
uint32_t motor_test_service_brake_drive_ms(const motor_test_service_t *service);
uint32_t motor_test_service_brake_time_ms(const motor_test_service_t *service);
int32_t motor_test_service_brake_drive_delta(const motor_test_service_t *service);
int32_t motor_test_service_brake_neutral_delta(const motor_test_service_t *service);
int32_t motor_test_service_brake_delta(const motor_test_service_t *service);
int32_t motor_test_service_brake_settle_delta(const motor_test_service_t *service);
int32_t motor_test_service_brake_cutoff_velocity_counts_s(const motor_test_service_t *service);
int32_t motor_test_service_brake_entry_velocity_counts_s(const motor_test_service_t *service);
int32_t motor_test_service_brake_release_velocity_counts_s(const motor_test_service_t *service);
int32_t motor_test_service_brake_final_velocity_counts_s(const motor_test_service_t *service);
int32_t motor_test_service_brake_peak_velocity_counts_s(const motor_test_service_t *service);
uint16_t motor_test_service_remaining_ms(const motor_test_service_t *service);
uint16_t motor_test_service_arm_remaining_ms(
    const motor_test_service_t *service);

#endif
