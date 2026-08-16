# AI-Assisted Embedded Project Playbook

## Scope

Reusable workflow for embedded/control projects where ChatGPT collaborates with a human engineer through GitHub and Google Drive without autonomous agent execution.

## Roles

### Human engineer

Owns physical truth, lab safety, hardware access, final engineering approval, flashing, and measurements.

### ChatGPT

Owns structured analysis, repository inspection, proposed/approved atomic edits, traceability, test interpretation, issue management, and documentation consistency.

ChatGPT must not invent missing hardware facts, promote assumptions to facts, or silently import terminology/mental models from another engineering domain.

## Project Identity Contract

Before technical design or restart work, define and freeze a short identity block:

- system / project name;
- physical plant or controlled system;
- actuator(s);
- controlled coordinates / state variables;
- primary operating modes;
- hardware model/revision/specimen;
- legacy known-good behavior;
- domain vocabulary;
- foreign-domain terms or analogies that must not be used as shorthand when they could change the mental model.

Re-check the identity block during every rebaseline, project handoff, architecture review, or long interruption.

A terminology error is not automatically a harmless wording issue. In a control project, language such as `flight`, `takeoff`, or `throttle` can imply a different plant, authority model, or commissioning procedure. Treat domain-language drift as an engineering communication defect unless it is an explicit comparison.

## Evidence provenance and capability maturity

For factual provenance, use:

- `LEGACY-PROVEN`
- `DOC`
- `CODE`
- `MEASURED`
- `INFERRED`
- `UNKNOWN`

For implementation maturity, use:

- `TARGET`
- `STUB`
- `IMPLEMENTED`
- `HOST-VALIDATED`
- `RUNTIME-VALIDATED`
- `PHYSICALLY-COMMISSIONED`

A source file, architecture block, successful build, or historical system behavior must never be described as a commissioned capability unless the matching evidence exists.

## Source-of-truth split

### GitHub

Use GitHub for information that must version with the code:

- source and build scripts;
- source-coupled architecture and contracts;
- hardware truth that directly affects implementation;
- test definitions and acceptance criteria;
- issue/dependency tracking;
- commits and tags;
- concise validation records and links to external evidence;
- reusable templates.

### Google Drive

Use Drive for long-lived project evidence and history:

- raw UART logs and measurement captures;
- photos/videos;
- vendor PDFs and schematics;
- long-form ADR / decision records;
- long-form reports;
- project plans and historical checkpoints;
- spreadsheets/datasets;
- project dashboards;
- release evidence packages;
- meeting/project records.

Rule: do not maintain two independently authoritative copies of the same technical truth. Source-coupled implementation/architecture truth stays in GitHub. Drive may hold long-form decision history and evidence, but each record should identify its snapshot date/commit and point back to the relevant GitHub issue, commit, or architecture document.

## Baseline policy

Do not use one commit as every kind of baseline. Track these separately:

- **Current GitHub main** — latest repository truth, which may contain documentation-only commits not yet flashed.
- **Latest build/test-validated commit** — latest source commit validated by the local build/test workflow.
- **Latest runtime-validated/flashed commit** — latest firmware identity proven on the embedded target.
- **Historical performance/reference baseline** — an earlier known checkpoint retained for comparison.

These may legitimately differ during development. Every dashboard, report, and experiment should state which baseline type it means.

## Project lifecycle gates

### Gate 0 — Rebaseline

Before feature development:

- freeze the Project Identity Contract and domain vocabulary;
- identify exact hardware revision/specimen;
- recover historical known-good behavior;
- build Hardware Truth Table;
- build Assumption Register;
- archive/index source documents;
- establish toolchain and reproducible build;
- record the separate baseline identities;
- mark each subsystem fact `LEGACY-PROVEN / MEASURED / DOC / CODE / INFERRED / UNKNOWN`;
- mark implemented capabilities with an explicit maturity state.

Exit only when the team knows which facts are proven, which implementation capabilities are actually validated, and which are not.

### Gate 1 — Observe-only software integration

- drivers compile and run;
- acquisition paths produce diagnostics;
- no automatic physical actuation;
- state safety is explicit;
- telemetry is diagnostics-only;
- runtime timing/memory baseline recorded.

### Gate 2 — Admission architecture

- operator intent explicit;
- state-machine transitions defined;
- admission gate fail-closed;
- run-permit semantics defined separately;
- safety status sourced from dedicated runtime state;
- fault and disable paths tested.

### Gate 3 — Controlled actuation

- motor/actuator authority explicit;
- controller-selection authority is distinguished from physical actuator authority;
- magnitude/rate limiters active;
- emergency stop verified;
- physical limits have provenance;
- current-limited supply and mechanical safety setup documented.

### Gate 4 — Plant identification and controller commissioning

- measured plant data archived;
- controller parameters trace to evidence;
- performance/safety envelope defined;
- staged commissioning with rollback criteria.

## Standard change loop

