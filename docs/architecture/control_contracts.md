# Control Contracts

This note fixes control-layer semantics as the platform-independent pipeline is integrated with the STM32 runtime.

## Coordinates

`theta` (`pendulum_angle_rad`) is circular and uses wrapped shortest-path deltas in the basic estimator.

`phi` (`arm_angle_rad`) is the continuous accumulated rotor-arm position relative to the defined home/reference. It is not wrapped at `+/-pi`. The STM32 sensor adapter extends the 16-bit encoder count wrap-safely before converting it to this coordinate.

For the first observe-only runtime integration, the arm position at firmware startup is the temporary `phi = 0` reference. A deliberate homing/reference procedure must replace this boot-time convention before automatic position-regulating control is enabled.

## Timing

`sample_period_s` defines the nominal cadence and admissible timestamp window. State derivatives use the measured elapsed time between accepted samples so accepted timing jitter does not bias angular-rate estimation.

One control step represents one real sampling interval. Missed control periods must not be replayed as back-to-back catch-up steps with nearly identical wall-clock timestamps. Runtime integration records/drops missed control cycles and runs the pipeline only for the newest real sample.

A forward gap longer than the accepted estimator timing window re-seeds estimator position history and clears rate history. That sample is not considered estimate-ready; one later valid real-time interval is required before rates become valid again. Duplicate, too-early, and backwards timestamps remain errors.

## Configuration ownership

Configuration validation is module-scoped. Estimator validation owns sample period and rate-filter parameters; balance validation owns output limit and feedback gains. The aggregate validator remains available for callers that require the complete configuration bundle.

`control_config_validate_safety()` currently validates capture/escape sequencing fields retained for the legacy `safety_manager` and future FSM sequencing policy. The authoritative runtime state-safety path uses `state_safety_limits_t` and does not depend on those fields.

The initial application profile is intentionally **observe-only**. It configures only the estimator timing contract and leaves controller gains plus state/output safety limits unconfigured. Therefore `control_allowed` remains false even though acquisition, estimation, and trace are live. This is intentional fail-closed bring-up behavior, not a missing runtime check.

Per-unit calibration such as pendulum upright ADC and direction remains owned by `app/runtime_parameters`. Project/runtime control profile values are owned by `app/control_profile`. Control-layer structs and validators remain the schema and contract boundary.

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

`app/main.c` now supplies real STM32 sensor samples to `control_pipeline_step()` and captures control trace records for diagnostics. Automatic control still has **no physical motor sink**: `control_pipeline_set_motor_output()` is intentionally not called.

Setting `motor_output_enabled = false` is not by itself sufficient isolation when a physical sink is bound, because fail-closed operation intentionally invokes the sink's safe-off callback. The unbound sink is therefore part of the current hardware-isolation contract.

The existing UART maintenance motor path remains independent and is still the only code path that writes the physical motor.

Slew-rate limiting, unsafe direction-reversal handling, explicit maintenance/control motor authority, configured state/output safety limits, verified control gains, and brake/coast policy remain required before automatic control is allowed to drive the motor.
