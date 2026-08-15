# Telemetry Schema

The runtime log is both a bring-up interface and a source of experiment evidence. Human-readable logs remain useful, but fields that are required for analysis should have stable names and units.

## Principles

- Preserve raw logs for every experiment.
- Filters affect display only; they must not discard evidence from the saved raw session.
- Units belong in the schema, not in assumptions made by the host parser.
- New schema revisions must remain explicitly versioned when field meaning changes.

## Current human-readable channels

- `BOOT`: boot and firmware identity.
- `BUILD`: build identity/toolchain information.
- `CONTROL_GATE`: admission configuration and startup status.
- `MOTOR_AUTH`: authority/watchdog startup status.
- `CTRL`: control state, faults, angle, sample age, gate result and related runtime information.
- `PERF`: execution timing and workload metrics.

## Target structured control record

A future machine-readable record may use a compact line form such as:

```text
@CTRL,v1,t_us,mode,control_allowed,faults,theta_mrad,theta_dot_mrad_s,arm_mrad,arm_dot_mrad_s,sample_age_us,gate_allowed,gate_reject,authority,command
```

The exact field set must be aligned to existing control types before implementation.

## Host-side use

`tools/windows/serial_tool.py` should evolve to support:

- channel filtering for display;
- complete raw-session logging;
- parsing structured records;
- CSV export;
- experiment metadata capture;
- plots of angle, rate, command, sample age, and execution time;
- scenario result summaries.
