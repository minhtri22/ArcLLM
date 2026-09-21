# ANL64 P2 — Exact Architecture Mapping and E2E Upper Bound

**Date:** 2026-09-21  
**Mode:** ZERO-SCIENCE / SPECIFICATION ONLY  
**Decision:** `P2_PASS_OPEN_P3_ZERO_SCIENCE_COMPATIBILITY`

## 1. Exact architecture mapping

ANL64 uses a model-load-time control plane and an exact-model Vulkan data plane.

```text
GGUF tensor metadata + exact device capability
        ↓
ANL64 planner
        ↓
immutable hashed execution plan
        ↓
heterogeneous executor selection
        ↓
prepared Vulkan chain
        ↓
one queue submit / decode token
```

No per-token autotuning, executor search, graph discovery, full-N scan, region sort or payload repack is permitted.

The planner preserves the exact logical dependency order of the frozen Q2 graph.

## 2. Ledger64 lane / region meaning

A Ledger64-derived region is admitted only for quantized linear work.

```text
one lane   = one logical output row
one region = 64 contiguous logical output rows
row_base   = region_index × 64
row        = row_base + lane
```

For the exact target, all admitted projection and LM-head row counts are multiples of 64:

- H=3584 → 56 regions;
- KV=512 → 8 regions;
- FFN=18944 → 296 regions;
- vocab=152064 → 2376 regions.

Therefore every exact-target region has a full 64-bit lane mask.

This is **not** a sparse-frontier claim. The dense decode graph has no active-operator frontier. Event-Ledger's dynamic sparse queue is not inserted into the hot path.

Ledger64 contributes a direct region/lane representation for the static plan, not an inherited NEXUS speedup.

### Exact region census

Per decoder layer:

```text
Q       56
K        8
V        8
O       56
gate   296
up     296
down    56
-------------
total  776
```

Across 28 layers:

```text
decoder quant regions = 21,728
LM-head regions        =  2,376
total                  = 24,104
```

The five projection families known to be Q4_K in every layer are Q/K/O/gate/up:

```text
fixed-Q4 regions/layer = 712
fixed-Q4 regions       = 19,936
```

V and down remain quant-type conditional.

## 3. Executor table

### Q4_FAST_FIXED

Applies to every layer's:

- Q projection;
- K projection;
- O projection;
- gate projection;
- up projection.

The mechanism is seeded by the closed SA1 Q4 subgroup-32 split-K evidence. It is not yet locally integrated into the real model.

### Q4_FAST_CONDITIONAL

V/down may use the same Q4 family only when their exact GGUF tensor type is Q4_K and P3 admits the mapping.

No P2 upper-bound credit is taken for these paths.

### Q6_SAFE

Q6 V/down and the Q6 segmented LM head remain on correctness-preserving existing paths.

P2 assumes **zero Q6 speedup**.

### Existing-safe paths

Embedding, normalization, RoPE, KV store, cached GQA, residual, SwiGLU and output norm remain unchanged.

P2 assumes **zero improvement** for all of them.

## 4. Planner semantics

The plan is built once at model load from:

- exact GGUF tensor role;
- exact quant type;
- exact shape;
- current arena/buffer binding;
- exact device capability record.

Executor selection is deterministic.

There is no online benchmark or performance search.

The prepared command-chain property is retained:

```text
decode dispatches/token = 469
queue submits/token      = 1
```

ANL64 control-plane work adds:

```text
extra control GPU dispatch/token = 0
extra queue submits/token        = 0
```

## 5. Residency / prefetch authority

The first ANL64 architecture keeps existing full-model residency:

```text
weights  = 4,677,120,000 B
KV       =   469,762,048 B
working  =   200,888,324 B
total    = 5,347,770,372 B
usable   =16,374,562,816 B
headroom =11,026,792,444 B
```

Therefore P2 does **not** introduce:

- SSD streaming;
- weight migration;
- duplicated weights;
- predequant cache;
- offline repack;
- per-token prefetch pass.

Those mechanisms would be new interventions and require separate evidence.

### Static control metadata budget

P2 freezes:

- max 64 bytes/region descriptor;
- max 128 bytes/PlanNode;
- max 469 PlanNodes.

For 24,104 regions the calculated upper bound is:

```text
1,602,688 bytes
```

with a hard cap of:

```text
2 MiB
```

This is less than 0.04% of current resident bytes.

## 6. Static packed-weight materiality

Five projection families are guaranteed Q4_K in every layer:

```text
Q      7,225,344 B
K      1,032,192 B
O      7,225,344 B
gate  38,191,104 B
up    38,191,104 B
-------------------
       91,865,088 B / layer
```

