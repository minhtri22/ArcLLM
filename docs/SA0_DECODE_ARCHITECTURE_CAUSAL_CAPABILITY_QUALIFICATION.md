# ArcLLM SA0 — Decode Architecture Causal / Capability Qualification

**Date:** 2026-09-21  
**Phase:** SA0  
**Mode:** specification-only / zero target execution  
**Parent candidate:** SA-H1 — Decode-Specialized Packed-Quant Executor  
**Parent verdict:** `FEASIBLE_NO_DEMONSTRATED_ADVANTAGE`  
**Current ArcLLM architecture:** CLOSED  
**New kernel implementation:** NOT AUTHORIZED  
**Model execution:** NOT AUTHORIZED

## 1. SA0 question

SA0 asks a narrower question than “can ArcLLM be optimized?”:

> Is there enough source-level, matched-system and hardware-capability evidence to justify one bounded successor study in which the primary intervention is a decode-specialized packed-quant GEMM/dataflow path, without changing the exact GGUF model, quantization contract, greedy semantics, KV semantics or Q3 workload identity?

SA0 is not a benchmark and cannot establish a successor performance claim.

## 2. Evidence boundaries

### 2.1 ArcLLM authoritative evidence

- final Q3 result: `inputs/q3_formal_result.authoritative.json`, blob `06faac573838b1789e6ef007cd0a9ce11b5e1915`;
- frozen Q2/Q3 production graph: `src/q2_benchmark.cpp`, blob `ea1e986e22f6921e7f6c52a4fa5935121cfec663`;
- P7 production closeout: `docs/P7_CLOSEOUT.md`, blob `1b5b81646575384c44630baf1cce8588ac8d939d`;
- post-verdict candidate: `config/successor_architecture_candidate_v0.1.json`, blob `178c24e7c455c173b50802be03270eabab9c2431`.

### 2.2 NEXUS references — hypothesis support only

Only pre-existing scientific artifacts are admissible:

- R4-C compact result at canonical snapshot `6ee72552159113e3103a84106d83e701a0b48eac`, blob `76f841a8a82b87fdf0205e7f265ae0d9c5910450`;
- EL-L4 formal adjudication, blob `af3cce47905fd94a7109e462c6b128a05dc028b2`;
- cross-branch canonical integration review, blob `d38b247153d7ea7b314959a830b58e73295092f2`.

NEXUS telemetry/F01 work performed after the ArcLLM post-verdict review is explicitly excluded from SA0 evidence.

No NEXUS PASS label transfers into ArcLLM.

### 2.3 External references

External publications and specifications support mechanism plausibility only:

- FlashDecoding++ — https://arxiv.org/abs/2311.01282
- MARLIN — https://arxiv.org/abs/2408.11743
- QServe — https://arxiv.org/abs/2405.04532
- Vulkan specification — https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html
- Intel Core Ultra 7 258V public specification — Intel ARK / product specification page.

Published speedups are not ArcLLM predictions.

## 3. Exact decode graph census

The frozen source constructs one prepared decode chain per token with:

```text
28 decoder layers × 16 dispatches/layer = 448
endpoint dispatches                         = 21
------------------------------------------------
total                                      = 469 dispatches/token
```

Endpoint dispatches are:

```text
token embedding       1
output RMSNorm        1
segmented LM head    19
total                21
```

Each decoder layer contains:

```text
1  attn RMSNorm
2  Q projection
3  K projection
4  V projection
5  Q RoPE
6  K RoPE
7  KV store
8  cached GQA
9  O projection
10 attention residual
11 FFN RMSNorm
12 gate projection
13 up projection
14 SwiGLU
15 down projection
16 FFN residual
```

Seven projection GEMMs occur per layer:

```text
Q + K + V + O + gate + up + down
7 × 28 = 196 projection GEMM dispatches/token
```

Therefore projection GEMMs are `196 / 469 = 41.79%` of dispatch count.

This is a dispatch census, not a GPU-time attribution.

## 4. Important negative causal finding: ArcLLM is not 469 CPU submissions/token

Q2/Q3 require each decode step to satisfy:

