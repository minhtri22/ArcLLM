# ArcLLM v1 — I001 Decode Cost Model and Mechanism Discriminator

**Date:** 2026-09-22  
**Branch:** `research/arcllm-v1`  
**Parent HEAD:** `ad9a9922948f93d9cafe721545dcdc9f5b20d5fc`  
**Status:** HISTORICAL-EVIDENCE DISCRIMINATOR COMPLETE / NO NEW EXECUTION  
**I001 family under review:** `I001-PDEP — Persistent Decode Execution Plane / Decode Graph Compression`

## 1. Question

Before implementing PDEP, determine whether existing ArcLLM evidence supports a large enough **execution-topology-removable** share of decode cost to justify code.

This discriminator separates:

```text
H-LAUNCH/LIFECYCLE
H-SYNC
H-LOCALITY
H-KERNEL
H-MIXED
H-NULL-I001
```

No new performance run is used here. This is a retrospective synthesis of frozen historical evidence.

## 2. Evidence classes and transfer limits

### Exact 7B evidence

Q2/Q3 and ANL64 use the exact 7B target:

`60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`

Q2 establishes:

- 469 decode dispatches per token step;
- 31 decode steps per measured inference;
- W-S decode median = `0.3184449679 tok/s`;
- W-C decode median = `0.3990063239 tok/s`.

Equivalent ArcLLM decode-step durations are approximately:

```text
W-S  3140.26 ms/token
W-C  2506.23 ms/token
```

These are exact 7B measured values.

### Device-category proxy evidence

P7-H and P7-M are historical 1.5B timestamp profiles on the same 469-dispatch decode topology family and the same prepared-chain Vulkan execution design.

They are **architecture proxies**, not exact 7B category measurements.

They may support:
- topology shape;
- where measured device ticks concentrate;
- order-of-magnitude lifecycle/barrier ceilings.

They may not establish exact 7B category shares.

## 3. Existing execution topology

The production decode path has 469 dispatches per token.

The execution path already performs substantial preparation before timing:

- pipelines and descriptor sets are prepared once;
- a prepared chain is reused;
- each token step constructs the concrete `DispatchOp` values for the new position;
- `execute_prepared` records one command buffer;
- one queue submit and one fence wait execute the 469-dispatch chain.

Therefore PDEP is **not** starting from a completely unprepared pipeline-per-dispatch design.

Inside `execute_prepared`, the historical implementation creates a transient command pool, allocates/records one primary command buffer, binds the pre-created pipelines/descriptors, dispatches the chain, inserts compute barriers, creates one fence, submits once, waits once, and destroys temporary execution objects.

## 4. Host lifecycle envelope from historical timestamp profiles

### P7-H

```text
record_submit_wait_ms = 302.3118
submit_wait_ms        = 301.0001
outside-submit bucket =   1.3117 ms
share                 =   0.4339%
ideal elimination     ≈   1.00436× wall speedup
```

### P7-M

```text
record_submit_wait_ms = 283.4147
submit_wait_ms        = 273.0831
outside-submit bucket =  10.3316 ms
share                 =   3.6454%
ideal elimination     ≈   1.03783× wall speedup
```

The outside-submit bucket is broader than pure production lifecycle overhead because the profiled path also creates/reads/destroys timestamp-query state.

Therefore the 0.43–3.65% range must **not** be interpreted as an exact production host-overhead measurement.

However, it establishes an important historical bound-like observation:

> Host/API/recording work outside queue-submit+wait is not a dominant fraction in either frozen 469-dispatch proxy profile.

## 5. Device synchronization / unattributed envelope

P7-H:

```text
barrier_or_unattributed / chain = 0.3455%
ideal elimination ceiling       ≈ 1.00347×
```

P7-M:

```text
barrier_or_unattributed / chain = 0.3534%
ideal elimination ceiling       ≈ 1.00355×
```

Because this bucket contains both barriers and other unattributed device intervals, the true removable barrier-only share cannot exceed this bucket under these profiles.

Therefore the historical evidence strongly disfavors:

`H-SYNC = dominant decode cost`

for this execution family.

## 6. Device-work concentration

The six largest P7-M decode categories are:

| Category | Device-chain share |
|---|---:|
| FFN gate/up | 34.3204% |
| FFN down | 21.6450% |
| LM head | 17.9980% |
| Attention | 11.1876% |
| Attention Q/K/V projections | 10.4463% |
| Attention output projection | 3.6104% |

Combined:

`99.2078%`

P7-H independently gives:

`99.2223%`

for the analogous six categories.

This does **not** prove whether each category is compute-bound, memory-bound, under-occupied, or locality-limited.

It does establish that the measured device chain is overwhelmingly spent **inside useful kernel regions**, not between them.

## 7. ANL64 causal discriminator for topology versus device work

ANL64 is especially informative because:

