# Telemetry Schema

The runtime log is both a bring-up interface and a source of experiment evidence. Human-readable logs remain useful, but fields that are required for analysis should have stable names and units.

## Principles

- Preserve raw logs for every experiment.
- Filters affect display only; they must not discard evidence from the saved raw session.
- Units belong in the schema, not in assumptions made by the host parser.
- New schema revisions must remain explicitly versioned when field meaning changes.
- Software-facing records should prefer semantic state names over ambiguous symbol transliterations.

## Coordinate and naming convention

For mathematical/control documentation:

- `phi` (φ): rotary-arm angle.
- `theta` (θ): pendulum angle.

For software, telemetry, datasets, and host tooling, prefer explicit semantic names:

- `arm_angle_*`
- `arm_rate_*`
- `pendulum_angle_*`
- `pendulum_rate_*`

This keeps the physical meaning obvious even when a paper or external reference uses a different symbol convention.

Existing human-readable runtime fields that use `theta_*` / `phi_*` are legacy-compatible observability fields. Their meaning must not be silently changed. Any migration to semantic field names must be versioned in the structured schema and reflected in parsers/tests.

## Current human-readable channels

- `BOOT`: boot and firmware identity.
- `BUILD`: build identity/toolchain information.
- `CONTROL_GATE`: admission configuration and startup status.
- `MOTOR_AUTH`: authority/watchdog startup status.
- `CTRL`: control state, faults, angle, sample age, gate result and related runtime information.
- `PERF`: execution timing and workload metrics.

## Target structured control record

A future machine-readable record should use semantic state names, for example:

```text
@CTRL,v1,t_us,mode,control_allowed,faults,pendulum_angle_mrad,pendulum_rate_mrad_s,arm_angle_mrad,arm_rate_mrad_s,sample_age_us,gate_allowed,gate_reject,authority,command
```

The exact field set must be aligned to existing control types before implementation. A schema version must change whenever field meaning or ordering changes incompatibly.

## Experiment data products

Structured capture should preserve the distinction between immutable measurements and derived analysis products. A typical experiment directory may contain:

```text
experiments/<timestamp>/
├── metadata.json
├── raw.log
├── raw.csv
├── fitted_params.json
├── motor_response.png
└── model_vs_real.png
```

The exact files depend on the experiment, but metadata should preserve at least firmware commit/build identity, hardware/test conditions, coordinate convention, timing configuration, and analysis revision.

## Host-side use

`tools/windows/serial_tool.py` should evolve to support:

- channel filtering for display;
- complete raw-session logging;
- parsing structured records;
- CSV export;
- experiment metadata capture;
- plots of angle, rate, command, sample age, and execution time;
- real-versus-model comparison where applicable;
- scenario result summaries.
