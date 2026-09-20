# ArcLLM Q2 Implementation — matched benchmark harness

Date: 2026-09-20
Status: IMPLEMENTATION_CANDIDATE / NO Q2 MEASUREMENTS

## Purpose

Implements the already-frozen Q2 matched characterization contract without opening Q3 and without running any of the 20 measured attempts.

The implementation has four components:
1. ArcLLM variable-prompt benchmark harness.
2. Exact pinned llama.cpp raw-token baseline adapter.
3. 100 ms Windows process/resource sampler.
4. Evidence merger/summarizer that reports descriptive statistics only.

## ArcLLM harness

`src/q2_benchmark.cpp` is derived from the Q1 production path. Production kernels and model architecture are unchanged.

Allowed generalization:
- prefill extent from 4 to runtime-frozen 4 or 256 tokens;
- working buffers sized for max prompt 256;
- generated count from Q1's 5 to Q2's frozen 32;
- benchmark markers/timing/evidence fields.

Unchanged:
- 19 weight arenas / 341 pieces;
- segmented embedding and output;
- all 28 layers;
- GPU-resident FP32 KV capacity 4096;
- production prefill/decode kernels;
- 441 prefill dispatches;
- 469 dispatches per cached decode step;
- full 152064 logits;
- greedy argmax;
- no CPU model-math fallback;
- no teacher forcing.

Each Q2 cell loads the model once, prepares the production graph once, performs one warmup, then five measured attempts with full KV/transient reset between attempts.

Primary timing is measured inside the model process:
- TTFT: prefill start through full-logit greedy token #1;
- decode: 31 cached steps through token #32;
- e2e: prefill start through token #32.

## Baseline adapter

`baseline/q2_llama_adapter.cpp` binds directly to llama.cpp's public library API at:
- release `v0.4.1`;
- commit `391fac16460f15233a7740550d858ac96df3419d`.

It does not call llama tokenizer/chat APIs.

Raw frozen token IDs are passed directly via `llama_batch_get_one` and `llama_decode`.

Frozen baseline configuration:
- `n_gpu_layers=-1`;
- `n_ctx=4096`;
- `n_batch=256`;
- `n_ubatch=256`;
- `n_threads=8`;
- `n_threads_batch=8`;
- K/V cache F32;
- KQV offload enabled;
- greedy sampler;
- no EOS early stop;
- no speculative decode;
- one sequence.

KV state is cleared before every warmup/measured attempt.

Build/API qualification uses the same Vulkan SDK version as the pinned upstream llama.cpp Windows Vulkan CI: `1.4.357.0`. Qualification builds the adapter against the exact source tag/commit but does not load or execute the target model.

## Resource sampler

`tools/q2_resource_sampler.py` launches each benchmark cell and samples at a 100 ms target interval.

Mandatory metrics use Win32 APIs directly:
- `GetProcessMemoryInfo` for working-set/private bytes;
- `GetProcessTimes` for process CPU utilization normalized to total CPU capacity.

`tools/q2_gpu_sampler.ps1` conditionally attempts:
- GPU Engine compute utilization;
- GPU Process Memory dedicated usage;
- GPU Process Memory shared usage.

If those Windows counters are unavailable or invalid, the sampler records the explicit error and does not fabricate GPU measurements.

Sampling is delimited by process-emitted `Q2_ATTEMPT_BEGIN/END` markers, so setup/model load is excluded from measured attempt windows.

## Frozen execution order

`run_q2.ps1` hard-locks:
1. ArcLLM W-S;
2. llama.cpp W-S;
3. llama.cpp W-C;
4. ArcLLM W-C.

Each cell = 1 warmup + 5 measured attempts.

The runner verifies before measurement:
- exact ArcLLM HEAD/critical-file cleanliness;
- exact Q1 evidence archive SHA;
- exact 7B model SHA/size;
- exact CPU/GPU identity and Intel GPU driver 32.0.101.8860;
- AC power when Windows exposes PowerOnline;
- exact baseline qualification release/commit;
- baseline executable SHA against its qualification JSON;
- static package QA;
- pinned ArcLLM shader compile/native build.

## Summarization

`tools/summarize_q2.py` produces per-cell:
- median;
- min;
- max;
- MAD;
- stability/error rate.

It may report ArcLLM/baseline ratios, explicitly labeled descriptive.

It never produces:
- a winner;
- an advantage verdict;
- Q3 authorization.

Q2 official adjudication remains a separate evidence-review step after the full 20-attempt package exists.

## BuildOnly / baseline qualification gate

The implementation lock is not complete until Windows CI passes:
- Q2 static contract;
- ArcLLM 16-shader compile;
- ArcLLM native benchmark build;
- pinned Vulkan SDK setup;
- exact llama.cpp v0.4.1 commit checkout;
- baseline adapter Vulkan build;
- qualification JSON + executable fingerprint.

No target model is downloaded or executed in this CI gate.

## Current authorization

Q2 target measurement remains forbidden while this document is in IMPLEMENTATION_CANDIDATE state.

Q3 remains blocked.
