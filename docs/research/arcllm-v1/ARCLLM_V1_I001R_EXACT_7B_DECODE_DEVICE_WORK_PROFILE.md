# ArcLLM v1 — I001R Exact 7B Decode Device-Work Profile

**Study:** `ARCLLM_V1_I001R_EXACT_7B_DECODE_DEVICE_WORK_PROFILE`  
**Status:** FROZEN MEASUREMENT-ONLY DESIGN + IMPLEMENTATION  
**Parent decision:** `FALSIFY_I001_PDEP_AS_FIRST_IMPLEMENTATION`  
**Production optimization:** forbidden in this study

## 1. Scientific question

On the exact Q2/Q3-safe 7B ArcLLM graph, where does decode device time actually concentrate?

The study measures the unchanged 469-dispatch decode graph so that the next intervention is selected from **exact 7B device-work evidence**, not from 1.5B proxy profiles.

## 2. Frozen semantic substrate

The profiler inherits the Q2 graph builder byte-for-byte.

Frozen invariants:

```text
model       Qwen2.5-Coder 7B GGUF
model SHA   60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463
layers      28
vocab       152064
prefill     441 dispatches
decode      469 dispatches / step
decode      31 steps / inference
CPU math fallback false
teacher forcing    false
shaders      exact Q2-safe 16-shader set
```

No ANL64, PDEP, fused successor, graph-compression, or new production kernel is admitted.

## 3. Instrumentation design

Each full inference runs all 31 decode steps.

For measured attempts:

- decode indices `0`, `15`, and `30` use Vulkan timestamp instrumentation around every dispatch;
- the other 28 decode steps use the unchanged production `execute_prepared` path;
- warmup uses the normal path only.

This yields representative early/middle/late context probes without turning every token step into a query-instrumented execution.

Each profiled probe records:

- exact 469 op names;
- timestamp period in ns;
- whole-chain ticks;
- per-dispatch ticks;
- barrier/unattributed ticks;
- record/submit/wait timings.

Each normal decode step records production-path lifecycle timing through the unchanged `ChainStats`.

## 4. Sessions and repetition

Counterbalanced order:

```text
Session A: W-S → W-C
Session B: W-C → W-S
```

Each cell:

```text
1 warmup
5 measured attempts
```

Total expected:

```text
4 cells
20 measured full inferences
60 profiled decode steps
560 normal lifecycle decode steps
```

No automatic rerun and no selective rerun are permitted.

## 5. Device-family taxonomy

Raw op names and raw ticks are preserved. A convenience family map is also emitted:

- embedding
- rmsnorm
- attn_qkv
- rope
- kv_store
- attention
- attn_output
- residual_add
- ffn_gate_up
- swiglu
- ffn_down
- lm_head
- other

Independent adjudication may reclassify from raw op names if the convenience mapping is wrong.

## 6. Weight-byte model

For full-row linear/norm families the profiler emits the exact GGUF tensor payload bytes logically consumed per decode step.

This is **not measured DRAM traffic**, cache-miss traffic, or a hardware bandwidth counter.

The derived quantity:

`logical tensor bytes / timestamp family time`

may be used only as a model-throughput indicator.

## 7. Validity gates

A valid collection requires:

1. exact model hash/size;
2. exact CPU/GPU/driver/OS/power contract;
3. static graph equivalence to Q2;
4. exact 441/469 dispatch census;
5. four cells in frozen order;
6. five successful measured attempts per cell;
7. finite logits and dispatch guards on all attempts;
8. identical generated 32-token sequence across all measured attempts for the same workload;
9. exactly three probes per measured attempt at decode indices 0/15/30;
10. exactly 469 op ticks per probe;
11. positive timestamp-valid bits and timestamp period;
12. 28 normal lifecycle steps per measured attempt;
13. no automatic/selective rerun.

## 8. Preregistered derived metrics

For each cell and globally:

- median family share of profiled chain ticks;
- median barrier/unattributed share;
- median production lifecycle outside-submit share;
- p50/p90/p99/max dispatch duration by family;
- sum of median shares for quant-linear families;
- logical model-weight throughput by family.

## 9. Decision rules

These rules select the **next discriminator**, not a production implementation.

```text
if lifecycle outside-submit median >= 10%
    → REOPEN_EXECUTION_LIFECYCLE_MECHANISM

else if barrier/unattributed median >= 10%
    → REOPEN_SYNC_MECHANISM

else if one device family median share >= 20%
    → DOMINANT_DEVICE_FAMILY_IDENTIFIED:<family>

else if top two device-family median shares sum >= 40%
    → MIXED_DEVICE_FAMILIES_DOMINANT

else
    → DIFFUSE_DEVICE_WORK_NEEDS_DEEPER_BOUND_MODEL
```

Rationale:

- 10% complete-removal ceiling corresponds to only about 1.11× Amdahl speedup, so smaller lifecycle/sync shares cannot justify first priority by themselves;
- a 20% family share has an ideal zero-cost ceiling of 1.25× and is large enough to justify a family-specific lower-bound study.

No family is implemented directly from this study. The next step must distinguish compute, memory/locality, quant geometry, and occupancy inside the identified dominant family.

## 10. Fail-closed behavior

Any invalid cell or missing invariant:

```text
STOP
preserve partial evidence
no retry
no threshold change
no implementation
```

## 11. Expected output

The one-click local run returns a ZIP containing:

- 4 raw cell JSONs;
- environment;
- build manifest;
- frozen contract;
- run metadata;
- preregistered profile summary.

The bundle is then independently adjudicated before any ArcLLM v1 production optimization is selected.
