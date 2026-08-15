# Runtime Profiles

Runtime profiles prevent ad-hoc combinations of safety-critical settings during bring-up.

## Target profiles

### observe_only

- motor output disabled;
- telemetry enabled;
- admission logic may be evaluated for instrumentation;
- no closed-loop motor sink binding.

### bench_safe

- motor output allowed only after all safety contracts are validated;
- strict output magnitude limit;
- strict output slew-rate limit;
- restrained mechanical setup required.

### commissioning

- explicit operator arm/enable sequence;
- calibrated admission limits;
- bounded actuator authority;
- full telemetry and experiment logging.

### normal

- only introduced after repeatable physical validation;
- no safety relaxation relative to commissioning unless justified by evidence.

## Parameter provenance

Every safety-related profile value must record:

- value;
- unit;
- source or experiment ID;
- rationale;
- validation date;
- applicable hardware revision.

The current 2000 us sample-age and 250 mrad entry-angle values are provisional observe-only values and must not be promoted into active profiles without calibration evidence.