- candidate and reference both preserve the 469-node/dispatch semantic execution census;
- the candidate replaces execution for a substantial set of quant-linear nodes;
- candidate plan contains:
  - 469 nodes;
  - 215 quant-linear nodes;
  - 140 fixed-Q4-fast nodes;
- exact token semantics are preserved.

Fresh 7B decode candidate/reference speedups:

```text
A / W-S   2.5318×
A / W-C   1.2228×
B / W-S   2.1635×
B / W-C   3.2002×
```

The important discriminator is not simply that ANL64 was faster.

It is:

> Multi-x decode movement occurred **without reducing the 469-node topology**.

ANL64 does not isolate one kernel mechanism perfectly, but it is direct evidence that large recoverable cost exists in **device work / kernel execution choices** even when graph node count is unchanged.

## 8. Mechanism adjudication

### H-LAUNCH/LIFECYCLE

**Classification:** `LOW_HEADROOM_AS_PRIMARY_MECHANISM`

Evidence:
- outside-submit bucket only 0.43–3.65% in two 469-dispatch timestamp profiles;
- prepared pipelines/descriptors already reused;
- only one queue submit/fence wait per token chain.

This bucket may still be worth cleaning later, but it cannot justify Intervention-001 priority.

### H-SYNC

**Classification:** `FALSIFIED_AS_DOMINANT_PRIMARY_MECHANISM_BY_HISTORICAL_PROXY`

Evidence:
- barrier+unattributed device share ~0.35% in two independent profiles.

A future exact 7B profile may revise the numerical share, but current evidence gives no basis to prioritize barrier removal.

### H-LOCALITY

**Classification:** `UNRESOLVED`

Region fusion could theoretically improve cache/local-state reuse and reduce internal memory traffic even when explicit barrier ticks are small.

Historical evidence does not measure bytes moved or cache behavior well enough to attribute a material share.

This mechanism cannot currently authorize PDEP code.

### H-KERNEL

**Classification:** `SUPPORTED_AS_DOMINANT_NEXT-DISCRIMINATION CLASS`

Evidence:
- ~99.2% of proxy device-chain time is inside six compute categories;
- ANL64 achieves multi-x decode movement while preserving the 469-node topology and changing quant-linear execution.

This supports focusing the next exact 7B discriminator on **device-work efficiency by family**.

### H-MIXED

**Classification:** `POSSIBLE_BUT_UNQUANTIFIED`

A future architecture may combine kernel, locality, and execution-policy improvements.

Historical evidence does not justify treating topology-only PDEP as the anchor mechanism.

### H-NULL-I001

Defined for the original PDEP question as:

> execution-topology-removable cost is too small or insufficiently supported to justify PDEP as the first implementation.

**Classification:** `SUPPORTED_FOR_PDEP_AS_FIRST_IMPLEMENTATION`

## 9. Decision

```text
DECISION =
FALSIFY_I001_PDEP_AS_FIRST_IMPLEMENTATION
```

Meaning:

- do not implement persistent decode / graph compression merely to reduce API calls, command recording, dispatch count, or barriers;
- do not infer a large payoff from the raw number `469`;
- retain persistence/region fusion only as a secondary mechanism if a future exact 7B device-work study shows locality/state-reuse benefit inside dominant compute families.

This is **not** a universal claim that persistent execution can never help ArcLLM.

It is a priority decision based on current cost evidence.

## 10. Updated headroom ordering

The decode plane remains priority #1, but its internal ordering changes.

### Previous

```text
decode plane
  → PDEP / graph compression
```

### Updated

```text
decode plane
  → exact 7B device-work cost profile
  → dominant kernel/dataflow family
  → quantify compute vs memory/locality headroom
  → only then decide whether region fusion/persistence is part of the mechanism
```

## 11. What exact evidence is still missing

The current repository does **not** contain an exact 7B per-family Vulkan timestamp profile comparable to P7-H/P7-M.

Therefore these remain unknown for the Q2/Q3 7B path:

- FFN gate/up exact share;
- FFN-down exact share;
- LM-head exact share;
- attention/projection exact shares;
- device memory-traffic estimate;
- arithmetic-intensity / lower-bound estimate;
- per-family dispatch-duration distribution;
- exact production lifecycle overhead without profiling instrumentation;
- exact 7B barrier/unattributed share.

These are now the minimum measurements needed before a replacement implementation is selected.

## 12. Next scientifically valid step

Open a **measurement-only exact 7B study**:

`ARCLLM_V1_I001R_EXACT_7B_DECODE_DEVICE_WORK_PROFILE.md`

Its job is to timestamp the unchanged Q2/Q3-safe 7B decode graph and produce:

```text
device time by semantic family
per-dispatch duration distribution
record/submit/wait decomposition
barrier/unattributed envelope
bytes-accessed estimates where defensible
family-level lower-bound / headroom model
```

No production optimization is authorized by this discriminator.

The next implementation candidate must be selected **after** that exact 7B profile.
