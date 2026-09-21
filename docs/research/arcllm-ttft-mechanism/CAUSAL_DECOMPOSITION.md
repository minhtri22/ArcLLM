# ARCLLM_TTFT_M1 — P3 Causal Decomposition Specification

**Date:** 2026-09-22  
**State:** frozen design / execution blocked.

## Design principle

P3 does not compare the old safe binary against the old ANL64 binary and call the difference a mechanism.

Instead it requires a **single diagnostic harness** with one exact common prefill path and independently controlled pre-`t0` state factors.

This eliminates binary-level confounding.

## 2×2 factorial

Factor D — prepared decode pipeline set:

```text
SAFE
Q4FAST
```

Factor C — conditioning immediately before each measured prefill:

```text
PREFILL_ONLY
FULL_INFERENCE
```

Arms:

```text
SP = SAFE    + PREFILL_ONLY
SF = SAFE    + FULL_INFERENCE
QP = Q4FAST  + PREFILL_ONLY
QF = Q4FAST  + FULL_INFERENCE
```

All four arms execute **the exact same prefill graph** for the measured TTFT.

## Per-attempt causal sequence

Each measured observation must be generated independently as:

```text
reset
    ↓
conditioning pass selected by arm
    ↓
reset
    ↓
t0
    ↓
exact common 441-dispatch prefill
    ↓
q2_top2
    ↓
t1
```

For `FULL_INFERENCE`, the conditioning pass generates the same frozen 32-token workload using that arm's decode pipeline set.

For `PREFILL_ONLY`, no decode dispatch occurs in conditioning.

Conditioning time is not a primary endpoint.

## Freshness and replication

Parent P6 timing observations reused:

```text
0
```

Future frozen collection, if separately authorized:

```text
2 independent processes/sessions
2 workloads
4 arms
5 measured TTFT observations / arm / workload / session
= 80 fresh primary observations
```

Session order is counterbalanced and frozen in the JSON contract.

## Primary endpoint

```text
TTFT_ms
```

Material threshold:

```text
1.10 ratio
```

The threshold is the pre-existing ArcLLM practical-materiality convention. It is not calibrated to the historical P6 effect size.

## Diagnostic decomposition telemetry

Without changing compute commands, a future diagnostic harness may expose:

```text
prefill_execute_wall_ms
submit_wait_ms
host_record_and_lifecycle_ms
top2_ms
```

These fields can localize where a TTFT delta occurs.

They **cannot override** the primary TTFT gate.

In particular, the study does not manipulate command-pool policy, so it cannot conclude that command-pool allocation itself is causal merely because host lifecycle time differs.

## Frozen contrasts

```text
H-DPIPE
QP / SP

H-PRECOND-SAFE
SF / SP

H-PRECOND-Q4FAST
QF / QP

PARENT-LIKE
QF / SF

STATE INTERACTION
(QF / SF) / (QP / SP)
```

All ratios use medians of the five fresh measured observations inside a single session/workload cell. Sessions and workloads are never pooled to rescue a failed gate.

## Primary support rules

### H-DPIPE

Supported only if:

```text
QP / SP >= 1.10
```

for W-S and W-C independently in both sessions.

### H-PRECOND

Supported only if Q4FAST has material full-warmup harm while SAFE does not:

```text
QF / QP >= 1.10
AND
SF / SP < 1.10
```

for both workloads in both sessions.

### H-STATE-INTERACTION

Supported only if the parent-like contrast and the multiplicative interaction are both material:

```text
QF / SF >= 1.10
AND
(QF / SF) / (QP / SP) >= 1.10
```

for both workloads in both sessions.

### H-NULL

Supported only if **no H-DPIPE, H-PRECOND, or H-STATE-INTERACTION gate passes** and the parent-like contrast is non-material everywhere:

```text
QF / SF < 1.10
```

for both workloads in both sessions after exact prefill artifact identity has been established.

### UNRESOLVED_STABLE_HARM

If `QF/SF >=1.10` reproducibly but none of the preregistered mechanism gates pass, the correct result is unresolved stable harm. No post-hoc mechanism may be invented.

## H-ART is a hard pre-timing gate

Before any timing execution, all common prefill SPIR-V artifacts must be SHA256-compared.

If any differ:

```text
H-ART_SUPPORTED_STATIC
STOP FACTORIAL TIMING
```

The only scientifically valid next work would be a separately specified artifact-normalization intervention. No timing result is needed to claim the artifact mismatch exists.

If all match:

```text
H-ART_FALSIFIED_STATIC
PROCEED only if execution was separately authorized
```

## Semantic guard

Every measured prefill must:
- succeed;
- produce finite logits;
- report 441 dispatches and one submit;
- produce the same first greedy token across all four matched arms.

A semantic or structural mismatch invalidates performance interpretation.

## Current boundary

P3 freezes the causal design only.

No implementation, compilation, model load, GPU dispatch, or timing execution is authorized.
