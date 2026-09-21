# ANL64 P6 — Matched End-to-End Confirmation Specification

**Date:** 2026-09-21  
**State:** SPECIFICATION ONLY / EXECUTION BLOCKED

## Scientific question

P6 asks:

> Does the semantically validated ANL64 successor produce a reproducible material end-to-end improvement over the exact frozen safe ArcLLM reference on the target Intel Arc 140V system?

This is a successor-intervention confirmation.

It is **not** a claim that ANL64 beats llama.cpp. The external-runtime question remains outside P6.

## Freshness boundary

No P5 timing is admissible.

P5 was a semantic study. Its raw runtime files physically contained timing fields, but those values were preregistered as spent and non-reusable.

P6 therefore requires a completely fresh performance collection.

## Systems

Candidate:

```text
ANL64_P4_LOCKED
runtime blob dbcb7afed5a08e7aff3ca02a1bd95bd985076f70
plan blob    157be15c63363ba2d55093af829ca68be9107e27
exe SHA256   1F45DA9D8CACE3CF78FE31B7B7041B6027CB41E180F7FC99E50E1127EA4F5451
```

Reference:

```text
ARC_SAFE_REFERENCE
historical implementation 43afd71161c4dc8c766c09c3b55d5eca48352bde
runtime blob ea1e986e22f6921e7f6c52a4fa5935121cfec663
```

## Workloads

Exactly the already-frozen pair:
- W-S: 4 prompt tokens + 32 generated tokens;
- W-C: 256 prompt tokens + 32 generated tokens.

No workload search is permitted.

## Counterbalanced fresh design

Two independent runner processes are required.

Session A:

```text
ARC_SAFE_REFERENCE / W-S
ANL64_P4_LOCKED    / W-S
ANL64_P4_LOCKED    / W-C
ARC_SAFE_REFERENCE / W-C
```

Session B:

```text
ANL64_P4_LOCKED    / W-S
ARC_SAFE_REFERENCE / W-S
ARC_SAFE_REFERENCE / W-C
ANL64_P4_LOCKED    / W-C
```

Every cell receives:

```text
1 warmup
5 measured attempts
```

Total fresh measured attempts:

```text
2 sessions × 4 cells × 5 = 40
```

No artifact rebuild is permitted between measured cells/sessions.

## Primary performance metrics

P6 has two co-primary metrics:

1. decode throughput;
2. end-to-end latency.

For each session/workload pair:

```text
decode ratio = median(candidate decode_tps) / median(reference decode_tps)

E2E ratio = median(candidate e2e_ms) / median(reference e2e_ms)
```

Material-benefit thresholds are frozen at 10%:

```text
decode ratio >= 1.10

E2E ratio <= 0.90
```

The 10% magnitude is not chosen from P5 outcomes. It inherits the already-established Q3/P2 materiality convention and is now applied prospectively to the successor-vs-safe-reference causal contrast.

## Blocking-harm guard

Prefill is intentionally unchanged.

Therefore TTFT must not materially regress:

```text
median(candidate TTFT) / median(reference TTFT) <= 1.10
```

in all four session/workload comparisons.

## Semantic/structural guard

Performance is inadmissible unless every fresh measured attempt:
- succeeds;
- has finite logits;
- passes dispatch census;
- preserves exact 32-token candidate/reference greedy-sequence equality at matched attempt index.

Candidate must also report the exact frozen plan:

```text
469 PlanNodes
215 quant-linear nodes
140 Q4_FAST nodes
24,104 Region64
19,936 fixed-Q4 Region64
metadata <= 2 MiB
plan hash 04f3f884c0fc4fcc
```

## PASS rule

P6 passes only if, in **all four** session/workload comparisons:

```text
decode >= 1.10x
AND
E2E <= 0.90x
AND
TTFT <= 1.10x
AND
semantic/structural guard PASS
```

This deliberately requires reproduction across both W-S/W-C and both counterbalanced sessions.

## Failure and stop rules

If any fresh semantic/structural guard fails:

`P6_SEMANTIC_OR_STRUCTURAL_FAIL`.

If semantic validity holds but any required decode/E2E/TTFT threshold fails:

`P6_NO_MATERIAL_E2E_BENEFIT`.

No threshold tuning, workload search, Q6 optimization, fusion, repack, prefetch, quantization change, executor substitution, order change, or repetition change may rescue P6.

At most one infrastructure-only repair is permitted. Any valid repair requires a complete replay from zero; cell/session salvage is forbidden.

## Claim boundary

A P6 PASS would support:

> ANL64 materially improves the exact safe ArcLLM reference end-to-end under the frozen target/workloads.

It would **not** support:

> ANL64 is faster than llama.cpp.

That broader external-runtime comparison was not part of this successor causal program.

## Current authorization

```text
P6 specification            AUTHORIZED
P6 execution                BLOCKED
target model load           BLOCKED
GPU performance execution   BLOCKED
P7                           BLOCKED
```

Next: zero-science specification QA, then a separate explicit P6 execution-authorization gate.
