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

## Current open decode

Observed runtime telemetry currently includes:

```text
faults=0x00000002
```

The exact symbolic meaning of this bit is intentionally left unresolved in this document until it is traced to the source definition and producer. It must be decoded before the state-safety rejection is relaxed or physical closed-loop control is enabled.

## Target CLI

```text
fault status
```

Target output should provide both the raw mask and decoded names, for example:

```text
FAULTS 0x00000002
- <exact symbolic fault name>
```

## Registry table

| Bit | Symbol | Source | Severity | Latching | Actuator Action | Recovery | Validation |
|---:|---|---|---|---|---|---|---|
| 0x00000002 | TBD - source trace required | TBD | TBD | TBD | Preserve current fail-safe behavior | TBD | Runtime observation at 94ccf2a2 |
