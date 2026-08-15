# Experiment Workflow

Every hardware experiment should be reproducible and traceable to source and binary identity.

## Standard flow

```text
patch
  -> git am
  -> tools/post_patch_check.sh
  -> flash validated artifact
  -> Windows serial_tool scenario
  -> runtime log
  -> experiment record
  -> PASS / FAIL / INCONCLUSIVE
```

## Minimum evidence

Record:

- Git commit;
- firmware build timestamp;
- ELF/HEX/BIN SHA256 when available;
- hardware setup;
- power conditions;
- runtime parameters and profile;
- exact command scenario;
- `post_patch_check.log`;
- runtime log;
- acceptance criteria;
- measured result;
- conclusion and next action.

## Acceptance criteria

A test should not be marked PASS merely because it did not crash. Define measurable criteria where practical, for example:

- loop average/max execution time;
- stale-sample threshold;
- command delivery reliability;
- fault/recovery behavior;
- actuator magnitude and rate limits;
- capture/recovery envelope.
