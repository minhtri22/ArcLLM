# TTFT_M2 — P10 Final Program Adjudication

**Date:** 2026-09-22  
**Terminal status:** `TTFT_M2_PROGRAM_CLOSED_INFRASTRUCTURE_STOP_NO_MECHANISM_RESULT`

## Final verdict

`INFRASTRUCTURE_STOP_NO_MECHANISM_RESULT`

TTFT_M2 is formally closed.

This closure is an infrastructure stop. It is **not** a scientific TTFT-mechanism result.

## Closure chain

The program reached the following governed states:

```text
P4  zero-science package implemented
P5  atomic package QA PASS
P6  zero-science governance PASS
P7  exact frozen BuildOnly authorized
P8  STOP_INFRASTRUCTURE_UNSTABLE
P9  BLOCKED_NOT_OPENED
P10 FINAL PROGRAM ADJUDICATION
```

The relevant evidence bindings are:

- P5 artifact: `3012c817e2983d0928cd7599cfa70197c4debd3c`
- P6 artifact: `4ffae2c16d60d02a63c9cdbe96be1f5d1e7e0ad9`
- P7 authorization: `8faf0cd91da381333fdf7971e431fc091701a62b`
- P8 terminal artifact: `ae7b17678b9cbb29018c7e0207a12a5fb6b99ac9`

## Why P8 is terminal

P8 did not obtain a valid execution of the exact frozen BuildOnly runner.

### Attempt 1

GitHub Actions run `35678683835` failed before job creation.

Exact BuildOnly runner executions:

`0`

The failure was classified as a valid execution-stage orchestration defect and consumed the one permitted repair.

### Repair 1/1

Repair record:

`0a21904295708660df7cb06ceda8163768d6b50c`

The repair changed only orchestration and required a full replay from zero.

### Attempt 2

GitHub Actions run `35678861153` created job `106591157811` but failed before any observable step.

Observed:
- workflow jobs: 1;
- observable steps: 0;
- job logs: unavailable;
- artifacts: 0;
- exact BuildOnly runner executions: 0.

The bounded classification is:

`SECOND_EXECUTION_STAGE_INFRASTRUCTURE_STARTUP_FAILURE_UNRESOLVED`

No narrower cause is claimed.

At that point the execution-stage repair budget was:

```text
maximum repairs   1
consumed          1
remaining         0
```

The preregistered second-defect action was therefore mandatory:

`STOP_INFRASTRUCTURE_UNSTABLE`

No second repair, third attempt, environment switch, or local rescue is admissible inside TTFT_M2.

## P9 status

P9 never opened.

```text
P9 fresh mechanism identification     BLOCKED_NOT_OPENED
target model execution                 0
diagnostic executable launches         0
GPU dispatches                         0
performance measurements               0
fresh TTFT observations                0
```

There is therefore no fresh M2 scientific dataset and no mechanism-identification outcome.

## Scientific interpretation boundary

The P8 infrastructure failures **must not** be interpreted as evidence for or against any TTFT mechanism.

TTFT_M2 does not:
- support any candidate TTFT mechanism;
- falsify any candidate TTFT mechanism;
- rank or prefer any candidate mechanism;
- establish a TTFT-removal intervention;
- attribute the observed infrastructure failures to a model, GPU, shader, precondition, state interaction, or other scientific TTFT mechanism.

Those questions remain **not adjudicated** by TTFT_M2.

Infrastructure failure is infrastructure evidence only.

## Scientific accounting at close

```text
BuildOnly runner executions          0
diagnostic executable launches       0
target model loads                    0
GPU dispatches                        0
performance measurements              0
fresh TTFT observations               0
scientific result                   NONE
mechanism adjudication              false
```

## Frozen-package integrity

The frozen package was not mutated during P8:

- source HEAD: `7e807caf7dd357f9c820f89f5a719a1e4a10139a`
- runner blob: `4722e86a01453d973ee2122b49229b80bf7d84f6`
- execution lock blob: `9af4c7b4c96354223e1f671a43af64b215072330`
- package manifest blob: `4629910255580322706b01f318ad9a80044208d7`
- P7 authorization blob: `8faf0cd91da381333fdf7971e431fc091701a62b`

Thus the terminal stop is not a consequence of silently changing the frozen scientific/infrastructure package.

## Program closure

TTFT_M2 is now closed.

After P10:
- no further TTFT_M2 execution is authorized;
- no further TTFT_M2 repair is authorized;
- P9 cannot be opened retroactively;
- closed M2 evidence cannot be reinterpreted as a mechanism result;
- any future attempt must be a new independent governance program with its own origin, budget, authorization and lineage rather than an M2 rescue.

## Terminal statement

`TTFT_M2_PROGRAM_CLOSED_INFRASTRUCTURE_STOP_NO_MECHANISM_RESULT`

The program answered the infrastructure question negatively at the execution-stability boundary: the governed package could be specified, QA-validated, frozen and authorized, but a valid bounded BuildOnly execution was not obtained within the preregistered repair budget.

The scientific TTFT mechanism question remains unanswered by TTFT_M2.
