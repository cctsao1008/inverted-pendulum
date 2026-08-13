#ifndef CONTROL_TYPES_H
#define CONTROL_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Existing control-domain state used by the estimator and balance controller.
 * Field names are intentionally preserved for source compatibility.
 */
typedef struct {
    float pendulum_angle_rad;
    float pendulum_rate_rad_s;
    float arm_angle_rad;
    float arm_rate_rad_s;
    uint32_t timestamp_us;
} control_state_t;

/*
 * Existing legacy controller output type. Kept during architecture migration.
 */
typedef struct {
    float normalized_command;
    uint8_t enable;
} control_output_t;

/*
 * Sensor acquisition boundary.
 *
 * Raw and calibrated values are carried together so the current firmware can
 * migrate incrementally without forcing hardware conversion logic into the
 * control algorithms.
 */
typedef struct {
    uint32_t timestamp_us;
    uint32_t sample_age_us;

    uint16_t pendulum_adc_raw;
    int32_t arm_encoder_count;
    uint16_t vbus_adc_raw;
    uint32_t vbus_millivolts;

    float pendulum_angle_rad;
    float arm_angle_rad;

    bool valid;
} sensor_data_t;

/*
 * Hardware-independent controller output.
 *
 * "u" is an abstract normalized control effort. It is not a PWM percentage and
 * must pass through actuator mapping before reaching the motor driver.
 */
typedef struct {
    float u;
    bool valid;
} control_command_t;

typedef enum {
    MOTOR_DIRECTION_NONE = 0,
    MOTOR_DIRECTION_LEFT,
    MOTOR_DIRECTION_RIGHT
} motor_direction_t;

typedef struct {
    motor_direction_t direction;
    float pwm_percent;
    bool brake;
    bool valid;
} actuator_command_t;

typedef enum {
    CONTROL_MODE_DISABLED = 0,
    CONTROL_MODE_IDLE,
    CONTROL_MODE_SWING_UP,
    CONTROL_MODE_CAPTURE,
    CONTROL_MODE_BALANCE,
    CONTROL_MODE_FAULT
} control_mode_t;

typedef enum {
    BALANCE_CONTROLLER_LQR = 0,
    BALANCE_CONTROLLER_LQI
} balance_controller_mode_t;

typedef enum {
    STATE_ESTIMATOR_BASIC = 0,
    STATE_ESTIMATOR_KALMAN
} state_estimator_mode_t;

control_command_t control_command_zero(void);
actuator_command_t actuator_command_safe_off(void);

#endif
