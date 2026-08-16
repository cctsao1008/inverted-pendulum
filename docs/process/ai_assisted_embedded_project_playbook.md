# AI-Assisted Embedded Project Playbook

## Scope

Reusable workflow for embedded/control projects where ChatGPT collaborates with a human engineer through GitHub and Google Drive without autonomous agent execution.

## Roles

### Human engineer

Owns physical truth, lab safety, hardware access, final engineering approval, flashing, and measurements.

### ChatGPT

Owns structured analysis, repository inspection, proposed/approved atomic edits, traceability, test interpretation, issue management, and documentation consistency.

ChatGPT must not invent missing hardware facts or promote assumptions to facts.

## Source-of-truth split

### GitHub

Use GitHub for information that must version with the code:

- source and build scripts;
- architecture and ADRs;
- hardware truth tables that affect implementation;
- test definitions and acceptance criteria;
- issue/dependency tracking;
- commits and tags;
- concise validation records and links to external evidence;
- reusable templates.

### Google Drive

Use Drive for larger or externally sourced evidence:

- raw UART logs and measurement captures;
- photos/videos;
- vendor PDFs and schematics;
- long-form reports;
- spreadsheets/datasets;
- release evidence packages;
- meeting notes and project dashboards.

Rule: do not maintain two independently editable copies of the same technical truth. GitHub contains the versioned decision; Drive contains the evidence artifact. Cross-link them.

## Project lifecycle gates

### Gate 0 — Rebaseline

Before feature development:

- identify exact hardware revision/specimen;
- recover historical known-good behavior;
- build Hardware Truth Table;
- build Assumption Register;
- archive source documents;
- establish toolchain and reproducible build;
- capture baseline firmware/runtime identity;
- mark each subsystem LEGACY-PROVEN / MEASURED / DOC / CODE / INFERRED / UNKNOWN.

Exit only when the team knows which facts are proven and which are not.

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
2. State one engineering objective
3. Identify assumptions and safety impact
4. Make one atomic change
5. Verify exact diff; no unrelated deletions
6. Fast-forward only
7. Human: git pull --ff-only
8. Human: post_patch_check.sh
9. Flash only if runtime behavior changed
10. Capture runtime/measurement evidence
11. Classify PASS / FAIL / INCONCLUSIVE
12. Update issue and evidence index
13. Proceed only if gate criteria are satisfied
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

## Communication contract

For every non-trivial decision, distinguish:

- FACT: directly supported by evidence;
- INFERENCE: engineering interpretation of facts;
- ASSUMPTION: required to proceed but not proven;
- DECISION: chosen engineering direction;
- TODO: unresolved verification action.

When disagreement exists, resolve the evidence category before debating the design.

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

Use Issues for work that has acceptance criteria. Use ADRs for decisions. Use commits for implementation. Do not use PRs for a solo direct-commit workflow unless review/branch isolation is intentionally requested.

## Drive management pattern

Suggested reusable folder structure:

```text
00_Plans
01_Test_Procedures
02_Raw_Logs
03_Reports
04_References
05_ADRs_Exports
06_Experiments
07_Dashboard
08_Releases
09_Datasets
```

File names should include date and identity when useful:

```text
YYYYMMDD_<project>_<test>_<commit8>_<result>.log
YYYYMMDD_<project>_<experiment>_<board-rev>.md
```

## Stop conditions

Stop development and rebaseline when any of these occur:

- hardware identity is uncertain;
- documentation and measured behavior conflict;
- a safety decision depends on optional logging/telemetry;
- a new runtime feature changes timing materially without attribution;
- the build cannot be reproduced;
- repository history contains unexpected changes;
- a physical output path becomes enabled earlier than planned;
- acceptance criteria cannot be stated.

## Review cadence

At each major gate, perform a short retrospective:

- What did we believe?
- What evidence changed that belief?
- What did we implement too early?
- Which assumptions remain?
- What should become a reusable template or automated check?

This prevents chat history from becoming the only memory of why the project evolved.
