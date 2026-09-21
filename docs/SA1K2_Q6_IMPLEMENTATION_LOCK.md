# SA1-K2 Q6 Implementation Lock

**Date:** 2026-09-21  
**Prerequisite:** SA1-K1 Q4 = **Q4_STAGE_PASS** at `accd26471e4abd5bbe9981d49121068e116cc62c`.

Q4 established the frozen subgroup-32 split-K component mechanism on all five Q4 shapes in two independent processes. This lock does not change that mechanism. It only permits the preregistered Q6_K extension.

## Exact Q6 cells

| Cell | n | rows | bias | row bytes | packed weight bytes |
|---|---:|---:|---:|---:|---:|
| Q6_H3584_R512_BIAS | 3584 | 512 | 1 | 2940 | 1,505,280 |
| Q6_H18944_R3584_NOBIAS | 18944 | 3584 | 0 | 15540 | 55,695,360 |

## Frozen mechanism

```text
local_size_x = 128
required subgroup size = 32
full subgroups = required
4 subgroups/workgroup
1 subgroup/output row
lanes split K
direct packed Q6_K dequant
FP32 partial accumulation
subgroup reduction
lane 0 store
```

No cooperative matrix, fusion, staging, shared-memory tiling, pre-dequantization or geometry search is permitted.

## Baseline

```text
source:
shaders/p7_q6k_gemm_2d.comp

Git blob:
a0de99f972db6cd202606aad95fb9eab223639e6

frozen SPIR-V SHA256:
F2267838D099128F233EF30817464658AAD71AAFA3933461FB315FAD10ED3F67
```

## Current authorization

Implementation may now add exactly one candidate:

`shaders/sa1_q6k_subgroup_splitk.comp`

and the minimal Q6 extension to the existing SA1 component harness/build/preflight/adjudication tooling.

Q4 candidate/source/results are immutable.

Q6 **performance measurement remains forbidden**. The required sequence is:

```text
one Q6 implementation
      ↓
static QA
      ↓
compile / BuildOnly
      ↓
correctness-only preflight banks 0 + 3
      ↓
independent evidence adjudication
      ↓
separate execution authorization
      ↓
only then A/B Q6 timing
```

Correctness remains finite + max_abs <= 0.02 + RMSE <= 0.005. A correctness FAIL stops the extension and cannot be rescued through tuning.