```text
dispatch_count = 469
submit_count   = 1
```

The graph is already a prepared command chain submitted once per decode token.

Therefore SA0 rejects the simplistic hypothesis:

> “ArcLLM is slow because the CPU performs 469 Vulkan queue submissions per token.”

That hypothesis is false for the validated runtime.

GPU-side per-dispatch work partitioning, synchronization, materialization, dequantization and utilization remain valid mechanisms.

## 5. Decode/prefill architecture asymmetry

The frozen production path is asymmetric:

### Prefill

- tiled Q/K/V/O projections;
- fused Q4_K gate+up;
- tiled FFN-down Q4_K/Q6_K.

### Decode

Q/K/V/O/gate/up/down use generic batch-1:

- `p7_q4k_gemm_2d.spv`;
- `p7_q6k_gemm_2d.spv`.

Gate and up are separate in decode although the prefill path already has a correctness-validated fused gate+up family.

This source-level asymmetry is direct evidence that decode did not inherit the optimization depth of prefill. P7 closeout independently records that decode remained much slower than the external reference and had not received the same optimization depth.

## 6. Fusion-only bound

### 6.1 Dispatch-count effect

QKV fusion can reduce:

```text
3 dispatches/layer -> 1
saving = 2 × 28 = 56 dispatches
```

Gate+up fusion can reduce:

```text
2 dispatches/layer -> 1
saving = 1 × 28 = 28 dispatches
```

Combined:

```text
469 -> 385 dispatches
84 dispatches removed
17.91% dispatch-count reduction
uniform-cost count-only speedup bound = 469/385 = 1.21818x
```

The uniform-cost number is only a counterfactual count model. Real dispatches have unequal cost.

Its purpose is to reject another overly broad claim:

> Fusion alone, treated merely as dispatch-count reduction, cannot plausibly explain or close a 35.7–56.8× decode-throughput gap.

### 6.2 Activation reread effect

Hidden width is 3584 F32 values:

```text
one hidden vector = 3584 × 4 = 14,336 bytes
```

If Q/K/V currently reread the same normalized hidden vector independently, ideal fusion avoids at most two redundant input-vector reads per layer:

```text
2 × 14,336 × 28 = 802,816 bytes/token
```

Gate+up fusion avoids at most one additional normalized-input reread:

```text
14,336 × 28 = 401,408 bytes/token
```

Combined activation reread reduction:

```text
1,204,224 bytes/token
```

The frozen weight-residency arena total is `4,677,120,000` bytes. The full embedding tensor occupies `152,064 × 2,016 = 306,561,024` bytes, giving a non-embedding resident-footprint proxy of:

```text
4,370,558,976 bytes
```

The 1.204 MB ideal activation reread saving is only about `0.0276%` of this footprint proxy.

Therefore the selected successor mechanism must not be justified as “fusion saves model bandwidth.” Fusion is secondary: it may reduce intermediate materialization, repeated setup and enable better work partitioning/dequant pipelines, but the primary mechanism must be the GEMM/dataflow itself.

## 7. Same-hardware empirical feasibility envelope

Q3 used the exact same model and target hardware for ArcLLM and llama.cpp.

Observed decode throughput:

```text
ArcLLM:
0.2971–0.3348 tok/s across the four fresh session/workload cells

llama.cpp:
11.3508–18.5754 tok/s
```

Equivalent baseline/Arc decode factor:

```text
35.71×–56.79×
```

This does not identify the root cause, but it establishes a strong feasibility fact:

> The exact model/hardware combination can execute autoregressive decode far faster than the current ArcLLM decode path.

Thus the Q3 gap is not an intrinsic “Arc 140V cannot run this model faster” limit.

### 7.1 Resident-footprint processing-rate proxy

Using the non-embedding resident-footprint proxy `4,370,558,976 bytes/token`:

```text
ArcLLM footprint-rate proxy:
~1.30–1.46 GB/s

llama.cpp footprint-rate proxy:
~49.6–81.2 GB/s
```

This is **not measured DRAM bandwidth**. It ignores cache effects, padding, read amplification and exact tensor-touch behavior.

