# ArcLLM v1 — I001R Exact 7B Decode Device-Work Adjudication

**Date:** 2026-09-22  
**Branch:** `research/arcllm-v1`  
**Execution HEAD:** `8b4b8e1705df8c0b0b961485046c9ba67c51d78a`  
**Bundle SHA256:** `5C69C5ADD37A5C54F58942A4A4A2B74F21111FBED17D028AFDCCB02418A1ECBE`  
**Result:** `PASS_VALID_COLLECTION_DOMINANT_DEVICE_FAMILY_FFN_GATE_UP`

## 1. Validity

The returned collection is valid under the frozen v0.1.3 contract.

Verified independently from raw bundle files:

- exact execution HEAD and branch;
- exact 7B model SHA256 and 4,683,074,048-byte size;
- Windows build 26200;
- Intel Core Ultra 7 258V;
- Intel Arc 140V 16GB;
- driver 32.0.101.8860;
- AC online and frozen Balanced power scheme;
- exact frozen cell order A_WS → A_WC → B_WC → B_WS;
- 20/20 measured attempts present and successful;
- 60/60 profiled decode steps present;
- 560/560 normal lifecycle steps present;
- every profiled step contains 469 raw op timestamps;
- fixed probe indices 0 / 15 / 30;
- finite outputs and exact dispatch census on every attempt;
- no automatic rerun;
- no selective rerun;
- no production optimization;
- no PDEP implementation;
- no replacement intervention selected during collection.

Semantic stability also passes:

- W-S: 10/10 measured attempts have identical 32-token sequence, generated hash, final-logits hash, and final-hidden hash;
- W-C: 10/10 measured attempts have identical 32-token sequence, generated hash, final-logits hash, and final-hidden hash.

## 2. Independent recomputation

The formal decision was recomputed from the raw 469-op timestamp vectors rather than accepted from `I001R_PROFILE_SUMMARY.json`.

Global median family shares over the 20 measured attempts:

| Family | Median device-chain share |
|---|---:|
| **ffn_gate_up** | **59.6727%** |
| lm_head | 21.4557% |
| ffn_down | 15.8168% |
| attn_qkv | 2.5975% |
| attn_output | 1.0218% |
| attention | 0.2093% |
| rmsnorm | 0.0340% |
| residual_add | 0.0152% |
| rope | 0.0111% |
| swiglu | 0.0098% |
| kv_store | 0.0053% |
| embedding | 0.0006% |

Global medians:

```text
barrier / unattributed share       = 0.04596%
lifecycle outside submit share     = 0.08170%
```

Both are far below the frozen 10% materiality threshold.

## 3. Replication strength of the dominant family

`ffn_gate_up` exceeds the preregistered 20% dominant-family threshold in every independent cell:

```text
A / W-S   64.7356%
A / W-C   57.8115%
B / W-C   59.6859%
B / W-S   58.8697%
```

Across all 20 measured attempts:

```text
minimum share = 54.6698%
maximum share = 65.6827%
median        = 59.6727%
```

Across all 60 individual early/middle/late probes:

```text
minimum share = 52.0104%
maximum share = 68.6476%
median        = 61.1300%
```

Probe-position medians:

```text
decode index  0   60.7912%
decode index 15   60.5440%
decode index 30   62.1320%
```

The dominant-family result is therefore not produced by one session, one workload, or one context position.

## 4. Exact source mapping

The exact Q2-safe decode graph contains:

```text
28 layers
2 FFN gate/up dispatches per layer
= 56 ffn_gate/up dispatches per token
```

Both use:

`p7_q4k_gemm_2d.spv`

with exact gate/up geometry:

```text
quant       Q4_K
batch       1
K / n       3584
rows        18944
bias        false
row_bytes   2016
```

Gate and up are executed as two independent matvecs over the same normalized input.

Exact logical Q4 weight payload per token:

```text
one gate/up matrix       38,191,104 bytes
gate + up / layer        76,382,208 bytes
28 layers             2,138,701,824 bytes
```

This is approximately 2.139 GB of logical Q4 weight payload per generated token.