Across 28 layers:

```text
fixed-Q4 packed bytes = 2,572,222,464 B/token
```

Depending on whether V/down are Q4 or Q6, total decoder projection packed bytes lie between approximately 3.670 GB and 4.174 GB.

The Q6 LM head contributes approximately 0.447 GB.

Thus the guaranteed fixed-Q4 set represents roughly **55.7%–62.5%** of projection+LM packed-byte volume.

This is a static byte-coverage proxy only; it is **not** GPU-time attribution.

## 7. Empirical component substitution bound

SA1 Q4 measured the exact shape families used by the guaranteed fixed-Q4 paths.

P2 deliberately avoids cherry-picking process A or B. For each shape it takes the **smaller A/B measured baseline-minus-candidate median saving**:

```text
Q       0.3403905 ms
K       0.6639060 ms
O       0.3445575 ms
gate   11.3044525 ms
up     11.3044525 ms
----------------------
layer  23.9577590 ms
```

Across 28 layers:

```text
670.817252 ms / decode token
```

This is used only as an optimistic substitution upper bound:

> Assume standalone component savings transfer one-for-one into the integrated prepared decode chain, while Q6, attention, LM head and all other work remain unchanged.

It is not an ANL64 measurement or expected performance.

## 8. E2E upper-bound test

P2 reuses the already frozen Q3 practical E2E threshold:

```text
E2E Arc / comparison ≤ 0.90
```

It does not invent a new materiality threshold.

Applying only the 670.817252 ms/token fixed-Q4 substitution saving to the 31 decode steps of each historical Q3 cell gives:

| Historical cell | Current decode tok/s | Upper-bound decode tok/s | Decode speedup | Upper-bound E2E ratio |
|---|---:|---:|---:|---:|
| A / W-S | 0.33478 | 0.43174 | 1.2896× | 0.7784 |
| A / W-C | 0.29708 | 0.37102 | 1.2489× | 0.8247 |
| B / W-S | 0.32706 | 0.41899 | 1.2811× | 0.7830 |
| B / W-C | 0.31784 | 0.40398 | 1.2710× | 0.8136 |

All four are below 0.90.

The required fraction of the standalone Q4 saving that would have to survive integration to reach 0.90 is:

```text
A/W-S  45.13%
A/W-C  57.04%
B/W-S  46.08%
B/W-C  53.65%
```

The worst historical cell therefore requires approximately **57.04% transfer** of the observed fixed-Q4 component saving, assuming other work and control overhead are unchanged.

This is useful headroom information, not a P5 performance claim.

## 9. Why P2 passes

P2 does **not** require:

- Q6 optimization;
- fusion;
- attention redesign;
- LM-head redesign;
- sparse operator skipping;
- weight repacking;
- a new quantization format.

A material upper bound exists using only the Q4 component family that already has closed positive evidence, while every other path is held unchanged.

Therefore the architecture is not dependent on speculative improvement across most of the graph.

## 10. Falsification / stop conditions

ANL64 stops before scientific performance work if any of these becomes true:

1. the guaranteed Q4 Q/K/O/gate/up nodes cannot be mapped to Q4_FAST while preserving exact buffer/base-offset semantics without repack/predequant copy;
2. executor selection requires per-token discovery/sort/full graph scan or a new control-plane GPU dispatch/queue submit;
3. the bound device/driver cannot satisfy the inherited subgroup-32 arithmetic contract;
4. integrated Q4 correctness fails;
5. the static plan exceeds the 2 MiB metadata budget;
6. later integration evidence cannot plausibly reach the frozen material E2E target without assuming unmeasured gains in Q6/attention/other paths.

No fusion, Q6 optimization, sparse skipping, precision change, prefetch mechanism or online autotuning may be added as a rescue under this P2 architecture.

## 11. P2 adjudication

```text
EXACT ARCHITECTURE MAPPING                PASS
NATURAL LEDGER64 MAPPING                 PASS
PLANNER SEMANTICS                        PASS
RESIDENCY/PREFETCH AUTHORITY             PASS
STATIC COST MODEL                        PASS
E2E UPPER BOUND                          PASS
FALSIFICATION/STOP CONDITIONS            FROZEN
IMPLEMENTATION                           NOT AUTHORIZED
SCIENTIFIC PERFORMANCE MEASUREMENT       NOT AUTHORIZED
```

Final P2 result:

> **P2_PASS_OPEN_P3_ZERO_SCIENCE_COMPATIBILITY**

P3 may now test only local compatibility/expressibility of this frozen architecture. It may not yet claim performance or run a matched E2E experiment.
