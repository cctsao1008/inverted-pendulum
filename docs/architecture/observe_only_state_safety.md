# Observe-only State Safety Profile

This profile exists to remove the artificial `STATE_SAFETY_FAULT_CONFIG` condition during observe-only bring-up without claiming that plant-specific physical safety limits have been calibrated.

## Intent

The observe-only profile is diagnostic, not an active-control safety envelope.

It keeps these checks meaningful:

- sensor validity;
- estimator readiness;
- finite state values;
- sensor/state timestamp consistency.

It deliberately does not claim calibrated plant bounds yet:

- sample-age timeout is disabled in state safety (`max_sample_age_us = 0`); admission-gate freshness remains separately instrumented;
- pendulum-angle, arm-angle, pendulum-rate, and arm-rate limits are set to `FLT_MAX` as explicit non-restrictive sentinels;
- `motor_output_enabled` remains false;
- the automatic-control motor sink remains unbound.

## Why `FLT_MAX`

Using guessed physical thresholds would turn bring-up assumptions into apparent safety policy. `FLT_MAX` keeps the configuration structurally valid while making it explicit that the physical envelope has not been calibrated. Non-finite values are still rejected by `state_safety_check()` before these range comparisons.

## Promotion rule

This profile must never be promoted directly to `bench_safe`, `commissioning`, or `normal`. Active-control profiles require measured, documented limits with experiment provenance, hardware revision, uncertainty/margin, and validation evidence.

The existing admission values `max_sample_age_us=2000` and `max_entry_theta_mrad=250` remain provisional observe-only instrumentation values and are tracked separately from this state-safety profile.
