# TTFT_M2 — P8 Bounded BuildOnly Execution

**Date:** 2026-09-22  
**Terminal result:** `STOP_INFRASTRUCTURE_UNSTABLE`

## Scope

P8 was authorized by exact P7 authorization blob:

`8faf0cd91da381333fdf7971e431fc091701a62b`

The only permitted executable was the frozen runner:

`run_ttft_m2_buildonly.ps1`

blob:

`4722e86a01453d973ee2122b49229b80bf7d84f6`

No target-model, diagnostic-science, GPU, timing or fresh-TTFT execution was authorized.

## Attempt 1

Workflow run:

`35678683835`

head:

`1ecb6b7606f81e71fd7c5c6eb0f0d12c5838e2a1`

Result:
- status: completed;
- conclusion: failure;
- jobs observed: 0;
- exact runner executions: 0.

The one-shot orchestration workflow failed to parse because its plain-scalar `if` expression contained the sequence `: ` inside the compared commit message.

Classification:

`VALID_EXECUTION_STAGE_ORCHESTRATION_DEFECT`

This consumed the one allowed execution-stage repair.

## Repair 1/1

Repair evidence blob:

`0a21904295708660df7cb06ceda8163768d6b50c`

Repaired workflow blob:

`45bf73ca7aad83a361299ce4ea7c4490070b564f`

Only the orchestration predicate changed. The frozen execution package and scientific design were not modified.

A full replay from zero was required.

## Attempt 2 — full replay from zero

Workflow run:

`35678861153`

head:

`2fcb9756d032d806e52bc521457c7c49dfe6f868`

The workflow parsed successfully and GitHub created job:

`106591157811`

Observed:
- workflow name resolved correctly: `TTFT M2 P8 Bounded BuildOnly`;
- status: completed;
- conclusion: failure;
- jobs observed: 1;
- steps observed: 0;
- job logs: unavailable (404);
- workflow artifacts: 0;
- exact runner executions: 0.

The evidence supports only the bounded classification:

`SECOND_EXECUTION_STAGE_INFRASTRUCTURE_STARTUP_FAILURE_UNRESOLVED`

The job failed before any observable workflow step. Because no job log exists, P8 does not speculate about a narrower external cause such as billing, runner allocation or account policy.

## Repair-budget adjudication

```text
maximum repairs       1
repairs consumed      1
remaining             0
```

The governance rule for a second execution-stage defect is:

`STOP_INFRASTRUCTURE_UNSTABLE`

Therefore no second repair, runner rewrite, execution-environment switch, or third attempt is admissible inside TTFT_M2.

## Frozen package integrity

After both attempts:
- runner blob remains `4722e86a01453d973ee2122b49229b80bf7d84f6`;
- lock blob remains `9af4c7b4c96354223e1f671a43af64b215072330`;
- package manifest blob remains `4629910255580322706b01f318ad9a80044208d7`;
- P7 authorization blob remains `8faf0cd91da381333fdf7971e431fc091701a62b`.

No frozen package mutation occurred.

## Scientific accounting

```text
exact BuildOnly runner executions       0
diagnostic executable launches          0
target model loads                       0
GPU dispatches                           0
performance measurements                 0
fresh TTFT observations                  0
scientific result                      NONE
mechanism adjudication                  false
```

## P8 conclusion

`STOP_INFRASTRUCTURE_UNSTABLE`

P8 did not obtain a valid BuildOnly execution result. P9 is therefore blocked and must not open.

The next admissible stage is:

`M2_P10_FINAL_PROGRAM_ADJUDICATION`

P10 should close TTFT_M2 as an infrastructure stop with no mechanism result, unless governance is explicitly amended in a new independent program rather than rescued inside M2.
