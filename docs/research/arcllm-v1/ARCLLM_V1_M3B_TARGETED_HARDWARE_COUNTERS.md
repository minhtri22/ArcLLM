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
- preserve every pass as instrumented raw evidence;
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
`VkPerformanceQuerySubmitInfoKHR.counterPassIndex`. Every selected query must be recorded once
for every required pass before results are consumed.
