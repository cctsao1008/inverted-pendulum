# Experiment / Validation Record Template

## Identity

- Project:
- Date/time:
- Git commit:
- Branch:
- Hardware revision/specimen:
- Firmware version:
- ELF SHA256:
- HEX SHA256:
- BIN SHA256:

## Objective

One sentence describing exactly what this experiment validates.

## Preconditions

- Power condition:
- Mechanical setup:
- Motor/actuator authority state:
- Runtime parameters:
- Safety profile:
- Required calibration:

## Provenance

List the facts used by this test and classify each as LEGACY-PROVEN / DOC / CODE / MEASURED / INFERRED / UNKNOWN.

## Procedure

1.
2.
3.

## Acceptance criteria

Define measurable PASS criteria before running the test.

- Functional:
- Safety:
- Timing:
- Memory:
- Fault behavior:

## Evidence

- `post_patch_check.log`:
- runtime/UART log:
- photos/video:
- measurement dataset:
- Drive folder/file links:
- GitHub issue:

## Results

- Host build:
- Host tests:
- Target build:
- Runtime behavior:
- Timing:
- Faults:
- Unexpected observations:

## Conclusion

Result: PASS / FAIL / INCONCLUSIVE

Reason:

## Follow-up

- Issue(s) to open/close:
- ADR required? YES / NO
- Next experiment:
- Can project advance to next gate? YES / NO
