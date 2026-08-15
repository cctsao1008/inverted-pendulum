# System State Model

Status: architecture target. This document defines system-level intent; existing lower-level state machines remain the implementation source until explicitly migrated.

## States

```text
BOOT
  -> INITIALIZING
  -> READY
  -> ARMED
  -> CONTROL_ACTIVE
  -> FAULT
```

### BOOT

- Actuator safe-off.
- Motor authority is not CONTROL.
- Operator requests are not sufficient to produce motor output.

### INITIALIZING

- Hardware, sensors, estimator, runtime configuration, and control services initialize.
- Any critical initialization failure prevents READY admission.

### READY

- Runtime configuration is valid.
- Required sensors and estimator are healthy enough for observation.
- Actuator remains safe-off.
- A future explicit arm request may begin admission checks.

### ARMED

- Operator intent is explicit.
- Admission-only conditions have passed.
- The system is prepared to enter closed loop, but ARMED alone does not grant unrestricted actuator output.

### CONTROL_ACTIVE

- CONTROL owns motor authority.
- Continuous run-permit checks remain valid.
- Control commands flow only through the motor authority arbiter.
- The stale-command watchdog remains active.

### FAULT

- Actuator is safe-off.
- Fault behavior is latching where defined by the responsible subsystem.
- Admission is cleared.
- Automatic restart is forbidden; a fresh explicit operator action is required after recovery.

## Arm versus enable

The target operator model separates intent into two concepts:

- `arm`: permit admission evaluation and prepare for closed-loop entry.
- `enable`: request transition from an admitted/armed condition into CONTROL_ACTIVE.

The current `control.enable_request` parameter is an interim engineering/debug interface and must not be treated as the final operator model.

## Admission versus run permit

Admission checks may include:

- explicit operator intent;
- runtime readiness;
- active/eligible control mode;
- state-safety approval;
- fresh samples;
- entry-angle constraint;
- motor authority owner NONE.

Continuous run-permit checks must be evaluated after CONTROL ownership is acquired. They must not blindly reuse the admission requirement `authority == NONE`, and entry angle is normally an admission condition rather than a continuous operating envelope.

On any run-permit failure:

1. disable control authority;
2. safe-off the actuator;
3. release CONTROL ownership;
4. clear admitted state;
5. clear/revoke the pending operator request;
6. require a fresh explicit operator action before restart.