It is retained only as a same-model processing-rate proxy showing that the current Arc path is far from the rate already demonstrated by a matched implementation on the same machine.

## 8. Hardware capability qualification

Public Intel specification identifies the target class as:

- Intel Core Ultra 7 258V;
- Intel Arc 140V GPU;
- 8 Xe-cores;
- graphics max dynamic frequency up to 1.95 GHz;
- up to 32 GB LPDDR5X-8533 system memory.

The current ArcLLM runtime already proves these minimum capabilities on the exact machine:

- Vulkan compute execution;
- storage-buffer based packed-weight access;
- direct Q4_K/Q6_K packed execution;
- persistent decoder residency;
- GPU-resident KV;
- query timestamps;
- prepared multi-dispatch command chains.

These are sufficient to justify a successor kernel study that does **not** depend on optional extensions.

### 8.1 Capability-dependent accelerants

The following must not be assumed from marketing hardware identity:

- exact subgroup size and supported subgroup operations;
- `subgroupSizeControl` / `computeFullSubgroups`;
- extended subgroup scalar types;
- exact workgroup shared-memory limits usable by the intended tile;
- cooperative-matrix support and supported shapes/types;
- any vendor-specific asynchronous-copy mechanism.

Vulkan defines APIs/properties to query subgroup and cooperative-matrix support, but support is implementation/device specific.

Therefore these features are optional accelerants only until an exact-device zero-science capability probe binds them to the target driver.

## 9. Causal decomposition

SA0 separates SA-H1 into four sub-hypotheses.

### SA-H1a — PRIMARY: batch-1 packed-quant GEMM/dataflow under-utilization

Claim:

> Generic decode Q4_K/Q6_K GEMM work partitioning/dequant dataflow is a major avoidable contributor to the ArcLLM decode deficit.

Support:

- direct source evidence of generic batch-1 decode kernels;
- strong prefill/decode architecture asymmetry;
- same-hardware matched baseline demonstrates much higher exact-model decode throughput;
- FlashDecoding++ independently identifies flat GEMM under-utilization and static-dataflow loss;
- MARLIN independently demonstrates that quantized autoregressive GEMM requires specialized scheduling/pipelining;
- QServe independently shows dequantization/layout overhead can dominate low-bit execution when not co-designed.

Status: **CAUSALLY PLAUSIBLE / QUALIFIED FOR A BOUNDED SUCCESSOR STUDY**.

### SA-H1b — SECONDARY: QKV + gate/up fusion

Claim:

> Fusion may improve locality/materialization and enable a better packed-quant pipeline.

Status: **SUPPORTED AS ENABLER, NOT SUFFICIENT PRIMARY MECHANISM**.

Reason: count-only and activation-read bounds are too small to explain the Q3 gap.

### SA-H1c — OPTIONAL: subgroup/cooperative/hardware-adaptive path

Claim:

> Device-specific subgroup or cooperative features may improve the primary dataflow.

Status: **CAPABILITY-CONDITIONAL**.

No implementation may require these features until exact-device zero-science capability evidence exists.

### SA-H1d — DEFERRED: LM-head redesign

The LM head is 19 endpoint dispatches and touches the full output matrix, but SA0 has no decode stage-time attribution proving that it dominates.

Status: **DEFER UNTIL COMPONENT ATTRIBUTION**.

## 10. Fusion legality / semantic boundary matrix

| Candidate | Structural legality | Required semantic invariant | SA0 disposition |
|---|---|---|---|
| Q/K/V projection fusion | High: same normalized input, independent weights/biases, independent outputs | preserve each projection's Q4_K dequant/accumulation semantics and output tolerance; no cross-output arithmetic coupling | ALLOWED_AFTER_PREREGISTRATION |
| gate+up fusion | High: same FFN-normalized input, independent Q4_K weights, outputs meet at SwiGLU | preserve separate gate/up numerical semantics; decode batch=1 must requalify despite prefill precedent | ALLOWED_AFTER_PREREGISTRATION |
| O + residual fusion | Dependent pipeline | preserve O projection before residual addition and exact residual semantics | NOT_FIRST_MECHANISM |
| down + residual fusion | Dependent pipeline | preserve down projection before residual addition | NOT_FIRST_MECHANISM |
| RMSNorm + projection fusion | Possible but numerically sensitive | preserve normalization/reduction semantics and projection inputs | DEFER |
| LM-head segment fusion/layout | Possible but endpoint-sensitive | preserve both segmented weight boundaries and full-vocab greedy logits semantics | DEFER |
| sparse neuron/operator skipping | Changes executed model path | exact dense-model semantics not preserved without new proof | FORBIDDEN_IN_SA-H1 |

