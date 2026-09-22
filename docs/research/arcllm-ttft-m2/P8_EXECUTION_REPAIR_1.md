# TTFT_M2 — P8 Execution-Stage Repair 1

**Date:** 2026-09-22  
**Classification:** `VALID_EXECUTION_STAGE_ORCHESTRATION_DEFECT`

## Failed first P8 attempt

GitHub Actions run:

`35678683835`

Head:

`1ecb6b7606f81e71fd7c5c6eb0f0d12c5838e2a1`

Observed:
- workflow status: completed;
- conclusion: failure;
- jobs returned by Actions API: 0;
- exact frozen BuildOnly runner executions: 0.

The failure occurred before job creation.

## Defect

`P8_WORKFLOW_YAML_PLAIN_SCALAR_COLON_PARSE_FAILURE`

The one-shot job condition was emitted as a YAML plain scalar while the compared commit message contained the sequence `: `. That made the workflow definition invalid before any Windows job could start.

This is an orchestration defect introduced by the P8 execution harness. It does not modify or invalidate the frozen P4 execution package and did not expose any scientific outcome.

## Bounded repair

The only repair is:
- keep the exact frozen P4 package unchanged;
- keep exact P7 authorization blob `8faf0cd91da381333fdf7971e431fc091701a62b`;
- replace the invalid job predicate with a colon-free one-shot predicate;
- use commit message `P8-REPLAY-ONE-SHOT`;
- replay the full P8 execution from zero.

No package member, science design, model, GPU, timing or endpoint logic is changed.

## Repair budget

```text
maximum execution-stage repairs   1
before repair consumed            0
after repair consumed             1
remaining                         0
```

If the replay exposes a second execution-stage infrastructure defect, governance requires:

`STOP_INFRASTRUCTURE_UNSTABLE`

## Scientific accounting

```text
BuildOnly runner executions          0
diagnostic executable launches       0
target model loads                    0
GPU dispatches                        0
performance measurements              0
fresh TTFT observations               0
scientific result                   NONE
```

Next: full fresh P8 replay from zero under repair 1/1.
