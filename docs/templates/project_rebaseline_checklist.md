# Project Rebaseline Checklist

Use this template whenever development restarts on existing, reused, or previously validated hardware.

## 1. Project Identity Contract

- Project / system:
- Repository:
- Date:
- Engineer:
- Physical plant / controlled system:
- Actuator(s):
- Controlled coordinates / state variables:
- Primary operating modes:
- Hardware model:
- Hardware revision:
- Board serial / specimen ID:
- Legacy firmware/version:
- Legacy known-good behavior summary:
- Domain vocabulary:
- Foreign-domain terms / analogies that must not be used as shorthand:

Reconfirm this identity block after a long interruption, project handoff, major architecture change, or any terminology mismatch.

## 2. Baseline identities

Do not use one SHA as every kind of baseline.

- Current GitHub main:
- Latest local build/test-validated commit:
- Latest runtime-validated / flashed commit:
- Historical performance/reference baseline:
- Toolchain identity:
- Hardware specimen used for runtime validation:

Explain any intentional difference between these baselines.

## 3. Define what "validated" means

For each subsystem, distinguish historical proof, current physical evidence, current implementation state, and capability maturity.

| Subsystem | Legacy proven | Current specimen measured | Current SW implemented | Maturity | Evidence |
|---|---|---|---|---|---|
| Power | | | | | |
| MCU/clock | | | | | |
| ADC/sensors | | | | | |
| Encoder | | | | | |
| Motor/actuator | | | | | |
| Display/UI | | | | | |
| Communication | | | | | |
| State estimation | | | | | |
| State safety | | | | | |
| Control state machine | | | | | |
| Actuator authority | | | | | |
| Operator interface | | | | | |
| Emergency stop | | | | | |

Capability maturity must use one of:

`TARGET / STUB / IMPLEMENTED / HOST-VALIDATED / RUNTIME-VALIDATED / PHYSICALLY-COMMISSIONED`

A source file, successful build, or legacy behavior is not by itself evidence of runtime validation or physical commissioning.

## 4. Hardware Truth Table

Use one row per physical signal/interface/property. Avoid one coarse `verified` flag when different properties have different evidence.

| Item | Pin/interface | Property | Polarity | Scale/unit | Physical meaning | Provenance | Status | Evidence |
|---|---|---|---|---|---|---|---|---|
| | | mapping / signal / zero / sign / scale / range / noise / timing / other | | | | LEGACY-PROVEN / DOC / CODE / MEASURED / INFERRED / UNKNOWN | | |

Typical properties that should be separated where applicable:

- pin/interface mapping;
- signal presence;
- zero/reference;
- direction/sign convention;
- scale/unit conversion;
- full physical range and wrap behavior;
- noise/bias/repeatability;
- timing/freshness;
- control/safety suitability.

## 5. Assumption Register

| ID | Assumption | Why needed | Risk if wrong | Verification method | Owner | Status |
|---|---|---|---|---|---|---|
| A-001 | | | | | | OPEN |

Rules:

- assumptions must be explicit;
- assumptions cannot silently become facts;
- high-risk assumptions block physical actuation;
- an assumption may be closed only by identified evidence or an explicit engineering decision.

## 6. Source inventory

- Schematics:
- Datasheets:
- Vendor manuals:
- Legacy source:
- Legacy binary:
- Photos/videos:
- Previous test reports:
- Known-good measurements:
- Toolchain/build instructions:
- Current GitHub issues/ADRs:
- Google Drive evidence folders:

For every source, record date/revision/freshness when possible. If sources conflict, record the conflict instead of silently selecting one.

## 7. Reproducible build/test baseline

- [ ] clean checkout
- [ ] exact Git HEAD / origin state recorded
- [ ] toolchain version recorded
- [ ] dependency versions recorded
- [ ] host build PASS
- [ ] host tests PASS
- [ ] target build PASS
- [ ] firmware artifact hashes recorded
- [ ] memory usage recorded
- [ ] control-loop timing recorded if applicable

For an embedded runtime baseline additionally require:

- [ ] exact flashed artifact identity recorded
- [ ] boot/runtime identity reports commit/version
- [ ] runtime log captured
- [ ] hardware specimen and power condition recorded
- [ ] conclusion tied to the matching GitHub issue / experiment record

## 8. Physical baseline

Before changing control behavior or enabling new actuation:

- [ ] supply voltage/current limit verified
- [ ] board revision/specimen verified
- [ ] sensor mapping verified against source evidence
- [ ] sensor raw values observed
- [ ] sensor zero/reference verified where required
- [ ] sensor direction/polarity verified where required
- [ ] sensor range/noise/repeatability characterized to the required maturity
- [ ] encoder direction/count convention verified
- [ ] actuator channel/direction verified
- [ ] actuator safe-off behavior verified
- [ ] photos/wiring diagram captured or indexed

## 9. Control and actuator authority state

Record these separately; controller computation is not physical actuator permission.

- Controller-selection / mode-transition owner identified? YES / NO
- Physical actuator authority owner identified? YES / NO
- Automatic actuator output bound? YES / NO
- Maintenance path separated from automatic CONTROL path? YES / NO
- Explicit operator intent required? YES / NO
- Admission checks defined? YES / NO
- Continuous run-permit checks defined separately? YES / NO
- No-auto-restart behavior defined? YES / NO
- Magnitude limiter active? YES / NO
- Slew-rate limiter active? YES / NO
- Stale-output watchdog active? YES / NO
- Emergency stop independent of normal command parsing verified? YES / NO
- Fault latch/recovery behavior verified? YES / NO

If any required answer is uncertain, do not enter active physical commissioning.

## 10. GitHub / Drive freshness check

- [ ] GitHub README/current-status text matches the current plant and development position
- [ ] source-coupled architecture matches current implementation boundaries
- [ ] current Drive plan identifies its snapshot date and current GitHub position
- [ ] Drive dashboard separates current main / build-test / runtime / historical baselines
- [ ] historical plans are marked HISTORICAL / SUPERSEDED
- [ ] duplicate architecture summaries are marked non-authoritative when appropriate
- [ ] ADR validation sections reflect later implementation evidence without rewriting the original decision context

## 11. Exit criteria for Rebaseline Gate

Rebaseline is complete only when:

- exact project/plant identity and domain vocabulary are known;
- exact hardware identity is known;
- no critical signal/property remains UNKNOWN without an explicit blocking or verification plan;
- all high-risk assumptions have verification plans;
- baseline identities are explicit and reproducible;
- legacy-known vs current measured/implemented facts are separated;
- capability maturity claims are evidence-backed;
- source-of-truth ownership is clear and GitHub/Drive current-status records agree;
- physical automatic output remains blocked until its named commissioning gate is satisfied;
- the next development gate has measurable acceptance criteria.

## 12. Rebaseline conclusion

- Result: PASS / FAIL / INCONCLUSIVE
- Current GitHub main:
- Latest build/test-validated commit:
- Latest runtime-validated commit:
- Historical reference baseline:
- Blocking issues:
- Accepted assumptions:
- Next gate:
- GitHub issue/milestone:
- Google Drive evidence folder:
- Date of next truth/freshness review:
