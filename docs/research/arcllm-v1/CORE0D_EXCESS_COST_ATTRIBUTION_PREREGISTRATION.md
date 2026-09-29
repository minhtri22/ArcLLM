# CORE-0D — Excess-Cost Attribution Preregistration

Date: 2026-09-29

Parent results:
- CORE-0B: PASS_CORE0B_MATCHED_REQUEST_CHARACTERIZATION_COMPLETE
- CORE-0C: PASS_CORE0C_DIAGNOSTIC_LOCALIZATION_COMPLETE

Status: PREREGISTERED_NOT_AUTHORIZED_FOR_MEASURED_EXECUTION

## Scientific question

Under one frozen common teacher-forced token trajectory, can the current ArcLLM-vs-llama.cpp request excess be decomposed prospectively into two independent regions:

1. G — token-GPU execution excess
2. H — outside-token-GPU residual excess

with enough transfer back to CORE-0B to support an Amdahl-gated choice of which region deserves the next core study?

CORE-0D does not modify CORE-0B's performance result and does not authorize optimization.

## Why teacher-forced decode is mandatory

CORE-0B used the same prompt and the same 32-token request length on both systems, but the two systems did not produce the same natural autoregressive token sequence. That remains valid for request-level performance characterization, but it is not strong enough for exact internal decode-cost attribution.

CORE-0D therefore freezes 31 common cached-decode input tokens per workload. Both systems still compute full logits and run the full-vocabulary finite/top-1 scan at every step, but the predicted token is recorded only; it is never fed back into the next decode step.

Required shape on both systems:

    same frozen prompt
      -> prefill
      -> full logits/top-1 scan
      -> frozen decode input #0
      -> cached decode
      -> full logits/top-1 scan
      -> frozen decode input #1
      -> ...
      -> 31 common decode inputs total

Predicted-token equality is not a gate. Forced-input identity and finite logits are gates.

### W-S forced continuation

136406, 144325, 181, 8100, 16019, 23938, 31857, 39776, 47695, 55614, 63533, 71452, 79371, 87290, 95209, 103128, 111047, 118966, 126885, 134804, 142723, 150642, 6498, 14417, 22336, 30255, 38174, 46093, 54012, 61931, 69850

### W-C forced continuation

3112, 11031, 18950, 26869, 34788, 42707, 50626, 58545, 66464, 74383, 82302, 90221, 98140, 106059, 113978, 121897, 129816, 137735, 145654, 1510, 9429, 17348, 25267, 33186, 41105, 49024, 56943, 64862, 72781, 80700, 88619

## Authority hierarchy

CORE-0B remains the sole product-performance authority.

- CORE-0B uninstrumented child wall: BENCHMARK_AUTHORITY
- CORE-0D CONTROL: attribution transfer check only
- CORE-0D PHASE_ONLY: DIAGNOSTIC_ONLY
- CORE-0D GPU_TRACE: DIAGNOSTIC_ONLY

No CORE-0D result may replace, average with, or retroactively correct CORE-0B.

## Exact llama.cpp measurement surfaces

Baseline remains pinned to ggml-org/llama.cpp v0.4.1 at b29c606e28a01b1bc8c1351026a0fa6e616bf6c4 with Vulkan and 29/29 layers offloaded.

The exact commit already exposes llama_perf_context_data with t_load_ms, t_p_eval_ms, t_eval_ms, n_p_eval, n_eval and n_reused. These are secondary phase evidence only.

The exact Vulkan backend also already contains GGML_VK_PERF_LOGGER. CORE-0D freezes:

- GGML_VK_PERF_LOGGER enabled
- GGML_VK_PERF_LOGGER_CONCURRENT unset
- GGML_VK_PERF_LOGGER_FREQUENCY=1

The logger uses Vulkan query-pool timestamps but also forces command-buffer end/submit/wait, so its timing is diagnostic-only.

## Primary attribution regions

### G — TOKEN_GPU

G_system is token-model GPU execution measured by GPU timestamps for exactly one prefill graph plus 31 cached-decode graphs.

ArcLLM uses the frozen CORE-0C Token-XRay Vulkan timestamp contract.
llama.cpp uses the exact pinned built-in Vulkan perf logger after cardinality qualification.

G excludes process launch, model load, runtime/context construction, ArcLLM Q4V4 P1 materialization, serialization, process exit and all unobserved host work.

### H — OUTSIDE_TOKEN_GPU

For the same TRACE request:

    H_system = external_child_wall - G_system

H is an arithmetic residual only. It must not automatically be renamed model-load, setup, CPU orchestration, allocation, I/O or any other specific mechanism.

## Secondary phase evidence

PHASE_ONLY mode may subdivide H descriptively.

llama.cpp: external wall plus the existing load/prompt-eval/eval timing surface.

ArcLLM: benchmark-only monotonic host timestamps around qualified runtime/backend init, model/GGUF load, context/buffer/pipeline preparation, prefill call, 31-step decode loop and serialization/finalization.

Implementation preflight must freeze the exact nesting. Any boundary that cannot be observed without changing canonical execution semantics is marked unavailable, never inferred.

## Prospective collection

For each workload: 3 blocks × 3 modes × 2 systems = 18 requests.
Across W-S and W-C: exactly 36 measured requests.

Modes:
- CONTROL_UNINSTRUMENTED
- PHASE_ONLY
- GPU_TRACE

