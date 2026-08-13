# Control Contracts

This note fixes control-layer semantics before the runtime pipeline is connected to STM32 motor output.

## Coordinates

`theta` (`pendulum_angle_rad`) is circular and uses wrapped shortest-path deltas in the basic estimator.

`phi` (`arm_angle_rad`) is the continuous accumulated rotor-arm position relative to the defined home/reference. It is not wrapped at `+/-pi`. The STM32 sensor adapter must extend encoder counts wrap-safely before converting them to this coordinate.

## Timing

`sample_period_s` defines the nominal cadence and admissible timestamp window. State derivatives use the measured elapsed time between accepted samples so accepted timing jitter does not bias angular-rate estimation.

One control step represents one real sampling interval. Missed control periods must not be replayed as back-to-back catch-up steps with nearly identical wall-clock timestamps. Runtime integration must either execute at the real cadence or record/drop a missed cycle and resume on a later real sampling interval.

## Configuration ownership

Configuration validation is module-scoped. Estimator validation owns sample period and rate-filter parameters; balance validation owns output limit and feedback gains. The aggregate validator remains available for callers that require the complete configuration bundle.

`control_config_validate_safety()` currently validates capture/escape sequencing fields retained for the legacy `safety_manager` and future FSM sequencing policy. The authoritative runtime state-safety path uses `state_safety_limits_t` and does not depend on those fields.

An invalid state-safety configuration blocks control through `control_allowed == false`. Whether a configuration issue should latch `FAULT` is a state-machine policy decision rather than an implicit property of the configuration validator.

## Runtime capability contract

Architecture stubs remain buildable and linkable, but an algorithm is not a supported runtime configuration until its implementation is complete.

The currently selectable runtime capabilities are:

- basic state estimator
- LQR balance controller
- swing-up disabled
- capture disabled

Kalman, LQI, swing-up, and capture remain architecture placeholders and must be rejected by `control_runtime_config_validate()` until their implementations are promoted from stubs.

## Safety authority ownership

The authoritative automatic-control safety chain is:

`state_safety -> control_state_machine -> actuator_mapper -> output_safety -> motor_output`

`state_safety` decides whether the current state is controllable. `control_state_machine` owns the active control mode. `output_safety` owns actuator-domain hard constraints. `motor_output` is the platform safe-off boundary.

The older `safety_manager` is retained only as a migration source for capture/escape sequencing policy. New runtime integration code must not use it as a second safety state machine.

## Hardware-enable gate

This hardening work does not connect `control_pipeline_step()` to `app/main.c` or enable physical motor output.

For the first STM32 runtime-integration milestone, the control pipeline motor sink must remain unbound. Setting `motor_output_enabled = false` is not by itself sufficient isolation when a physical sink is bound, because fail-closed operation intentionally invokes the sink's safe-off callback.

Slew-rate limiting, unsafe direction-reversal handling, explicit maintenance/control motor authority, and brake/coast policy remain required before automatic control is allowed to drive the motor.
