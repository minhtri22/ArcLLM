# ArcLLM v1 — I003-MEB Matched External Baseline Gap Specification

**Date:** 2026-09-24  
**Program:** `I003-MEB`  
**Parent:** closed I002 `PASS_I002_REAL_MODEL_CARRY_THROUGH`  
**Status:** FROZEN SPECIFICATION / ZERO-SCIENCE IMPLEMENTATION AUTHORIZED / FRESH COMPARISON NOT YET AUTHORIZED

## 1. Scientific question

After closing I002, what is the fresh practical performance/resource gap between the closed ArcLLM-v1 I002 candidate and the pinned mature llama.cpp baseline under the same exact model and workloads on the same shared DEV_HOST?

This is a matched characterization study. It does not optimize another kernel and does not presuppose that ArcLLM must beat llama.cpp.

## 2. Systems

### A — closed ArcLLM-v1 I002 candidate

Exact architecture:
- exact 7B Q2-safe production path;
- unchanged prefill;
- unchanged 469 semantic decode nodes;
- exactly 56 Q4_K `ffn_gate` / `ffn_up` decode substitutions;
- candidate shader `sa1_q4k_subgroup_splitk.spv`;
- source blob `56999d88dc1bef6486e7e1908982f6de4b0f9f6a`;
- historical/closed SPIR-V SHA256 `B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569`;
- no further kernel/runtime tuning.

### B — pinned external baseline

Exact Q2 baseline identity is inherited:
- repository: `ggml-org/llama.cpp`;
- release: `v0.4.1`;
- commit: `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`;
- Vulkan backend;
- n_ctx=4096;
- n_batch=256;
- n_ubatch=256;
- n_threads=8;
- n_threads_batch=8;
- K/V cache F32;
- all layers requested for GPU offload;
- raw token IDs;
- greedy argmax;
- no speculative decode;
- no EOS early stop.

No newer or alternative baseline may replace it after fresh data exposure.

## 3. Model

Both systems load the exact same GGUF bytes:

```text
SHA256 60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463
bytes  4,683,074,048
```

No conversion or requantization.

## 4. Workloads

Exact Q2 workloads are inherited.

### W-S
Prompt token IDs:

`[1,133151,133152,152062]`

Prompt length 4.

### W-C
Prompt length 256.

Positions 0..3 are the same four IDs above. For i=4..255:

`token[i] = 1 + ((104729 + 7919*i) mod 152063)`

Both workloads generate exactly 32 tokens:
- token #1 from prefill;
- 31 cached decode steps;
- no EOS early termination.

## 5. Why paired-attempt design

Q2 used system-level cell order reversal. I003 runs on a shared DEV_HOST where ambient load may vary during a long study.

I003 therefore pairs systems temporally.

Every arm invocation:
1. loads its model/runtime;
2. performs exactly one full warmup inference;
3. resets state;
4. performs exactly one measured inference.

Model load/setup and warmup are excluded from primary timing, matching Q2 timing semantics.

## 6. Frozen cells and pair order

Four cells:

```text
A / W-S
A / W-C
B / W-C
B / W-S
```

Each cell has 5 adjacent pairs.

Session A:
- pair 0 candidate → llama
- pair 1 llama → candidate
- alternate thereafter.

Session B reverses parity:
- pair 0 llama → candidate
- pair 1 candidate → llama
- alternate thereafter.

Total fresh measured inferences:

`4 cells × 5 pairs × 2 systems = 40`

No selective pair/cell rerun.

## 7. DEV_HOST governance

Ambient CPU/RAM/GPU/process load is metadata, not an eligibility blocker.

For every arm invocation record, when available:
- CPU load;
- total/free physical memory;
- top processes by working set;
- active power scheme;
- AC state.

Operational failure permits a later complete-collection rerun under the exact same frozen payload.

Rules:
- every attempt/collection is append-only;
- first complete valid collection is primary;
- later complete collections are replication/robustness only;
- no candidate/baseline/threshold/workload mutation;
- no deleting an unfavorable complete collection;
- no selective cell or pair rerun.

## 8. Timing endpoints

Definitions are inherited from Q2.

### TTFT
Prompt-prefill start → availability of full-vocabulary logits and greedy token #1.

### Decode latency
Time for generated tokens #2..#32, 31 cached decode steps.

### Decode throughput
`31 / decode_elapsed_seconds`

### E2E
Same prefill start → availability of generated token #32.

## 9. Resource endpoints

Per measured arm:
- peak working set;
- peak private bytes;
- mean/peak CPU;
- conditional GPU counters where available.

GPU counter unreliability does not invalidate timing comparison.

## 10. Output semantics

Each arm must:
- succeed;
- produce finite final logits;
- produce exactly 32 generated token IDs.

Cross-system token identity is **not required**, matching Q2. Both systems receive the exact same raw prompt IDs/model/greedy rule, but numerical implementation differences may change autoregressive continuation.

Generated token IDs/hashes remain recorded.

## 11. Primary matched statistics

For every successful pair compute:

```text
TTFT latency ratio   = candidate_ttft / llama_ttft
decode latency ratio = candidate_decode_ms / llama_decode_ms
E2E latency ratio    = candidate_e2e / llama_e2e

decode throughput ratio = candidate_decode_tps / llama_decode_tps

working-set ratio = candidate_peak_ws / llama_peak_ws
private-bytes ratio = candidate_peak_private / llama_peak_private
```

Per cell report:
- paired median;
- min;
- max;
- MAD.

Across the four cells report geometric means for:
- decode latency ratio;
- E2E latency ratio;
- decode throughput ratio.

These are descriptive matched-gap quantities, not a winner score.

## 12. Historical Q2 context

Historical Q2 may be displayed only as context:

```text
W-S old Arc/llama:
TTFT 9.1535×
decode throughput 0.02499×
E2E latency 38.7580×

W-C old Arc/llama:
TTFT 10.3313×
decode throughput 0.03061×
E2E latency 23.9398×
```

Historical and fresh runs must not be algebraically mixed into a causal estimate because environment/execution design differ.

The fresh I003 ratios are authoritative for the post-I002 practical gap.

## 13. Validity classifications

Exactly one final characterization class:

### I003_MATCHED_EXTERNAL_CHARACTERIZATION_COMPLETE
Require:
- exact candidate/baseline/model/workload contracts;
- all 20 pairs recorded;
- at least 3 successful matched pairs in every cell;
- valid primary timing for successful arms;
- baseline full-offload/runtime qualification valid;
- evidence/provenance complete.

### I003_BASELINE_NOT_MATCHED
Pinned llama.cpp cannot execute the exact matched contract.

### I003_RUNTIME_INCOMPLETE
Fewer than 3 valid matched pairs in any cell due genuine runtime failures.

### I003_MEASUREMENT_INVALID
Instrumentation/provenance cannot support matched comparison.

No other classification is permitted.

## 14. Decision boundary after I003

I003 itself selects no next kernel.

After a valid complete fresh table:
- if a large practical external gap remains, open a **post-I002 exact device-work profile** before selecting any new mechanism;
- if ArcLLM is near the external baseline, do not reflexively optimize the next kernel; examine product/runtime integration, memory/resource behavior and reproducibility first;
- if the result is mixed by workload/metric, decompose that observed regime difference before choosing an intervention.

The decision is made from the complete fresh evidence, not from historical Q2 projection.

## 15. Current authorization

Authorized now:
- bounded benchmark adaptation;
- exact baseline build/qualification;
- zero-science static/build preflight;
- DEV_HOST metadata instrumentation.

Not authorized:
- fresh measured external comparison.

Fresh I003 measurements require a separate post-preflight authorization.
