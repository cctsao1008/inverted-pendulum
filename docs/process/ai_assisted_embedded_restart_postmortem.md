# AI-Assisted Embedded Restart Postmortem

Date: 2026-08-16
Project: Rotary Inverted Pendulum

## Purpose

This document records the mistakes and process lessons from restarting development of a previously working embedded-control platform with ChatGPT. The goal is not to assign blame; it is to make the next restart evidence-driven, traceable, and less wasteful.

## Executive finding

The largest mistake was treating a historically validated hardware platform as a greenfield bring-up problem. The repository itself reinforced that framing: the README described the firmware as "from-scratch" and stated that several physical inputs still required validation. That was internally consistent with the new codebase, but it did not preserve the distinction between:

1. hardware that had already worked in the original product/system;
2. facts recovered from schematics/manuals/source;
3. facts newly revalidated on the current specimen;
4. assumptions introduced while rebuilding the software.

Once those categories were mixed, communication and technical decisions became harder than necessary.

## What went wrong

### 1. We did not freeze the legacy validated baseline before redesigning

We should have started with a "known-good baseline recovery" phase. Instead, architecture and new firmware structure advanced while some hardware facts were still being rediscovered.

Missing baseline evidence included items such as exact board revision, pin truth, sensor polarity/zero/range, encoder direction/count convention, motor channel mapping, power conditions, original firmware behavior, and the exact meaning of "validated" for each subsystem.

### 2. Provenance was not explicit enough

A statement such as "PA7 is the pendulum ADC" can come from several different sources: schematic, old source code, datasheet, runtime observation, or inference. These are not equivalent.

We need explicit provenance tags:

- LEGACY-PROVEN: demonstrated by the original validated system;
- DOC: stated by schematic/manual/datasheet;
- CODE: encoded by prior implementation;
- MEASURED: newly observed on the current specimen;
- INFERRED: engineering inference, not yet verified;
- UNKNOWN: unresolved.

A fact should not silently move from INFERRED to MEASURED.

### 3. "Hardware validated" and "current implementation validated" were conflated

The old platform may have been valid, while the new implementation of its interface was not yet valid. That distinction should have been explicit in every subsystem status table.

The correct question is not "is the ADC validated?" but:

- Was this signal proven on the legacy system?
- Is the schematic mapping known?
- Is the new driver mapping correct?
- Has the current board specimen been measured?
- Has the new software interpretation been compared with physical truth?

### 4. We allowed stale documentation to act as truth

Repository documentation can lag implementation. One example already observed is display naming: project material can still refer to SSD1306 while the actual driver is SSD1315. Stale documentation must be treated as evidence with a freshness state, not as unquestioned truth.

### 5. We did not establish a formal hardware truth table early enough

A single table should have existed before substantial control work. It should cover each I/O, polarity, scaling, physical meaning, source, current validation state, and test evidence.

Without that table, the same facts were repeatedly rediscovered in chat and code review.

### 6. Safety state was initially structurally ambiguous

The control pipeline initially reported CONFIG faults because state-safety limits were intentionally left unconfigured. That was safe, but diagnostically ambiguous: it did not distinguish "physical limits not yet approved" from "software safety object missing." We later corrected this with an explicit observe-only safety profile.

Lesson: represent "configured but non-enforcing/observe-only" explicitly instead of overloading "unconfigured."

### 7. Optional diagnostics were temporarily used as safety-state transport

The closed-loop gate initially consumed optional trace capture. That made diagnostic infrastructure part of a safety decision path. We corrected this by exposing dedicated pipeline runtime status and making unavailable/stale status fail closed.

Lesson: telemetry, trace, UI, and logging must observe safety state, never carry it.

### 8. The state-machine admission path was not fully defined before the gate was added

The gate correctly rejected DISABLED mode, but there was no legitimate transition path from DISABLED into an admission-ready state. This produced a control-architecture chicken-and-egg problem. We later introduced IDLE as the pre-active admission state and a dedicated mode manager.

Lesson: before implementing an admission gate, define the entire state transition graph, including pre-active, active, fault, disable, and recovery paths.

### 9. Performance budgets arrived too late

After state safety became active, average control-path time increased from roughly 142 us to about 200 us. The system still met the 1 ms deadline, but the regression was material and had to be opened as a separate investigation.

Lesson: establish timing budgets and regression thresholds before enabling additional runtime layers.

### 10. Tooling mistakes created avoidable repository noise and risk

Observed process failures included:

- an earlier whole-file replacement risk on `app/main.c`, caught before the user pulled it;
- candidate branches left behind, causing GitHub to suggest opening a PR and confusing the workflow;
- accidental temporary/no-op files and cleanup commits;
- a missing `<stddef.h>` include that caused the new host test to fail to build.

These were not control-theory failures. They were repository-operation and review-discipline failures.

## What worked well

Several corrective practices proved effective:

- atomic commits with one engineering objective;
- `git pull --ff-only` before local validation;
- deterministic `post_patch_check.sh` output;
- host tests before target flashing;
- build identity and ELF/HEX/BIN hashes;
- runtime logs tied to exact firmware commit IDs;
- keeping automatic control motor output unbound during architecture validation;
- dedicated GitHub Issues for safety, architecture, performance, and commissioning dependencies;
- fail-closed behavior when runtime state is unavailable;
- separating structural validation from physical commissioning.

The current `617af05` post-patch check demonstrates this improved discipline: clean tree, 26/26 host tests PASS, STM32 build PASS, exact artifact hashes, and an explicit next validation step.

## Root-cause model

The failures can be summarized as four interacting root causes:

```text
Legacy knowledge not captured as structured evidence
                    +
Chat conversation used as temporary system memory
                    +
Greenfield software framing applied to known hardware
                    +
Repository/Drive roles not defined early enough
                    =
Repeated rediscovery, assumption drift, communication ambiguity,
and avoidable implementation churn
```

## Permanent corrective actions

1. Every reused/known hardware project starts with a Rebaseline Gate before feature development.
2. Maintain a Hardware Truth Table with provenance and validation state.
3. Keep assumptions in an explicit Assumption Register; unresolved assumptions cannot silently become requirements.
4. Define source-of-truth ownership between GitHub and Google Drive.
5. Every engineering change must have an evidence chain: Issue -> Commit -> Build/Test -> Runtime/Measurement -> Conclusion.
6. Safety decisions must consume dedicated runtime state, not optional diagnostics.
7. Establish timing, memory, and safety budgets before active control.
8. Keep automatic actuation physically or logically unbound until a named commissioning gate is passed.
9. Candidate branches are temporary implementation details and must not be used as user-facing workflow unless explicitly requested.
10. Large-file edits require exact-file fetch, candidate diff verification, zero unexpected deletions, then fast-forward only.

## Key principle for future projects

**Do not restart a known system from zero. Restart from evidence.**

The first deliverable is not new code. It is a reliable model of what is already known, what was once proven, what is proven now, and what remains uncertain.
