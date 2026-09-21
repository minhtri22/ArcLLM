# ANL64 P4 — Bounded Implementation

**Date:** 2026-09-21  
**State:** IMPLEMENTED / STATIC QA PASS / TARGET-LOCAL BUILDONLY PENDING

## Scope implemented

P4 uses an isolated ANL64 runtime rather than modifying the closed Q2 runtime.

New production files:

```text
src/anl64_plan.hpp
src/anl64_runtime.cpp
```

The historical files remain immutable:

```text
src/q2_benchmark.cpp
shaders/p7_q4k_gemm_2d.comp
shaders/sa1_q4k_subgroup_splitk.comp
```

## Ledger64 control plane

`anl64_plan.hpp` implements the frozen `STATIC_ROW_REGION64_PLAN`.

It builds at model load and fail-closes unless the exact census is:

```text
PlanNodes                 469
quant-linear PlanNodes    215
Q4_FAST_FIXED PlanNodes   140
Region64 descriptors    24,104
fixed-Q4 Region64       19,936
metadata hard limit      2 MiB
```

The plan receives the actual V/down Q4/Q6 tensor types but does not promote them to Q4_FAST in P4.

The plan is hashed with FNV-1a over the immutable descriptor material.

## Data-plane substitution

Only five decode roles inside each of 28 layers use the inherited Q4_FAST shader:

```text
q_proj
k_proj
o_proj
ffn_gate
ffn_up
```

Total admitted nodes:

```text
5 × 28 = 140
```

The inherited shader processes four output rows per workgroup, therefore the new dispatch dimensions are:

```text
Q/O    3584 / 4 = 896
K       512 / 4 = 128
gate/up 18944/4 = 4736
```

Every Region64 maps to exactly 16 Q4_FAST workgroups.

## Explicitly unchanged

Prefill contains zero Q4_FAST references.

V/down remain on the existing Q4/Q6 safe decode paths.

Embedding, RoPE, attention/KV, residual, SwiGLU, norm and LM head remain unchanged.

No fusion redesign, Q6 optimization, weight repack, predequant cache, prefetch or online tuning was introduced.

## Build isolation

P4 has separate tools:

```text
tools/compile_anl64_p4_shaders.ps1
tools/build_anl64_p4.ps1
tests/test_anl64_p4_package.py
```

The shader build compiles the original 16 Q2 shader sources plus the exact inherited SA1 Q4 shader into:

```text
anl64_q4_fast.spv
```

The native build produces:

```text
artifacts/ANL64/P4/anl64_p4.exe
```

The BuildOnly tool records that the executable is not launched, no model is loaded, no GPU dispatch occurs, and no performance measurement occurs.

## Static QA

Connector-equivalent static audit verifies:

- closed legacy blobs unchanged;
- exactly five Q4_FAST decode source sites;
- zero Q4_FAST prefill sites;
- exact allowed roles only;
- V/down remain safe;
- exact planner census and budget checks;
- exact inherited Q4_FAST shader source reused unchanged;
- BuildOnly path cannot launch the executable.

Result:

```text
PASS_CONNECTOR_EQUIVALENT_STATIC_QA_BUILDONLY_PENDING
```

P4 is not complete until the committed Windows BuildOnly runner returns a valid bundle.
