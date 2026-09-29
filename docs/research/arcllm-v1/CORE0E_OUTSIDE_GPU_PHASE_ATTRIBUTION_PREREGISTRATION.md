# CORE-0E — Outside-GPU Phase Attribution Preregistration

Date: 2026-09-29

Status: **PREREGISTERED_NOT_AUTHORIZED_FOR_MEASURED_EXECUTION**

## Parent result

CORE-0D-R1 closed with:

**PASS_CORE0D_R1_EXCESS_COST_ATTRIBUTION_COMPLETE**

and the frozen Amdahl decision:

**H_OUTSIDE_TOKEN_GPU**

The parent study established only that H, defined as request wall minus observed token-GPU execution, carries the higher robust recoverable current-wall fraction under the preregistered rule. It did **not** identify H as model loading, Vulkan setup, CPU orchestration, allocation, I/O, cleanup, or any other specific mechanism.

CORE-0E therefore asks one narrower question:

> Can H be prospectively decomposed into a closed set of cross-system temporal phases, and does exactly one phase carry enough robust recoverable current-wall excess to deserve the next attribution study?

No optimization is authorized.

## Outcome firewall

CORE-0E was designed from source-level boundary semantics and the final CORE-0D-R1 G/H result.

The parent PHASE_ONLY timing magnitudes are not used to:

- choose the phase partition;
- rank phases;
- set thresholds;
- merge or split phases;
- construct primary CORE-0E evidence.

Old phase evidence may be used later only for structure/schema qualification, never as fresh CORE-0E outcome.

## Why temporal phases

ArcLLM and llama.cpp expose different internal implementation concepts. A label such as "model load" or "Vulkan init" is therefore not automatically cross-system equivalent.

CORE-0E freezes only temporal regions that can be defined on both systems using the same timeline semantics.

The child science window has six ordered markers:

```text
child_science_window_start
        |
        v
prefill_wall_start
        |
        v
prefill_wall_end
        |
        v
decode_wall_start
        |
        v
decode_wall_end
        |
        v
child_science_window_end
```

For ArcLLM, the child window starts at entry to the benchmark-only diagnostic derivative of `generate` before request validation/model mapping and ends after the existing runtime teardown, before phase-sidecar serialization.

For llama.cpp, the child window starts immediately before benchmark-only runtime/backend initialization and ends immediately after context/model/backend teardown, before phase-sidecar serialization.

Ordinary result serialization and phase-sidecar serialization are outside the child science window on both systems.

## Frozen five-phase partition

### P0 — PROCESS_ENVELOPE

```text
P0 = external_child_wall - child_science_window
```

This is only a temporal residual outside the child science window.

It may contain process launch, argument handling, result/sidecar serialization, process exit and parent-observation residual.

It must not be renamed "process-launch cost" or "I/O bottleneck" in CORE-0E.

### P1 — PRE_TOKEN

```text
P1 = prefill_wall_start - child_science_window_start
```

This is all in-window work before prefill begins.

It may contain model/runtime/backend setup and materialization, but CORE-0E does not identify any one of those mechanisms.

### P2 — PREFILL_HOST

```text
P2 = prefill_wall_duration - G_prefill
```

This is the non-G residual inside the prefill wall interval.

It may include host orchestration, full-vocabulary scan or waiting, but it must not be called CPU-only.

### P3 — DECODE_HOST

```text
P3 = decode_wall_duration - G_decode
```

This is the non-G residual inside the complete 31-step decode wall interval.

Again, it is an arithmetic non-G residual, not a predeclared CPU mechanism.

### P4 — POST_TOKEN

```text
P4 = child_science_window_end - decode_wall_end
```

This is all in-window work after the final decode scan through runtime/model/backend teardown.

It must not be renamed cleanup/teardown bottleneck without a successor study.

## Exact closure identities

The implementation must make these identities testable:

```text
child_science_window
  = P1
  + prefill_wall_duration
  + decode_wall_duration
  + P4
```

and:

```text
H_system
  = P0 + P1 + P2 + P3 + P4
```

For a matched ArcLLM/llama pair:

```text
DeltaH
  = DeltaP0 + DeltaP1 + DeltaP2 + DeltaP3 + DeltaP4
```

Negative cross-system phase excess is preserved exactly and never clamped.

## GPU timing remains frozen

The token-GPU definition is unchanged from CORE-0D-R1.

ArcLLM:

```text
G_prefill = prefill Token-XRay device span
G_decode  = sum of 31 decode Token-XRay device spans
```

with the existing 441/469 dispatch geometry, 451 semantic nodes and full timestamp coverage.

llama.cpp:

```text
G_prefill = Vulkan perf block 0
G_decode  = sum of Vulkan perf blocks 1..31
```

using the already qualified scientific-notation parser and exact pinned llama commit.

## Implementation boundary

Only benchmark-only diagnostic derivatives may change.

ArcLLM must combine:

- the frozen CORE-0D-R1 Token-XRay trace contract;
- six monotonic phase markers in the same request.

Canonical ArcLLM runtime, kernels and shaders remain untouched.

llama.cpp source remains untouched. Only the benchmark adapter may add the corresponding monotonic phase markers.

All child markers within one process use one monotonic clock.

CORE-0B remains the product-performance authority. All CORE-0E timing is **DIAGNOSTIC_ONLY**.

## Fresh collection

CORE-0E freezes a smaller, exact 24-request design because only two modes are required:

```text
2 workloads
x 3 blocks
x 2 modes
x 2 systems
= 24 requests
```

Modes:

```text
CONTROL_UNINSTRUMENTED
COMBINED_PHASE_TRACE
```

System order:

```text
block 0: ArcLLM -> llama.cpp
block 1: llama.cpp -> ArcLLM
block 2: ArcLLM -> llama.cpp
```

