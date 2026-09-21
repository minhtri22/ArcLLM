# ARCLLM_TTFT_M1 — P1 Prior-Art and Source Review

**Date:** 2026-09-22  
**State:** specification/research only. No implementation or target execution is authorized.

## Purpose

P1 asks only which TTFT mechanisms are already established elsewhere, which source mechanisms actually exist in the local runtime, and which transfers are admissible. It does not choose an intervention from outcome data.

## External prior art

### Prefill and decode are distinct latency regimes

DistServe treats TTFT and decode latency as distinct service objectives and shows that prefill/decode interference matters in multi-request serving. Sarathi-Serve and DeepSpeed-FastGen likewise redesign prompt/prefill scheduling to manage latency-throughput coupling.

Transfer decision:

```text
phase separation / separate TTFT reasoning  -> TRANSFERABLE_INVARIANT
multi-request scheduler policy             -> NOT_TRANSFERABLE
cluster prefill/decode disaggregation       -> NOT_TRANSFERABLE
```

The local parent P6 is a single-request matched experiment, so serving-level contention cannot be imported as an explanation.

### Vulkan resource lifecycle

Khronos Vulkan guidance establishes two relevant engineering facts:

1. frequent allocate/free of command buffers is expensive compared with recycling/resetting pools;
2. compute-pipeline creation may incur compilation cost and pipeline cache can preserve internal representations.

These are source-grounded mechanism families, not local causal conclusions.

Local transfer constraint:
- ArcLLM creates pipelines in `prepare_chain()`, which occurs before measured TTFT;
- therefore pipeline creation time itself cannot explain measured P6 TTFT;
- only driver state/residency/deferred compilation effects that survive into the measured prefill interval remain admissible;
- command-pool/buffer/fence lifecycle is inside measured execution, but the code path is nominally shared between candidate/reference, so it is admissible primarily as an **interaction** with different preceding GPU/runtime state.

### Source implementations

Pinned source inspection adds these constraints:

- llama.cpp Vulkan keeps persistent command-pool/command-buffer structures and explicit pipeline compiled state;
- FlashInfer explicitly separates plan from run for prefill kernels;
- vLLM/Sarathi expose prefill scheduling/chunking as a first-class phase;
- SGLang prefix caching is not applicable to the parent's fresh no-prefix-reuse cells.

No external implementation establishes that any of these mechanisms causes ANL64's local TTFT regression.

## Local source audit

Frozen local sources:

```text
safe reference:
src/q2_benchmark.cpp
ea1e986e22f6921e7f6c52a4fa5935121cfec663

ANL64 candidate:
src/anl64_runtime.cpp
dbcb7afed5a08e7aff3ca02a1bd95bd985076f70

ANL64 plan:
src/anl64_plan.hpp
157be15c63363ba2d55093af829ca68be9107e27

shared Vulkan runtime:
src/p8c_segmented_access_correctness.cpp
8432ca554b36a2167429b640c5ec6779cf2b3e6b
```

### Exact prefill-source equality

A direct source extraction of the complete `build_prefill()` region in safe reference and candidate is exact-equal.

Both construct the same 441-dispatch prefill graph with the same shader family and launch geometry.

The Q4_FAST substitution occurs only in `build_decode()`.

Therefore:

```text
H: Q4_FAST executes during prefill
REJECTED_BY_SOURCE_MAPPING
```

### Measurement boundary

Both systems:

```text
model read / TensorStore
weights + buffers
plan construction (candidate only)
VkRuntime init
prefill graph build
decode graph build
prepare prefill chain
prepare decode chain
--------------------------------  all before t0

t0
execute prepared prefill chain
CPU full-vocabulary top2 scan
t1
TTFT = t1 - t0
```

Therefore:

```text
H: ANL64 planner CPU construction directly inflates measured TTFT
REJECTED_BY_MEASUREMENT_BOUNDARY
```

Pipeline creation in `prepare_chain()` is also outside TTFT.

### Shared measured execution path

`execute_prepared()`:
- creates a transient command pool;
- allocates and records one primary command buffer;
- creates one fence;
- submits once;
- waits for the fence;
- destroys the fence and command pool.

This work is inside TTFT, but it is shared source. It cannot be a sufficient standalone explanation for candidate/reference divergence.

### Remaining causal degrees of freedom

After the static exclusions above, the scientifically admissible mechanism space is much smaller:

1. candidate/reference prefill **artifacts** may differ despite source equality;
2. the distinct prepared decode-pipeline set may alter driver/shader residency or deferred state that changes later identical-prefill execution;
3. the full-inference warmup differs greatly in decode duration/work and may leave different power/DVFS/thermal/cache state before measured TTFT;
4. shared command submission/allocation overhead may interact with that different state;
5. the parent TTFT effect may fail to reproduce after these state variables are controlled.

## P1 conclusion

Prior art does **not** justify changing the prefill algorithm now.

The next valid work is a local, source-constrained TTFT path mapping and preregistered causal decomposition. The study must separate:

```text
artifact identity
decode-pipeline prepared state
warmup/preconditioning state
their interaction
null / no stable mechanism
```

No implementation is authorized by P1.
