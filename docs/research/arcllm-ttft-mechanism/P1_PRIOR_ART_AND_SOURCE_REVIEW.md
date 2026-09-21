# ARCLLM_TTFT_M1 — P1 Prior-Art + Source Review

Date: 2026-09-22

## External prior art

### Sarathi / Sarathi-Serve

Paper:
- SARATHI: arXiv:2308.16369
- Sarathi-Serve: arXiv:2403.02310
- source repository: microsoft/sarathi-serve
- pinned main commit: `96f9911790ecc00af12ee9fae47cb8fa9ba0d199`
- README blob observed during review: `ddaf3b2211d0798b0269ad5966e2d1909f4e879f`

Transfer classification: `TRANSFERABLE_INVARIANT` for **prefill/decode phase separation** and phase-aware latency reasoning.

Not transferable as a direct intervention: ANL64 is a single-request, single-process offline runtime; it does not have co-located request scheduling, batching, or serving admission. Chunked-prefill scheduling therefore does not identify the local TTFT regression mechanism.

### vLLM

Repository:
- `vllm-project/vllm`
- pinned commit: `54020c3c3ec9de219929a5a956010a0251b87520`
- relevant source: `vllm/config/scheduler.py`

The source explicitly represents chunked prefill and asynchronous scheduling as latency/throughput controls.

Transfer classification: `SOURCE_VERIFIED / NOT_DIRECTLY_TRANSFERABLE`.

Useful invariant: TTFT is a prefill-side metric and phase-local effects must be separated from decode-side effects.

### Vulkan pipeline/runtime guidance

Khronos Vulkan documentation establishes:
- compute-pipeline creation may be costly;
- pipeline caches can reuse baked state;
- first inferences may be slower due to driver/shader/allocation warmup;
- accurate profiling should separate CPU submission time from GPU timestamps;
- stable benchmarking benefits from warmup and controlled clocks.

Transfer classification:
- pipeline-creation cost: `NOT_CAUSAL_FOR_MEASURED_TTFT_UNDER_CURRENT_TIMER_BOUNDARY`;
- warmup/device-state sensitivity: `TRANSFERABLE_INVARIANT`;
- GPU timestamp decomposition: `LOCAL_COMPATIBILITY_REQUIRED`.

### FlashInfer source practice

Repository:
- `flashinfer-ai/flashinfer`
- pinned commit: `d7a7447cb4bb29b4637f7fa02fe5eaeeb5e61a6e`
- relevant source example: `benchmarks/bench_grouped_fp8.py`

The benchmark alternates backend order specifically to reduce systematic bias from warmup / clock ramping.

Transfer classification: `SOURCE_VERIFIED / TRANSFERABLE_MEASUREMENT_INVARIANT`.

## Local source review

Frozen sources:
- ANL64 runtime blob: `dbcb7afed5a08e7aff3ca02a1bd95bd985076f70`
- safe Q2 runtime blob: `ea1e986e22f6921e7f6c52a4fa5935121cfec663`
- Vulkan runtime source blob: `8432ca554b36a2167429b640c5ec6779cf2b3e6b`

### Timer boundary

Both runtimes start TTFT timing immediately before:

`vk.execute_prepared(ppchain, ppops, false)`

and stop after prefill completion plus CPU top-2 extraction.

Therefore the measured TTFT excludes:
- model loading;
- arena creation;
- transient-buffer reset;
- `prepare_chain`;
- `vkCreateComputePipelines`;
- descriptor creation;
- shader-module creation.

### Prefill code identity

ANL64 and the safe reference use the same prefill operator graph, same prefill shaders, same `execute_prepared` function, and same expected prefill dispatch census.

The ANL64 intervention is in decode:
- Q/K/O/gate/up decode projections use `anl64_q4_fast.spv`;
- prefill continues using the historical safe shaders.

Therefore a direct "ANL64 changed prefill math" explanation is source-inconsistent.

### Warmup ordering

Before the five measured attempts, each process performs one complete warmup attempt:

`prefill -> 31 decode steps`.

Thus the candidate measured TTFT occurs after a warmup containing the new Q4_FAST decode kernels, while reference measured TTFT occurs after a warmup containing the safe decode kernels.

This creates a causal path from **decode intervention -> post-warmup GPU/driver/cache/clock state -> next prefill TTFT** despite identical prefill source.

### Runtime submission path

`execute_prepared` creates a transient command pool, allocates/records a command buffer, submits one queue operation, waits on a fence, destroys the fence and command pool.

The API sequence is identical for candidate and reference, but its latency can depend on device/driver state inherited from prior work.

## P1 conclusions

Retained mechanism families:

1. `M-WARM` — decode-warmup carryover alters device/cache/clock state before measured prefill.
2. `M-ORDER` — process/order/DVFS state creates systematic candidate/reference TTFT bias.
3. `M-RES` — the prepared decode-chain resource/pipeline footprint indirectly perturbs later prefill execution.
4. `M-SUBMIT` — identical host submission/fence path experiences state-dependent latency after different warmups.
5. `M-PREFILL-GPU` — GPU-side prefill duration itself changes despite identical command semantics because inherited device state differs.

Rejected as primary measured-TTFT mechanisms:
- model load;
- CPU buffer reset;
- pipeline compilation inside the TTFT interval;
- direct prefill graph/shader differences;
- serving-level chunked-prefill scheduler interference.

No intervention is selected in P1.
