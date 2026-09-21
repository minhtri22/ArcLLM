# ARCLLM_TTFT_M1 — P2 TTFT Path Mapping and Mechanism Hypotheses

**Date:** 2026-09-22  
**State:** specification only.

## Exact measured boundary

Parent P6 TTFT is inherited only as a historical observation. The source definition is nevertheless authoritative for mapping what the metric contains.

For both the safe reference and ANL64 candidate:

```text
OUTSIDE TTFT
-----------
GGUF read / TensorStore
weight arena creation
working/KV buffer creation
ANL64 plan construction (candidate only)
VkRuntime init
build_prefill()
build_decode()
prepare_chain(prefill)
prepare_chain(decode)
reset_execution()
-----------
t0

INSIDE TTFT
-----------
execute_prepared(prefill)
  create transient VkCommandPool
  allocate primary VkCommandBuffer
  begin command buffer
  record 441 dispatches
  record inter-dispatch barriers
  final compute->host barrier
  end command buffer
  create VkFence
  vkQueueSubmit
  vkWaitForFences
  destroy fence
  destroy command pool
CPU q2_top2 over full vocabulary logits
-----------
t1
TTFT = t1 - t0
```

This mapping rules out mechanisms whose cost is wholly before `t0`.

## Local equivalence and divergence

### Prefill construction

The extracted `build_prefill()` source region in:
- `src/q2_benchmark.cpp` blob `ea1e986e22f6921e7f6c52a4fa5935121cfec663`;
- `src/anl64_runtime.cpp` blob `dbcb7afed5a08e7aff3ca02a1bd95bd985076f70`;

is exact-equal.

Both produce the same 441-dispatch prefill graph from the same source shader names and launch geometry.

### Decode construction

The candidate substitutes `anl64_q4_fast.spv` for Q/K/O/gate/up decode projections. The safe reference uses `p7_q4k_gemm_2d.spv`.

Therefore the important pre-`t0` state difference is not the prefill graph; it is the **prepared decode pipeline set** and the different work performed by the prior full-inference warmup.

## Rejected explanations before science

### R-P1 — planner CPU time

`anl64_build_plan()` executes before `t0`.

Verdict:

```text
REJECTED_BY_MEASUREMENT_BOUNDARY
```

It may affect process startup, but not the measured P6 TTFT metric.

### R-P2 — Q4_FAST executes in prefill

The prefill source is exact-equal and does not dispatch `anl64_q4_fast.spv`.

Verdict:

```text
REJECTED_BY_SOURCE_MAPPING
```

### R-P3 — serving scheduler interference

The P6 cells are isolated single-system runs rather than concurrent multi-request serving.

Verdict:

```text
NOT_TRANSFERABLE_TO_PARENT_MEASUREMENT
```

## Admissible hypotheses

### H-ART — prefill artifact divergence

Although source is equal, exact binary identity for every common prefill SPIR-V artifact was not independently bound in P6.

Hypothesis:

> Candidate/reference common prefill SPIR-V artifacts differ, causing different execution despite source equality.

First test is static SHA256 identity only. No timing is admissible until this is resolved.

Falsifier:
- every common prefill SPIR-V artifact is byte-identical.

### H-DPIPE — prepared decode-pipeline state interaction

Both binaries prepare the decode chain before TTFT. Candidate preparation includes a different Q4_FAST pipeline family.

Hypothesis:

> Merely preparing the Q4_FAST decode pipeline set changes Vulkan driver/pipeline/shader residency state enough to slow a later identical prefill execution.

This is not pipeline-creation time itself; that creation is outside `t0`. The hypothesis concerns state carried across the boundary.

### H-PRECOND — full-inference warmup preconditioning

P6 uses one full-inference warmup per cell. The candidate and safe reference have very different decode work/duration.

Hypothesis:

> The prior full-inference warmup leaves different GPU/driver/power/thermal/cache state, changing the latency of the next identical prefill.

This is a causal preconditioning hypothesis, not a claim that warmup is scientifically improper.

### H-INTERACT — decode-state × command lifecycle interaction

The measured `execute_prepared()` path allocates/records/submits/waits/destroys a transient command pool/buffer/fence for every attempt. That shared path alone cannot explain divergence.

Hypothesis:

> The shared command-record/submit/wait lifecycle amplifies a difference created by decode-pipeline state or warmup preconditioning.

It is interaction-only: no standalone command-lifecycle causal claim is allowed unless a new design creates an explicit intervention.

### H-NULL — no stable local TTFT mechanism

Hypothesis:

> After artifact identity and preconditioning are controlled, the historical parent TTFT regression does not reproduce as a stable material effect.

This is a valid terminal scientific outcome.

## Hypothesis priority is causal, not convenience

The ordering is fixed by ability to eliminate confounding with minimal scientific intervention:

```text
H-ART static identity
    ↓
H-DPIPE / H-PRECOND 2×2 causal decomposition
    ↓
H-INTERACT only if identified by decomposition
    ↓
H-NULL if no stable mechanism survives
```

No hypothesis may be skipped because another is easier to implement.

## P2 decision

The TTFT mechanism space is sufficiently constrained for a finite causal design.

No architecture mutation, model execution, GPU dispatch, or timing collection is authorized by P2.
