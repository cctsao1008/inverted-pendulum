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

A second audit finding was that the core GitHub control architecture had remained a rotary inverted-pendulum architecture, but one later chat explanation imported flight-control imagery (`flight`, `takeoff`, `throttle`). That wording did not prove that the codebase had been designed as flight control, but it exposed a domain-language contamination risk: a foreign analogy can silently change the reader's mental model even when the underlying architecture is correct.

## What went wrong

### 1. We did not freeze the legacy validated baseline before redesigning

We should have started with a "known-good baseline recovery" phase. Instead, architecture and new firmware structure advanced while some hardware facts were still being rediscovered.

Missing baseline evidence included items such as exact board revision, pin truth, sensor polarity/zero/range, encoder direction/count convention, motor channel mapping, power conditions, original firmware behavior, and the exact meaning of "validated" for each subsystem.

### 2. Provenance was not explicit enough

A statement such as "PA7 is the pendulum ADC" can come from several different sources: schematic, old source code, datasheet, runtime observation, or inference. These are not equivalent.

We need explicit provenance tags:

- LEGACY-PROVEN: demonstrated by the original validated system;
- DOC: stated by schematic/manual/datasheet;
- CODE: encoded by the implementation;
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

Repository documentation can lag implementation. One observed example was display naming: project material still referred to SSD1306 while the active Forest target build used the SSD1315-class driver.

The later project-truth audit corrected README and the source-coupled control architecture. The lesson is broader: stale documentation must be treated as evidence with a freshness state, not as unquestioned truth.

### 5. We did not establish a formal hardware truth table early enough

A single table should have existed before substantial control work. It should cover each I/O/property, polarity, scaling, physical meaning, source, current validation state, and test evidence.

Without that table, the same facts were repeatedly rediscovered in chat and code review.

### 6. Safety state was initially structurally ambiguous

The control pipeline initially reported CONFIG faults because state-safety limits were intentionally left unconfigured. That was safe, but diagnostically ambiguous: it did not distinguish "physical limits not yet approved" from "software safety object missing." We later corrected this with an explicit observe-only safety profile.

Lesson: represent "configured observe-only" explicitly instead of overloading "unconfigured."

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
- a missing `<stddef.h>` include that caused a new host test to fail to build;
- an attempted README documentation edit during the retrospective that accidentally replaced most of README before restoration.

These were not control-theory failures. They were repository-operation and review-discipline failures.

The later project-truth audit intentionally changed README and `docs/architecture/control_architecture.md`, so an earlier statement that README had "no diff" after restoration is historical only and must not be treated as current repository state.

### 11. Domain-language contamination was detected in communication

The project is a **rotary inverted pendulum**, with a rotary-arm motor and pendulum/arm states. A chat explanation nevertheless used flight-control / takeoff / throttle imagery.

The repository audit found that the core control architecture itself was still expressed as a rotary inverted pendulum: pendulum angle/rate, rotary-arm position/rate, SWING_UP/CAPTURE/BALANCE, actuator mapping, and TB6612 motor drive. The foreign terminology was therefore a communication defect rather than evidence that the whole implementation had been designed as flight control.

That distinction matters, but the wording error is still serious because engineering language carries architecture assumptions. The permanent corrective rule is to keep a Project Identity Contract and explicit domain vocabulary, and to treat foreign-domain shorthand as a defect when it could alter the mental model.

### 12. "The baseline" was overloaded

During rapid development, the same word was used for different things:

- latest GitHub main;
- latest locally build/test-validated commit;
- latest flashed/runtime-validated commit;
- an older performance-reference commit.

These are not interchangeable. Documentation-only commits can advance `main` without changing runtime behavior, while a historical runtime checkpoint can remain useful for performance comparison long after it stops being current.

Lesson: every plan, dashboard, validation record, and discussion should name the baseline type explicitly.

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

The `617af05` post-patch check is an example of the improved discipline: clean tree, 26/26 host tests PASS, STM32 build PASS, exact artifact hashes, and an explicit next validation step. That establishes build/test integration for the runtime mode-transition wiring; it does not by itself replace the required embedded runtime behavior validation for issue #30.

## Root-cause model

The failures can be summarized as interacting root causes:

```text
Legacy knowledge not captured as structured evidence
                    +
Chat conversation used as temporary system memory
                    +
Greenfield software framing applied to known hardware
                    +
Repository/Drive roles not defined early enough
                    +
Project identity/domain vocabulary not frozen
                    +
Baseline types not separated
                    =
Repeated rediscovery, assumption drift, communication ambiguity,
stale-status drift, and avoidable implementation churn
```

## Permanent corrective actions

1. Every reused/known hardware project starts with a Rebaseline Gate before feature development.
2. Freeze a Project Identity Contract: plant/system, actuators, controlled states, modes, hardware identity, legacy behavior, and domain vocabulary.
3. Maintain a Hardware Truth Table with property-level provenance and validation state.
4. Keep assumptions in an explicit Assumption Register; unresolved assumptions cannot silently become requirements.
5. Distinguish fact provenance from capability maturity. Use `TARGET / STUB / IMPLEMENTED / HOST-VALIDATED / RUNTIME-VALIDATED / PHYSICALLY-COMMISSIONED` for the latter.
6. Define source-of-truth ownership between GitHub and Google Drive and keep current plans/dashboards fresh.
7. Separate current main, build/test-validated, runtime-validated, and historical performance/reference baselines.
8. Every engineering change must have an evidence chain: Issue -> Commit -> Build/Test -> Runtime/Measurement -> Conclusion.
9. Safety decisions must consume dedicated runtime state, not optional diagnostics.
10. Establish timing, memory, and safety budgets before active control.
11. Keep automatic actuation physically or logically unbound until a named commissioning gate is passed.
12. Candidate branches are temporary implementation details and must not be used as user-facing workflow unless explicitly requested.
13. Large-file edits require exact-file fetch, candidate diff verification, zero unexpected deletions, then fast-forward only.
14. Re-audit project identity, source-of-truth freshness, and baseline labels after a major interruption or before resuming physical commissioning.

## Project-truth audit result — 2026-08-16

The audit did **not** find evidence that the repository had been architected as a flight-control system. It did find documentation/status drift that could have caused a future reader or AI session to build the wrong mental model.

Corrective documentation actions included:

- README changed from greenfield `from-scratch` framing to re-engineering/revalidation of the existing Forest rotary inverted-pendulum platform;
- active SSD1315-class display identity made explicit;
- hardware validation represented property by property rather than one coarse flag;
- control computation separated from physical actuator authority in the authoritative architecture;
- Control State Machine authority separated from Motor Authority Arbiter authority;
- capability maturity terminology added;
- Drive historical/current plans and dashboard rebaselined;
- reusable project playbook and rebaseline checklist strengthened.

## Key principle for future projects

**Do not restart a known system from zero. Restart from evidence.**

The first deliverable is not new code. It is a reliable model of what is already known, what was once proven, what is proven now, what the current implementation actually supports, and what remains uncertain.