The input vector is only 3,584 FP32 values. Even if gate/up fusion perfectly eliminated one full re-read of that input per layer, the theoretical byte saving is only:

```text
3584 × 4 × 28 = 401,408 bytes / token
```

or about 0.019% of gate/up weight payload.

Therefore the exact profile does **not** support a large benefit hypothesis based merely on:
- reducing two dispatches to one;
- removing a barrier;
- reusing the small activation vector.

A material mechanism must improve packed-Q4 device work itself: work partitioning, packed dequantization, memory access/coalescing, subgroup utilization, or a coupled mechanism.

## 5. Historical mechanism evidence now becomes directly relevant

SA1 had already tested the exact same Q4 gate/up geometry:

`Q4_H3584_R18944_NOBIAS`

against the exact baseline `p7_q4k_gemm_2d.comp`.

Frozen SA1 mechanism:

`SUBGROUP32_SPLIT_K_PER_OUTPUT_ROW`

SA1 result on Intel Arc 140V:

```text
Process A speedup   5.33376×
Process B speedup   5.50431×
correctness         PASS
```

The candidate changed work partitioning:

```text
baseline:
one invocation owns one output row
and scans all K serially

SA1:
one 32-lane subgroup owns one output row
lanes split K
subgroup reduction
lane 0 stores row result
```

This is unusually strong prior evidence because it matches:
- quant format;
- batch size;
- n=3584;
- rows=18944;
- no-bias gate/up geometry;
- hardware;
- baseline shader semantics.

SA1 used deterministic component fixtures, not real target-model activations/weights. Therefore transfer into the 7B model remains to be demonstrated.

## 6. Carry-through headroom

Using the independently recomputed exact-7B gate/up share:

`f = 0.5967268`

and the SA1 exact-shape speedups:

```text
s_A = 5.33376×
s_B = 5.50431×
```

Amdahl device-chain envelopes are:

```text
A-based carry-through envelope   ≈ 1.941×
B-based carry-through envelope   ≈ 1.954×
geometric-mean mechanism         ≈ 1.948×
```

These are **device-chain predictions under transfer**, not observed target-model speedups.

For reference:

```text
2× gate/up speedup → ~1.425× device-chain
3×                 → ~1.661×
4×                 → ~1.810×
ideal elimination  → ~2.480×
```

The gate/up family therefore has enough theoretical and empirically supported headroom to justify a bounded model-level carry-through intervention.

## 7. Analyzer QA note

The returned summary contains:

`quant_linear_family_share_sum_of_medians = 1.0056456`

This quantity is not a valid compositional share because independent per-family medians were summed and may exceed 1.

Independent recomputation gives:

`median(per-attempt sum of quant-linear shares) = 0.9965738`

The defective summary field is **not used** in this adjudication and does not affect the preregistered dominant-family decision.

## 8. Formal decision

Frozen rule:

> if one device family median share ≥20% → `DOMINANT_DEVICE_FAMILY_IDENTIFIED:<family>`

Independent result:

```text
ffn_gate_up = 59.6727%
threshold   = 20%
ratio       = 2.984× threshold
```

Therefore:

`PASS_VALID_COLLECTION_DOMINANT_DEVICE_FAMILY_FFN_GATE_UP`

and:

`I001R_EXACT_7B_PROFILE_CLOSED`

## 9. Consequence for ArcLLM v1

The PDEP hypothesis remains falsified as first implementation.

The exact 7B evidence redirects the architecture-learning loop to:

```text
Q4_K batch-1 FFN gate/up device work
        ↓
known subgroup32 split-K mechanism
        ↓
real-model transfer / carry-through test
        ↓
decode + E2E + TTFT
```

No evidence supports reopening generic dispatch-count reduction.

## 10. Next intervention

Select a new bounded intervention:

`I002 — Q4 FFN Gate/Up Subgroup32 Split-K Decode Executor`

I002 must initially modify **only the 56 decode gate/up nodes**.

It must not:
- modify prefill;
- replace Q/K/O projection kernels;
- replace FFN-down;
- change model/quantization;
- change decode graph semantics;
- change thresholds after outcome exposure.

The first I002 stage is a real-model transfer and carry-through specification, not broad runtime optimization.
