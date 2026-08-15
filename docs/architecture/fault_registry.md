# Fault Registry

This registry is the canonical human-readable mapping for control and safety fault bits. Do not invent names for bits before tracing their exact declaration and producer in source.

## Rules

Every fault entry must define:

- bit value;
- symbolic name;
- owner/source subsystem;
- trigger condition;
- severity;
- latching behavior;
- actuator consequence;
- recovery condition;
- runtime/telemetry representation;
- validation evidence.

## Current decoded state-safety faults

Runtime telemetry on `b6bd30d4` normally reports:

```text
faults=0x00000002
```

Source trace in `control/state_safety.h` identifies:

```c
STATE_SAFETY_FAULT_CONFIG = (1U << 1)
```

Therefore:

```text
0x00000002 = STATE_SAFETY_FAULT_CONFIG
```

`control_pipeline_init()` intentionally initializes `state_safety_limits` as unconfigured. The current observe-only STM32 startup configures the control and runtime profiles but does not yet call `control_pipeline_set_state_safety_limits()`. The persistent `0x00000002` is therefore a deliberate fail-closed configuration-not-present condition, not evidence of a sensor failure.

One runtime sample on `b6bd30d4` reported:

```text
faults=0x0000000A
```

This decodes as:

```text
0x00000002 = STATE_SAFETY_FAULT_CONFIG
0x00000008 = STATE_SAFETY_FAULT_ESTIMATE_NOT_READY
```

The transient coincided with a large input/estimated-rate excursion and returned to `0x00000002` on the next sample. Treat it as record/replay evidence for later estimator robustness work; do not relax the fail-closed behavior to hide it.

## Target CLI

```text
fault status
```

Target output should provide both the raw mask and decoded names, for example:

```text
FAULTS 0x00000002
- STATE_SAFETY_FAULT_CONFIG
```

## Registry table

| Bit | Symbol | Source | Severity | Latching | Actuator Action | Recovery | Validation |
|---:|---|---|---|---|---|---|---|
| 0x00000001 | `STATE_SAFETY_FAULT_ARGUMENT` | `state_safety_check()` | Hard | Evaluation result | Fail closed | Correct invalid/null input path | Source definition |
| 0x00000002 | `STATE_SAFETY_FAULT_CONFIG` | state-safety limits validation | Non-hard readiness block | Evaluation result | Deny control | Install a valid state-safety limit set | Runtime `b6bd30d4` |
| 0x00000004 | `STATE_SAFETY_FAULT_SENSOR_INVALID` | sensor validity | Hard | State machine may enter FAULT once active | Safe off | Restore valid sensor input and explicit recovery flow | Source definition |
| 0x00000008 | `STATE_SAFETY_FAULT_ESTIMATE_NOT_READY` | estimator readiness | Non-hard readiness block | Evaluation result | Deny control | Estimator becomes ready | Transient runtime `b6bd30d4` |
| 0x00000010 | `STATE_SAFETY_FAULT_STATE_NONFINITE` | state finite-value validation | Hard | State machine may enter FAULT once active | Safe off | Restore finite state and explicit recovery flow | Source definition |
| 0x00000020 | `STATE_SAFETY_FAULT_TIMESTAMP` | state/sensor timestamp mismatch | Hard | State machine may enter FAULT once active | Safe off | Restore timestamp consistency and explicit recovery flow | Source definition |
| 0x00000040 | `STATE_SAFETY_FAULT_SAMPLE_TIMEOUT` | sample age limit | Hard | State machine may enter FAULT once active | Safe off | Restore fresh samples and explicit recovery flow | Source definition |
| 0x00000080 | `STATE_SAFETY_FAULT_PENDULUM_LIMIT` | pendulum angle limit | Hard | State machine may enter FAULT once active | Safe off | Return to validated envelope and explicit recovery flow | Source definition |
| 0x00000100 | `STATE_SAFETY_FAULT_ARM_LIMIT` | arm angle limit | Hard | State machine may enter FAULT once active | Safe off | Return to validated envelope and explicit recovery flow | Source definition |
| 0x00000200 | `STATE_SAFETY_FAULT_PENDULUM_RATE_LIMIT` | pendulum rate limit | Hard | State machine may enter FAULT once active | Safe off | Return to validated envelope and explicit recovery flow | Source definition |
| 0x00000400 | `STATE_SAFETY_FAULT_ARM_RATE_LIMIT` | arm rate limit | Hard | State machine may enter FAULT once active | Safe off | Return to validated envelope and explicit recovery flow | Source definition |

## Current next action

Add an explicit observe-only state-safety profile with documented provisional limits while keeping `motor_output_enabled=false` and the automatic-control motor sink unbound. This must remove the configuration fault by supplying configuration, not by bypassing state-safety checks.