## 11. NEXUS interpretation

R4-C and EL-L4 are used only for one general lesson:

> Runtime representation/scheduling gains occur when the implementation removes work that is genuinely unnecessary or amortizes overhead against enough useful payload.

They do **not** establish that dense Qwen contains an active frontier or that Event Ledger should be inserted into ArcLLM.

Direct Event-Ledger transfer remains rejected.

## 12. Falsification and stop conditions

SA-H1 must close before target-model benchmarking if any of the following becomes true:

1. zero-science exact-device capability shows the proposed primary kernel cannot be expressed without changing the frozen model/precision semantics;
2. source-level/component attribution shows generic batch-1 Q4_K/Q6_K projection work is not a material fraction of decode GPU time;
3. a shape-identical component implementation cannot improve the frozen generic decode GEMM under matched conditions;
4. any proposed fusion requires a numerical-order change outside the separately frozen tolerance contract;
5. a best-case component model shows that plausible GEMM improvements cannot move real decode E2E materially;
6. implementation requires sparse skipping, model conversion, new quantization precision or workload changes not covered by SA-H1.

No rescue phase may be opened merely because one component fails.

## 13. Required zero-science capability preflight

Before any successor kernel implementation, a separate preflight specification must query the exact target without loading the model:

- physical device/vendor/device/driver/API identity;
- compute queue support;
- timestamp period/valid bits;
- subgroup size, stages and operation bits;
- subgroup-size-control feature/properties if exposed;
- relevant 8/16-bit storage/arithmetic feature bits;
- max compute workgroup size/invocations;
- max workgroup shared memory;
- memory heap/type properties;
- cooperative-matrix extension + supported properties if exposed.

Output must bind exact driver and probe executable/source SHA.

Absence of optional subgroup/cooperative features does not automatically fail SA-H1a; it only removes those optional implementation paths.

## 14. Finite successor roadmap

```text
SA0-S  specification / causal qualification          COMPLETE
  ↓
SA0-CAP exact-device zero-science capability probe   NEXT
  ↓ PASS
SA1-P  component-study preregistration
       exact kernel shapes
       baseline kernel identity
       semantic tolerances
       performance gate
       resource budget
       stop rule
  ↓ independent QA / implementation lock
SA1-K  one primary batch-1 Q4_K/Q6_K kernel family
  ↓ component qualification
SA1-F  optional QKV / gate+up fusion only if SA1-K passes
  ↓
SA2    fresh exact-model matched confirmation
```

There is no SA1-K implementation authorization in this SA0 commit.

## 15. SA0 adjudication

```text
CAUSAL QUALIFICATION                         PASS
STATIC HARDWARE FEASIBILITY                  PASS
FUSION-ONLY AS PRIMARY EXPLANATION           REJECTED
EVENT-LEDGER DIRECT TRANSFER                  REJECTED
SA-H1a PACKED-QUANT DATAFLOW                 QUALIFIED
SA-H1b FUSION                                SECONDARY_ONLY
SA-H1c OPTIONAL DEVICE FEATURES              EXACT-DEVICE-PREFLIGHT_REQUIRED
EXACT-DEVICE CAPABILITY                      NOT YET MEASURED
SUCCESSOR IMPLEMENTATION                     NOT AUTHORIZED
TARGET MODEL EXECUTION                       NOT AUTHORIZED
```

Overall SA0 status:

> **SA0_SPECIFICATION_QUALIFIED_CAPABILITY_PREFLIGHT_REQUIRED**

This is enough to justify the next zero-science exact-device capability step. It is not enough to write a new inference kernel.
