# ANL64 P6 — Matched End-to-End Confirmation Specification

**Date:** 2026-09-21  
**State:** SPECIFICATION ONLY / EXECUTION BLOCKED

## Question

P6 asks the terminal performance question for the frozen successor architecture:

> Does the P5-validated ANL64 architecture produce a practically material and reproducible decode **and** end-to-end improvement over the exact safe ArcLLM reference on both frozen workloads, without material TTFT or working-set harm?

P6 uses **fresh measurements only**. Timing emitted during P5 is permanently inadmissible.

## Systems

Reference:

```text
ARC_SAFE_REFERENCE
runtime blob:
ea1e986e22f6921e7f6c52a4fa5935121cfec663

historical exact implementation:
43afd71161c4dc8c766c09c3b55d5eca48352bde
```

Candidate:

```text
ANL64_P4_LOCKED
runtime:
dbcb7afed5a08e7aff3ca02a1bd95bd985076f70

plan:
157be15c63363ba2d55093af829ca68be9107e27

Q4_FAST nodes:
140

plan hash:
04f3f884c0fc4fcc
```

No production mutation is permitted.

## Why the P6 primary metrics are decode + E2E

The intervention changes only selected Q4 decode executors.

It does not redesign:
- prefill;
- model residency;
- Q6;
- attention;
- LM head.

Therefore P6 does not require TTFT or memory to improve. They are **harm guards**.

The intended causal effect must appear in both:

```text
decode throughput
AND
end-to-end latency
```

This prevents a verdict based on an isolated micro-effect that does not survive to the user-visible inference boundary.

## Fresh counterbalanced reproduction

Two independent PowerShell invocations are required.

Session A:

```text
ANL64 / W-S
reference / W-S
reference / W-C
ANL64 / W-C
```

Session B:

```text
reference / W-S
ANL64 / W-S
ANL64 / W-C
reference / W-C
```

Each cell runs:

```text
1 warmup
5 measured attempts
```

Therefore:

```text
20 measured attempts / session
40 fresh measured attempts total
```

No session pooling is permitted.

## Frozen thresholds

P6 reuses the already-established historical practical thresholds instead of inventing post-P5 cutoffs.

For every workload in every session:

### Primary — both required

```text
decode_tps_ANL64 / decode_tps_reference >= 1.10

E2E_ms_ANL64 / E2E_ms_reference <= 0.90
```

### Blocking-harm guards — both required

```text
TTFT_ms_ANL64 / TTFT_ms_reference <= 1.10

working_set_peak_ANL64 /
working_set_peak_reference <= 1.10
```

To obtain a positive P6 result, **all four conditions must pass for W-S and W-C independently in both A and B**.

## Correctness guard

Performance evidence is inadmissible unless every measured attempt:
- succeeds;
- has finite logits;
- passes dispatch census;
- emits exactly 32 tokens;
- reproduces the frozen P5 workload hash.

Frozen semantic hashes:

```text
W-S  f31d4bb9fe5eb9c3
W-C  471519ddc45b232e
```

ANL64 must additionally report:

```text
nodes              469
quant nodes        215
Q4_FAST nodes      140
regions         24,104
fixed regions   19,936
plan hash 04f3f884c0fc4fcc
metadata <= 2 MiB
```

## F0 freshness/environment

Both sessions must use:
- exact model SHA256 `60E05F21...BFC2463`;
- Intel Core Ultra 7 258V;
- Intel Arc 140V;
- driver `32.0.101.8860`;
- Windows build 26200;
- Balanced power scheme;
- AC online.

The sessions must be separate runner processes and preserve the frozen counterbalanced orders.

No artifact may be rebuilt between cells inside a session.

## Outcomes

```text
P6_REPRODUCED_MATERIAL_E2E_GAIN
P6_NO_MATERIAL_E2E_GAIN
P6_BLOCKING_HARM
P6_INVALID_F0
P6_STOP_INFRASTRUCTURE_UNSTABLE
```

### Positive result

Requires:
- valid F0;
- correctness/stability PASS;
- decode >= 1.10× reference everywhere;
- E2E <= 0.90× reference everywhere;
- TTFT <= 1.10× reference everywhere;
- working set <= 1.10× reference everywhere.

### Valid negative

If harm guards pass but either primary threshold fails anywhere:

```text
P6_NO_MATERIAL_E2E_GAIN
```

This is a valid scientific result and does not trigger tuning.

### Blocking harm

If TTFT or working set exceeds the 10% harm guard anywhere:

```text
P6_BLOCKING_HARM
```

## No rescue

After valid P6 evidence there is no:
- threshold revision;
- workload search;
- Q6 optimization;
- fusion redesign;
- alternate Q4_FAST;
- session pooling;
- P5 timing reuse.

P6 is the fresh matched performance test of this frozen architecture.

## Current authorization

```text
P6 specification     AUTHORIZED
P6 execution         BLOCKED
model load           BLOCKED
GPU dispatch         BLOCKED
performance measure  BLOCKED
P7                   BLOCKED
```

Next: zero-science P6 specification QA, then a separate explicit execution authorization gate.
