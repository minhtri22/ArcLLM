# TTFT_M2 — P9A Fresh Scientific Design Adoption and Revalidation

**Result:** `PASS_M2_P9A_FRESH_SCIENTIFIC_DESIGN_ADOPTION_AND_REVALIDATION`

P9A resolves the exact deficiency found by the first P9 authorization gate: M1 scientific assets were methodological templates, not canonical M2 science.

P9A performed zero-science static revalidation at current HEAD and created fresh M2 canonical contracts.

## Current-source revalidation

Current exact source identities:

```text
q2 safe runtime    ea1e986e22f6921e7f6c52a4fa5935121cfec663
ANL64 runtime      dbcb7afed5a08e7aff3ca02a1bd95bd985076f70
shared Vulkan      8432ca554b36a2167429b640c5ec6779cf2b3e6b
Q4FAST shader      56999d88dc1bef6486e7e1908982f6de4b0f9f6a
diagnostic source  88d97ddb497bfddcec191358f1e21d982c5efccf
```

Static checks PASS:
- q2 and ANL64 `build_prefill` source regions are exact-equal;
- Q4FAST references in candidate prefill: 0;
- Q4FAST references in candidate decode: 5;
- diagnostic source exposes independent `SAFE/Q4FAST` and `PREFILL_ONLY/FULL_INFERENCE` factors;
- arm map is exactly `SP/SF/QP/QF`;
- measured attempts per cell is fixed at 5;
- expected prefill dispatch count is fixed at 441.

No model, GPU or timing execution occurred.

## Fresh M2 canonical design

Canonical manifest:

`config/arcllm_ttft_m2_p9a_scientific_design_manifest_v0.1.json`

blob:

`abce0545cec33367f9b3a82f15d5ffd4fb026f64`

Members:

```text
source revalidation   17759ae80ce0cbbfb4d2dedd88c0aabe8216ac9c
hypotheses             55192815d02637cc6e469794565ba8134ff63ae1
causal design          9d1960ee253e59f1835ea0d320b40e9975039ed3
target environment     5b79f7b42b238471e0d4bb3ee802bab7ffe83185
falsification / STOP   1ed4187934be119611f9fd1f0ce9f8e61bf88116
```

The fresh M2 hypothesis set is:
`H-ART`, `H-DPIPE`, `H-PRECOND`, `H-STATE-INTERACTION`, `H-NULL`.

The prior alias `H-INTERACT` is normalized prospectively to `H-STATE-INTERACTION`.

## Adopted causal design

The 2×2 factors are frozen as:

```text
decode pipeline  SAFE vs Q4FAST
conditioning     PREFILL_ONLY vs FULL_INFERENCE
```

Arms: `SP`, `SF`, `QP`, `QF`.

Workloads:
- W-S = 4 prompt tokens;
- W-C = 256 prompt tokens;
- FULL_INFERENCE conditioning = 32 output tokens.

Fresh design:
- 2 independent sessions;
- separate processes;
- frozen A/B session orders;
- 5 measured attempts per arm/workload/session;
- 80 total planned fresh TTFT observations;
- no parent timing reuse;
- no selective rerun;
- no pooling for gates.

Primary endpoint remains `ttft_ms`; secondary decomposition telemetry cannot override the primary gate.

## Target contract

P9A adopts the historical target identity only as a **required F0 invariant**, not as a claim that the current runtime already matches it.

Required runtime target:

```text
model SHA256  60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463
model bytes   4683074048
CPU           Intel(R) Core(TM) Ultra 7 258V
GPU           Intel(R) Arc(TM) 140V GPU (16GB)
driver        32.0.101.8860
OS build      26200
power scheme  Balanced
AC online     true
```

Any mismatch at F0 invalidates performance interpretation.

## Falsification and STOP

Materiality remains prospectively fixed at 1.10 with strict workload/session replication.

Adjudication order is frozen:

```text
F0 provenance/environment/freshness
F1 H-ART static identity
F2 semantic/structural guard
F3 session/order validity
F4 H-DPIPE
F5 H-PRECOND
F6 H-STATE-INTERACTION
F7 parent-like reproduction or H-NULL
FINAL mechanism classification
```

No post-outcome threshold change, arm removal, workload search, selective pooling, parent-timing reuse or invention of a rescue mechanism is permitted.

## Scientific boundary

P9A authorizes no science.

```text
model load              false
GPU dispatch            false
performance timing      false
fresh TTFT observations 0
mechanism result        NONE
```

The next admissible step is a second explicit P9 authorization review against this newly canonical M2 design:

`M2_P9_EXPLICIT_FRESH_MECHANISM_IDENTIFICATION_AUTHORIZATION_GATE_REVIEW_2`
