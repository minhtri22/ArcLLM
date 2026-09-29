# CORE-0C — Final Scientific Adjudication

Date: 2026-09-29

## Verdict

**PASS_CORE0C_DIAGNOSTIC_LOCALIZATION_COMPLETE**

Open findings: **0**.

This PASS means the frozen Token-XRay diagnostic localization completed validly. It is **not** a performance-authority result and does not select an optimization mechanism.

## Collection integrity

Exactly 12 measured requests were executed:

```text
W-S: 3 × (CONTROL, TRACE)
W-C: 3 × (CONTROL, TRACE)

6 controls
6 Token-XRay traces
12 total requests
```

All six TRACE requests preserved the exact frozen ArcLLM output/topology contract.

Each TRACE request produced:

- 1 prefill TOKEN_TRACE with 441 physical dispatches;
- 31 decode TOKEN_TRACE artifacts with 469 physical dispatches each;
- 451 semantic nodes represented per phase;
- 1 RUNTIME_LIFECYCLE_TRACE;
- 14 measured Q4V4 P1 materialization events.

Dispatch timestamp coverage was 100% in all traces.

Dataset:

`results/core0c_measured_20260929T064130264001Z`

Frozen dataset commit:

`73fd1064495578bf2aeb9d90d108df3385c46edc`

Independent adjudication recomputed the frozen trace semantics, observer ratios, physical timing aggregation and lifecycle evidence with **0 findings**.

## Observer effect

The paired external-wall ratios TRACE / CONTROL were:

### W-S

```text
block 0 = 1.330430  (+33.04%)
block 1 = 1.022072  (+2.21%)
block 2 = 1.003531  (+0.35%)

median  = 1.022072
range   = 1.003531 .. 1.330430
```

### W-C

```text
block 0 = 1.099245  (+9.92%)
block 1 = 0.873115  (-12.69%)
block 2 = 0.930542  (-6.95%)

median  = 0.930542
range   = 0.873115 .. 1.099245
```

The negative W-C ratios cannot represent a literal negative instrumentation cost. Combined with the broad W-S spread, the correct interpretation is:

`HOST_NONSTATIONARITY_PREVENTS_SIMPLE_CAUSAL_OVERHEAD_ESTIMATE`

Therefore CORE-0C does **not** subtract a single Token-XRay overhead correction from any timing. CORE-0B remains the sole product-performance authority.

## Physical localization

### W-S

Median TRACE wall time was about **61.35 s**.

Median observed token-execution GPU span plus P1 materialization was about **30.07 s**, roughly **49.0%** of TRACE wall time.

Within that observed GPU time:

- decode: **96.89%**;
- prefill: **3.03%**;
- Q4V4 P1 materialization: **0.084%**.

Largest prefill physical runtime families by median share of dispatch time:

1. `lm_head` — **57.02%**
2. `ffn_gate_up_fused` — **21.49%**
3. `ffn_down` — **15.07%**
4. `o_proj` — **2.36%**
5. `q_proj` — **2.29%**

Largest decode families:

1. `lm_head` — **60.63%**
2. `ffn_down` — **13.64%**
3. `ffn_up` — **7.81%**
4. `ffn_gate` — **7.80%**
5. `q_proj` — **2.63%**

Median P1 materialization time was **25.41 ms**.

### W-C

Median TRACE wall time was about **70.36 s**.

Median observed token-execution GPU span plus P1 materialization was about **38.54 s**, roughly **54.8%** of TRACE wall time.

Within that observed GPU time:

- decode: **72.12%**;
- prefill: **27.82%**;
- Q4V4 P1 materialization: **0.056%**.

Largest prefill families:

1. `ffn_gate_up_fused` — **48.69%**
2. `ffn_down` — **31.14%**
3. `q_proj` — **6.46%**
4. `o_proj` — **6.34%**
5. `lm_head` — **4.12%**

Largest decode families:

1. `lm_head` — **57.16%**
2. `ffn_down` — **14.11%**
3. `ffn_gate` — **8.19%**
4. `ffn_up` — **8.18%**
5. `q_proj` — **2.80%**

Median P1 materialization time was **21.44 ms**.

## Scientific interpretation

Three statements are supported.

First, Q4V4 P1 materialization is not a large share of the observed GPU execution budget in these traces. It is below 0.1% of the observed GPU span in both workloads.

Second, decode GPU work is dominated by `lm_head` under both workloads, with `ffn_down` the next largest single family. Prefill structure differs materially between W-S and W-C.

Third, roughly half of TRACE request wall time is outside the observed token GPU spans. CORE-0C does not identify that remainder as model loading, host orchestration, allocation, file I/O or any other single mechanism; those causes are not isolated by this study.

## Boundaries

CORE-0C does **not** establish:

- a corrected product-performance ratio;
- a causal Token-XRay overhead estimate;
- that `lm_head` alone explains the CORE-0B 7.398× gap;
- that the unobserved request-wall remainder is a specific lifecycle/setup mechanism;
- that FFN-down should or should not be moved to NPU;
- a next optimization mechanism.

CORE-0B remains the authoritative uninstrumented request-level comparison.

## Consequence

CORE-0C is formally closed.

The next scientific step, if opened, should be **CORE-0D excess-cost attribution preregistration**: join the authoritative CORE-0B request gap with CORE-0C phase/runtime-family localization and exact reference-runtime evidence, then test candidate excess-cost explanations under an explicit Amdahl gate before selecting any core intervention.
