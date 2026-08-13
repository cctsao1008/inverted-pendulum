# Control Contracts

This note fixes control-layer semantics before the runtime pipeline is connected to STM32 motor output.

## Coordinates

`theta` (`pendulum_angle_rad`) is circular and uses wrapped shortest-path deltas in the basic estimator.

`phi` (`arm_angle_rad`) is the continuous accumulated rotor-arm position relative to the defined home/reference. It is not wrapped at `+/-pi`. The STM32 sensor adapter must extend encoder counts wrap-safely before converting them to this coordinate.

## Timing

`sample_period_s` defines the nominal cadence and admissible timestamp window. State derivatives use the measured elapsed time between accepted samples so accepted timing jitter does not bias angular-rate estimation.

## Configuration ownership

Configuration validation is module-scoped. Estimator validation owns sample period and rate-filter parameters; safety validation owns capture/escape windows; balance validation owns output limit and feedback gains. The aggregate validator remains available for callers that require the complete configuration bundle.

An invalid state-safety configuration blocks control through `control_allowed == false`. Whether a configuration issue should latch `FAULT` is a state-machine policy decision rather than an implicit property of the configuration validator.

## Hardware-enable gate

This hardening work does not connect `control_pipeline_step()` to `app/main.c` or enable physical motor output. Slew-rate limiting and unsafe direction-reversal handling remain required before automatic control is allowed to drive the motor.
