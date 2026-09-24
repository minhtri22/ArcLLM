# ArcLLM v1 — M3-B Targeted Vulkan Hardware Counters

## Question

For the current post-I002 decode graph, which physical hardware conditions accompany the residual
excess cost identified by M1/M2?

M3-B is descriptive hardware-state evidence, not an optimization test.

## Qualified provider

M3-A qualified `VK_KHR_performance_query` on the exact Arc 140V:

- 268 counters;
- COMMAND scope;
- full inventory = 12 passes;
- VTune absent;
- provider-native counters are performance-impacting and concurrently-impacted.

## Collection

Run six independent processes:

```text
W-S × {memory_cache, execution_occupancy, stall_cause}
W-C × {memory_cache, execution_occupancy, stall_cause}
```

Each process:

- one warmup;
- one measured inference;
- profile decode index 15 only;
- counter query around each of the 469 dispatches;
- compute selected-group pass count from the driver at runtime;
- restore the exact pre-dispatch mutable state before every pass;
- preserve the required pass count and pass-index submission contract; emit counter values only after all required passes complete;
- emit semantic NodeID + dispatch_id + provider-native counter values.

Quiet-host mode is required for M3-B. The machine hardware profile itself is not re-audited.

## Counter groups

### memory_cache

Physical GPU-memory bytes, LSC bytes/hit/access, L3 hit/miss/stall, TLB, memory-request-queue
pressure, and memory-active state.

### execution_occupancy

GPU/core time, threadgroups, XVE active/occupancy/stall, issued/ALU/SEND instructions,
thread-dispatch pressure, compute-shader ALU utilization.

### stall_cause

XVE ALU-write, barrier, control, instruction-fetch, pipeline, scoreboard and SEND stalls, plus
L3/SuperQ and LSC input/output pressure.

## Interpretation rules

1. Counter timing does not replace I003/M1 timing.
2. A provider-native counter is authoritative; normalized names are aliases.
3. Overlapping stall percentages are not summed into a total.
4. Multi-pass observations remain explicitly multi-pass.
5. COMMAND-scope data can be joined to one Token-XRay dispatch; multi-dispatch semantic nodes
   such as LM-head are aggregated only after dispatch-level evidence exists.
6. Counter evidence is descriptive. The later Q4-down sentinel remains the causal intervention.

## Vulkan rules

The profiling lock must be held before recording a command buffer containing performance queries
and while it is recording/executable/pending. Required pass count is selected at submit time with
`VkPerformanceQuerySubmitInfoKHR.counterPassIndex`. Every selected query must be recorded once for every required pass before results are consumed. `vkCmdResetQueryPool` is recorded in a dedicated reset-only command buffer, submitted and completed once before pass 0. No performance-query begin/end occurs in that reset command buffer, and no reset occurs between required passes. The final counter values are labeled `COMBINED_AFTER_REQUIRED_PASSES`, not fabricated as per-pass measurements.


## Token-XRay contract binding

M3-B emits `HARDWARE_OBSERVATION` records compatible with Token-XRay main
`03e4c7ef5fc0dbba52f99d96669ac40ce18b5e6e`.

Each of the six raw runs yields 469 dispatch-scoped observations. Total expected observations:
`6 × 469 = 2,814`.

The semantic mapping is the existing ArcLLM Token-XRay adapter. LM-head remains 19 dispatch
observations sharing the stable semantic NodeID `decode.output.lm_head`; aggregation happens only
after dispatch-level evidence exists.

## Pre-run attribution hardening

Before the first M3-B collection, three validity changes were made without changing the counter
groups, model, workloads, decode index, shaders, or scientific question.

1. **Exact query boundary.** A compute-to-bottom-of-pipe barrier is recorded after each dispatch
   and before `vkCmdEndQuery`. This intentionally serializes the instrumented path so each
   COMMAND-scope observation encloses completed dispatch work. Counter-run timing remains
   diagnostic only.
2. **Shared-memory cache-pollution guard.** Full transient/KV host memcpy restoration was removed.
   The decode graph overwrites those outputs/current-KV slots before read; only the fixed 4-byte
   `b_dec_id` token input is restored between counter passes. Generated-hash guards remain
   mandatory.
3. **Current Token-XRay multi-pass contract.** Final observations record
   `pass_indices_executed=[0..pass_count-1]`, `pass_index=null`, and
   `pass_semantics=COMBINED_AFTER_REQUIRED_PASSES`.