System order:
- block 0: ArcLLM -> llama.cpp
- block 1: llama.cpp -> ArcLLM
- block 2: ArcLLM -> llama.cpp

Mode order:
- block 0: CONTROL -> PHASE_ONLY -> GPU_TRACE
- block 1: GPU_TRACE -> CONTROL -> PHASE_ONLY
- block 2: PHASE_ONLY -> GPU_TRACE -> CONTROL

Within each mode the two systems are always adjacent. No early stop, no selective rerun, no automatic rerun. First complete valid collection is primary.

## G0 — common trajectory validity

PASS requires exact model, prompt and forced-token identities; exactly 1 prefill + 31 cached decode calls on both systems; finite logits scans; and no predicted token fed back.

Failure class: STOP_CORE0D_COMMON_TRAJECTORY_INVALID

## G1 — CONTROL transfer to CORE-0B

Frozen CORE-0B workload pooled references:
- W-S median Arc/llama = 6.91387843464958
- W-C median Arc/llama = 8.63225451567552

For each workload, the three new CONTROL ratios must satisfy:
1. all three ratios > 1
2. median within multiplicative factor 1.5 of the corresponding CORE-0B pooled median
3. max_ratio / min_ratio <= 2.5

Failure class: STOP_CORE0D_TOTAL_TRANSFER_NOT_QUALIFIED

## G2 — TRACE transfer

For each workload:
1. all three TRACE matched pairs retain positive ArcLLM total excess
2. TRACE median Arc/llama wall ratio stays within multiplicative factor 1.5 of the corresponding CONTROL median

Failure class: STOP_CORE0D_TRACE_TRANSFER_NOT_QUALIFIED

No post-hoc trace-overhead correction is fitted.

## G3 — GPU measurement qualification

ArcLLM TRACE must preserve 1 prefill Token-XRay trace, 31 decode traces, 441/469 physical dispatch geometry, 451 semantic nodes and 100% physical timestamp coverage.

llama TRACE must expose exactly 1 prefill Vulkan timing group and 31 decode timing groups. Every group must be non-empty with positive finite total op time.

Failure class: STOP_CORE0D_GPU_MEASUREMENT_NOT_QUALIFIED

## G4 — residual closure

For each TRACE request:

    H_arc   = W_arc   - G_arc
    H_llama = W_llama - G_llama

For each adjacent TRACE pair:

    DeltaE = W_arc - W_llama
    DeltaG = G_arc - G_llama
    DeltaH = H_arc - H_llama
    DeltaE = DeltaG + DeltaH

Arithmetic closure tolerance is 1 microsecond after unit conversion.

Every G and H must be non-negative, and every DeltaE must remain positive. Cross-system DeltaG or DeltaH may be negative and must be preserved, never clamped.

Failure class: STOP_CORE0D_RESIDUAL_CLOSURE_NOT_QUALIFIED

## Primary excess attribution

Per block:

    f_G = DeltaG / DeltaE
    f_H = DeltaH / DeltaE = 1 - f_G

Preserve all block values. Per workload report median, min, max and MAD of DeltaE, DeltaG, DeltaH, f_G and f_H. Do not merge W-S and W-C before workload-specific adjudication.

## Amdahl gate

Amdahl selection is forbidden unless G0 through G4 all PASS.

For workload-authoritative CORE-0B ratio R:

    q_X = median(f_X) * (R - 1) / R
    S_X = 1 / (1 - q_X)

q_X is the estimated fraction of current ArcLLM request wall recoverable under the idealized upper bound where the excess attributed to region X is reduced to the pinned llama reference.

A region is eligible only if:
- median(f_X) > 0 in both workloads
- q_X >= 0.10 in both workloads
- at least one workload has q_X >= 0.20

Robust score:

    score_X = min(q_X_W-S, q_X_W-C)

A single region receives priority only if:
- winner score >= 0.15
- winner score exceeds the other region by at least 0.05 absolute current-wall fraction

Otherwise the mandatory decision is NO_SINGLE_REGION_PRIORITY.

Successor routing:
- G wins -> CORE-0E_GPU_EXCESS_SUBFAMILY_ATTRIBUTION_PREREGISTRATION
- H wins -> CORE-0E_OUTSIDE_GPU_PHASE_ATTRIBUTION_PREREGISTRATION
- no material winner -> CORE-0E_SPLIT_REGION_BOUNDARY_REPLICATION_PREREGISTRATION

Even a winner authorizes only the next attribution study, not an optimization patch.

## Semantic mapping boundary

CORE-0D does not map llama operation names to ArcLLM runtime families for the primary G/H result. If G wins, a successor must prospectively qualify cross-system op-family semantics before attributing excess to lm_head, FFN, attention or any individual kernel family.

## Current authorization

Authorized now:
- benchmark-only teacher-forced adapters
- ArcLLM benchmark-only phase instrumentation
- parser/qualification of llama's existing Vulkan perf logger
- static QA and zero-science fixtures
- execution-lock construction

Not authorized:
- any of the 36 measured requests
- canonical product runtime/kernel/shader changes
- hardware counters
- external profiler
- resource sampler
- NPU
- optimization or mechanism selection

Next: CORE0D_IMPLEMENTATION_STATIC_PREFLIGHT_AND_EXECUTION_LOCK
