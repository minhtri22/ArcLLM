# ARCLLM_TTFT_M1 — P6 Terminal Infrastructure Adjudication

**Date:** 2026-09-22  
**Result:** `P6_STOP_INFRASTRUCTURE_UNSTABLE_BEFORE_BUILDONLY_EXECUTION`

No BuildOnly runner was executed, no target model was loaded, no GPU dispatch occurred, and no fresh TTFT observation exists.

## Why P6 must stop

The first pre-run defect was the H-ART operationalization tautology. It was formally repaired before execution, and the single P6 BuildOnly repair allowance was conservatively consumed.

Lock v0.2 then froze:

```text
repair_budget_consumed = 1 / 1
further_p6_buildonly_repair_permitted = false
```

A later audit found a second runner defect after lock v0.2:

- preflight correctly points to implementation lock v0.2;
- the result JSON still resolves the implementation-lock blob from v0.1;
- the bundle destination still labels the copied v0.2 lock as a v0.1 filename.

This would make the returned evidence provenance stale/ambiguous.

Fixing those references would require changing the locked runner after the only allowed repair had already been consumed. That is explicitly disallowed.

## Scientific consequence

This is an infrastructure/governance stop, not evidence for or against any TTFT mechanism.

No result is available for:
- H-ART;
- H-DPIPE;
- H-PRECOND;
- state interaction;
- H-NULL.

No parent ANL64 conclusion changes.

## Required action

```text
DO NOT RUN run_ttft_m1_p6_buildonly.ps1
DO NOT CREATE LOCK v0.3
DO NOT PATCH AND CONTINUE THIS PROGRAM
P7 REMAINS BLOCKED
```

The only valid remaining action is final program adjudication with **no mechanism result**.