Mode order:

```text
block 0: CONTROL -> COMBINED
block 1: COMBINED -> CONTROL
block 2: CONTROL -> COMBINED
```

Within every mode, the two systems are adjacent.

No early stop, no selective rerun, no automatic rerun. The first complete valid collection is primary.

## E0 — common trajectory

PASS requires the exact frozen model, prompts, 31 teacher-forced decode inputs, one prefill and 31 cached-decode calls, finite full-vocabulary scans, and no predicted-token feedback.

Failure:

**STOP_CORE0E_COMMON_TRAJECTORY_INVALID**

## E1 — CONTROL transfer

Per workload, all three CONTROL ArcLLM/llama ratios must be greater than one.

The CONTROL median must remain within multiplicative factor **1.5** of the frozen CORE-0B reference:

```text
W-S = 6.91387843464958
W-C = 8.63225451567552
```

and max/min spread must not exceed **2.5**.

Failure:

**STOP_CORE0E_TOTAL_TRANSFER_NOT_QUALIFIED**

## E2 — combined trace transfer

All six COMBINED_PHASE_TRACE pairs must retain positive ArcLLM total excess.

For each workload, the COMBINED median ArcLLM/llama wall ratio must remain within multiplicative factor **1.5** of the corresponding CONTROL median.

No post-hoc instrumentation correction may be fitted.

Failure:

**STOP_CORE0E_COMBINED_TRACE_TRANSFER_NOT_QUALIFIED**

## E3 — phase and GPU qualification

PASS requires:

- all six child markers exist, are finite and strictly ordered;
- child window, P1, prefill wall, decode wall and P4 are non-negative;
- P0 is non-negative;
- ArcLLM keeps exact 1 prefill + 31 decode Token-XRay traces and frozen geometry;
- llama keeps exact 1 prefill + 31 decode Vulkan groups;
- G_prefill and G_decode are positive finite;
- P2 and P3 are non-negative on every request.

Failure:

**STOP_CORE0E_PHASE_MEASUREMENT_NOT_QUALIFIED**

## E4 — phase closure

Tolerance remains **1 microsecond**.

Per request:

```text
child_science_window
= P1 + prefill_wall + decode_wall + P4
```

and:

```text
W - G
= P0 + P1 + P2 + P3 + P4
```

Per matched pair:

```text
DeltaH
= sum(DeltaP0..DeltaP4)
```

Failure:

**STOP_CORE0E_PHASE_CLOSURE_NOT_QUALIFIED**

## E5 — H materiality transfer

CORE-0E may select a subphase only if H remains material in fresh evidence.

For each workload:

```text
f_H = DeltaH / DeltaE
q_H = median(f_H) * (R_CORE0B - 1) / R_CORE0B
```

PASS requires:

- DeltaH > 0 on all six COMBINED pairs;
- median(f_H) > 0 in both workloads;
- q_H >= 0.10 in both workloads;
- at least one workload has q_H >= 0.20.

Failure:

**STOP_CORE0E_H_NOT_MATERIAL_OR_TRANSFERRED**

## Primary phase attribution

For each phase Pk and matched pair:

```text
DeltaPk = Pk_arc - Pk_llama
f_Pk    = DeltaPk / DeltaE
h_Pk    = DeltaPk / DeltaH
```

All three block values are preserved separately for each workload.

Report median/min/max/MAD. Negative values remain negative.

## Phase Amdahl gate

The gate opens only if E0 through E5 PASS.

For each phase:

```text
q_Pk = median(f_Pk) * (R_CORE0B - 1) / R_CORE0B
S_Pk = 1 / (1 - q_Pk)
```

A phase is eligible only if:

- median(f_Pk) > 0 in both workloads;
- q_Pk >= 0.10 in both workloads;
- at least one workload has q_Pk >= 0.20.

Robust score:

```text
score_Pk = min(q_Pk_W-S, q_Pk_W-C)
```

A single phase receives priority only if:

- score >= **0.15**;
- it exceeds the runner-up phase by at least **0.05** absolute current-wall fraction.

These are intentionally the same materiality/winner thresholds used by CORE-0D-R1; CORE-0E does not weaken them because the partition is finer.

Otherwise the mandatory result is:

**NO_SINGLE_OUTSIDE_GPU_PHASE_PRIORITY**

## Successor routing

```text
P0 -> CORE-0F_PROCESS_ENVELOPE_SUBPHASE_ATTRIBUTION_PREREGISTRATION
P1 -> CORE-0F_PRE_TOKEN_LOAD_INIT_ATTRIBUTION_PREREGISTRATION
P2 -> CORE-0F_PREFILL_HOST_ORCHESTRATION_ATTRIBUTION_PREREGISTRATION
P3 -> CORE-0F_DECODE_HOST_ORCHESTRATION_ATTRIBUTION_PREREGISTRATION
P4 -> CORE-0F_POST_TOKEN_TEARDOWN_ATTRIBUTION_PREREGISTRATION

no single winner
   -> CORE-0F_OUTSIDE_GPU_MULTIPHASE_REPLICATION_PREREGISTRATION
```

Even a winning phase authorizes only the next attribution study, not an optimization patch.

## Current authorization

Authorized now:

- benchmark-only phase-marker implementation;
- combined phase + GPU trace diagnostic adapters;
- static preflight;
- zero-science fixtures;
- parent-dataset structure-only validation;
- execution-lock construction.

Not authorized:

- any fresh measured request;
- canonical runtime/kernel/shader changes;
- llama.cpp source changes;
- optimization;
- mechanism selection;
- NPU;
- hardware counters;
- external profiler;
- resource sampler.

Next:

**CORE0E_IMPLEMENTATION_STATIC_PREFLIGHT_AND_EXECUTION_LOCK**