```text
1. Read latest main and relevant issue/evidence
2. Reconfirm project identity if the change crosses domains/subsystems
3. State one engineering objective
4. Identify assumptions and safety impact
5. Make one atomic change
6. Verify exact diff; no unrelated deletions
7. Fast-forward only
8. Human: git pull --ff-only
9. Human: post_patch_check.sh
10. Flash only if runtime behavior changed
11. Capture runtime/measurement evidence
12. Classify PASS / FAIL / INCONCLUSIVE
13. Update issue, dashboard, and evidence index
14. Proceed only if gate criteria are satisfied
```

## Evidence chain

Every important result should be traceable as:

```text
Requirement / Problem
  -> GitHub Issue
  -> Commit SHA
  -> build/test log
  -> firmware artifact SHA256
  -> runtime/measurement log
  -> conclusion
  -> ADR / next issue if a decision changed
```

For physical experiments, the stronger chain is:

```text
Git commit
  -> built artifact identity/SHA256
  -> flashed target
  -> runtime/measurement log
  -> experiment record
  -> conclusion / next action
```

## Communication contract

For every non-trivial decision, distinguish:

- FACT: directly supported by evidence;
- INFERENCE: engineering interpretation of facts;
- ASSUMPTION: required to proceed but not proven;
- DECISION: chosen engineering direction;
- TODO: unresolved verification action.

When disagreement exists, resolve the evidence category before debating the design.

For hardware/control properties, avoid coarse labels such as `hardware verified` when the properties differ. Split mapping, signal existence, zero/reference, direction/sign, scale, range, noise/repeatability, timing, and control suitability where applicable.

## GitHub management pattern

Recommended Issue types/labels:

- `type: hardware`
- `type: firmware`
- `type: architecture`
- `type: safety`
- `type: performance`
- `type: test`
- `stage: rebaseline`
- `stage: observe-only`
- `stage: commissioning`
- `risk: high`
- `blocked`

Use Issues for work that has acceptance criteria. Use source-coupled architecture documents for implementation contracts. Use Drive ADRs or GitHub decision records according to the project convention, but never let both become independently authoritative. Use commits for implementation.

Do not use PRs for a solo direct-commit workflow unless review/branch isolation is intentionally requested. Temporary candidate branches are implementation details; do not expose them as a user workflow unless intentionally chosen. Keep any unavoidable helper branch fast-forward aligned or delete it after use.

## Repository write discipline

For safety-critical or large files:

1. read latest `main` immediately before the write;
2. fetch the complete exact file, not a truncated snippet;
3. prefer a candidate tree/blob commit when available;
4. compare base -> candidate and confirm the exact file list/additions/deletions;
5. reject unexpected deletions or unrelated files;
6. update `main` only by fast-forward (`force=false`);
7. if the base moved, re-read/rebase rather than force.

A whole-file contents-API replacement is acceptable only when the complete exact source file has been verified and the resulting diff is reviewed. It should not be the casual default for large source files.

## Drive management pattern

Suggested reusable folder structure:

```text
00_Project_Plans
01_Test_Records
02_Runtime_Logs
03_Reports
04_References
05_Decision_Records
06_Experiments
07_Project_Dashboard
08_Releases
09_Datasets_Record_Replay
```

File names should include date and identity when useful:

```text
YYYYMMDD_<project>_<test>_<commit8>_<result>.log
YYYYMMDD_<project>_<experiment>_<board-rev>.md
```

Drive current-status documents should include a freshness rule:

- a **Current Plan** is rewritten/created when the active development position changes materially;
- old plans are marked **HISTORICAL / SUPERSEDED**, not silently edited into a new past;
- dashboards separate current main, build/test baseline, runtime baseline, and historical references;
- ADR validation sections are updated as implementation progresses without rewriting the original decision context;
- duplicated architecture summaries in Drive are marked historical when GitHub holds the authoritative source-coupled architecture.

## Stop conditions

Stop development and rebaseline when any of these occur:

- system/plant identity or domain vocabulary is drifting;
- hardware identity is uncertain;
- documentation and measured behavior conflict;
- GitHub source-coupled status and Drive current dashboard/plan materially disagree;
- a safety decision depends on optional logging/telemetry;
- a new runtime feature changes timing materially without attribution;
- the build cannot be reproduced;
- repository history contains unexpected changes;
- a physical output path becomes enabled earlier than planned;
- a capability is being called validated/commissioned without evidence;
- acceptance criteria cannot be stated.

## Review cadence

At each major gate, interruption, or project resume, perform a short retrospective:

- What system/plant are we actually controlling?
- What did we believe?
- What evidence changed that belief?
- Which source is authoritative for this statement?
- What did we implement too early?
- Which assumptions remain?
- Are the current plan/dashboard and GitHub main describing the same development position?
- What should become a reusable template or automated check?

This prevents chat history from becoming the only memory of why the project evolved and prevents terminology drift from silently becoming architecture drift.
