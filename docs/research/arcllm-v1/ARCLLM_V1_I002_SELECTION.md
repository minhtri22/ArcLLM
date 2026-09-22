# ArcLLM v1 — I002 Selection: Q4 FFN Gate/Up Subgroup32 Split-K Decode Executor

**Status:** SELECTED FROM I001R + SA1 CROSS-EVIDENCE / IMPLEMENTATION NOT YET AUTHORIZED

## 1. Intervention

`I002-Q4-GU-SG32`

> Replace only the 56 batch-1 decode `ffn_gate` / `ffn_up` Q4_K matvec dispatches with the previously validated subgroup32 split-K execution mechanism, then measure real-model correctness and carry-through.

## 2. Why I002 is selected

Exact I001R 7B profile:

`ffn_gate_up median device-chain share = 59.6727%`

This dominates every other measured family and replicates across both sessions, both workloads, and all three context positions.

SA1 independently tested the exact component geometry:

`Q4_H3584_R18944_NOBIAS`

and obtained:

```text
5.33376×  process A
5.50431×  process B
correctness PASS
```

using one fixed mechanism:

`SUBGROUP32_SPLIT_K_PER_OUTPUT_ROW`.

This is direct mechanism evidence against the exact shader family that I001R now identifies as dominant.

## 3. Causal mechanism

Current baseline:

```text
local_size = 64
one invocation owns one output row
one invocation serially scans all 3584 K values
```

SA1 candidate:

```text
local_size = 128
4 subgroups/workgroup
1 subgroup (32 lanes) owns one output row
K dimension split across 32 lanes
subgroupAdd reduction
lane 0 stores output
```

I002 is therefore a work-partitioning intervention, not generic "FFN optimization".

## 4. Scope

Allowed initial delta:

- decode only;
- `ffn_gate`;
- `ffn_up`;
- Q4_K only;
- exact 28-layer 7B model;
- exactly 56 node substitutions per token;
- unchanged semantic graph and dispatch census unless explicitly preregistered otherwise;
- unchanged prefill;
- unchanged attention/QKV/O projection;
- unchanged FFN-down;
- unchanged LM-head;
- unchanged quantization/model.

## 5. Why gate/up fusion is not the primary mechanism

Gate + up logical weights per token:

`2,138,701,824 bytes`

Maximum eliminated duplicate FP32 input read from fusing gate/up:

`401,408 bytes/token`

That is about 0.019% of the gate/up weight payload.

Lifecycle and barrier shares are also below 0.1%.

Therefore simple launch reduction or shared-input fusion cannot explain a multi-x target. The evidence-supported mechanism is subgroup cooperative K partitioning.

## 6. Expected carry-through

With exact I001R share `f=0.5967268`:

| Gate/up speedup | Device-chain Amdahl envelope |
|---:|---:|
| 2× | 1.425× |
| 3× | 1.661× |
| 4× | 1.810× |
| SA1 A 5.334× | 1.941× |
| SA1 B 5.504× | 1.954× |
| infinite | 2.480× |

These are predictions, not acceptance thresholds.

## 7. First-stage requirement

Before a full confirmatory performance run, I002 must have a frozen real-model transfer stage that verifies:

1. exact candidate shader provenance from SA1 or an explicitly justified derivative;
2. exact gate/up real-weight binding;
3. exact real activation input;
4. output correctness against the current production gate/up path on sampled layers/tokens;
5. no changes outside the 56 allowed decode nodes;
6. exact model-level token/hash semantic guard;
7. TTFT measured as a non-regression guard even though prefill is unchanged;
8. candidate/reference paired measurements with counterbalanced order;
9. no threshold changes after outcome exposure.

## 8. Failure interpretation

If SA1 component speedup does not transfer to real gate/up:

- this is evidence of fixture/model-context transfer failure;
- do not tune subgroup size/local size/tile under I002;
- update Headroom Map and reopen mechanism attribution.

If gate/up improves locally but decode/E2E does not:

- classify as carry-through failure;
- inspect the newly dominant family before any further optimization.

## 9. Next artifact

`ARCLLM_V1_I002_REAL_MODEL_TRANSFER_AND_CARRY_THROUGH_SPEC.md`

No I002 production code is authorized by this selection document alone.
