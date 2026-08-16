# Project Rebaseline Checklist

Use this template whenever development restarts on existing or previously validated hardware.

## 1. Project identity

- Project:
- Repository:
- Date:
- Engineer:
- Hardware model:
- Hardware revision:
- Board serial / specimen ID:
- Legacy firmware/version:
- Current firmware commit:

## 2. Define what "validated" means

For each subsystem, record whether validation refers to the legacy system, the current specimen, the current software implementation, or all three.

| Subsystem | Legacy proven | Current specimen measured | Current SW verified | Evidence |
|---|---|---|---|---|
| Power | | | | |
| MCU/clock | | | | |
| ADC/sensors | | | | |
| Encoder | | | | |
| Motor/actuator | | | | |
| Display/UI | | | | |
| Communication | | | | |
| Safety path | | | | |

## 3. Hardware Truth Table

Use one row per physical signal/interface.

| Item | Pin/interface | Polarity | Scale/unit | Physical meaning | Provenance | Status | Evidence |
|---|---|---|---|---|---|---|---|
| | | | | | LEGACY-PROVEN / DOC / CODE / MEASURED / INFERRED / UNKNOWN | | |

## 4. Assumption Register

| ID | Assumption | Why needed | Risk if wrong | Verification method | Owner | Status |
|---|---|---|---|---|---|---|
| A-001 | | | | | | OPEN |

Rules:

- assumptions must be explicit;
- assumptions cannot silently become facts;
- high-risk assumptions block physical actuation.

## 5. Source inventory

- Schematics:
- Datasheets:
- Vendor manuals:
- Legacy source:
- Legacy binary:
- Photos/videos:
- Previous test reports:
- Known-good measurements:
- Toolchain/build instructions:

For every source, record date/revision/freshness when possible.

## 6. Reproducible baseline

- [ ] clean checkout
- [ ] toolchain version recorded
- [ ] dependency versions recorded
- [ ] host build PASS
- [ ] host tests PASS
- [ ] target build PASS
- [ ] firmware artifact hashes recorded
- [ ] boot log captured
- [ ] runtime identity prints commit/version
- [ ] memory usage recorded
- [ ] control-loop timing recorded if applicable

## 7. Physical baseline

Before changing control behavior:

- [ ] verify supply voltage/current limit
- [ ] verify board revision
- [ ] verify sensor raw values
- [ ] verify sensor direction/polarity
- [ ] verify encoder direction/count convention
- [ ] verify actuator channel/direction
- [ ] verify emergency/safe-off path
- [ ] capture photos/wiring diagram

## 8. Safety state

- Automatic actuator output bound? YES / NO
- Operator enable required? YES / NO
- Magnitude limiter active? YES / NO
- Slew-rate limiter active? YES / NO
- Watchdog active? YES / NO
- Emergency stop verified? YES / NO
- Fault latch behavior verified? YES / NO

If any answer is uncertain, do not enter active commissioning.

## 9. Exit criteria for Rebaseline Gate

Rebaseline is complete only when:

- exact hardware identity is known;
- no critical signal remains UNKNOWN;
- all high-risk assumptions have verification plans;
- baseline build/runtime is reproducible;
- known-good vs newly verified facts are distinguished;
- the next development gate has measurable acceptance criteria.

## 10. Rebaseline conclusion

- Result: PASS / FAIL / INCONCLUSIVE
- Blocking issues:
- Accepted assumptions:
- Next gate:
- GitHub issue/milestone:
- Google Drive evidence folder:
