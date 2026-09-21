# SA1 Closeout / Synthesis

**Date:** 2026-09-21  
**Final state:** `SA1_CLOSED_ASYMMETRIC_Q4_PASS_Q6_CORRECTNESS_FAIL`

## Preregistered question

Does one fixed subgroup-cooperative split-K dataflow materially accelerate exact batch-1 packed Q4_K
decode GEMM shapes while preserving the numerical contract, and if so does the same unchanged
mechanism extend to Q6_K?

## Evidence synthesis

### Q4_K

`Q4_STAGE_PASS` is closed evidence. Process A and B geometric-mean component speedups were
approximately `3.13715x` and `3.14750x`; the minimum frozen-cell speedups were approximately
`1.65942x` and `1.67147x`. Correctness and both frozen performance gates passed independently.

Conclusion: mechanism efficacy is supported for the frozen Q4_K component family. This remains a
component-level result, not an end-to-end model-speed claim.

### Q6_K

The exact same primary mechanism and frozen geometry compiled and built successfully, but the first
valid zero-measurement preflight failed at `candidate_cpu` under the frozen numerical contract.
Performance measurement was never authorized or run; measured pairs are zero; the target model was
not loaded.

Conclusion: the unchanged SA1 mechanism does not generalize to Q6_K under the frozen correctness
contract. This is a correctness boundary, not a Q6 performance result.

## Final scientific disposition

SA1 supports two different statements:

1. `SUBGROUP32_SPLIT_K_PER_OUTPUT_ROW` is effective for the frozen Q4_K component family.
2. Cross-quant generality of that unchanged mechanism from Q4_K to Q6_K is falsified within SA1.

SA1 does not establish end-to-end applicability. It also does not support the broader claims that
subgroup split-K never works or that Q6 is slow.

SA1 is now formally closed. Q4/Q6 reruns, Q6 rescue/tuning, Q6 timing, target-model integration from
SA1, and Q3 reopen remain forbidden.

## Single next scientifically valid step

Open a **new, specification-only research program** for the Q6 correctness boundary. Its question is
whether reduction topology / numerical conditioning causally explains the boundary or whether another
mechanism does. The new program must preregister alternatives, falsification gates, and fresh-data
rules before any new execution. SA1 itself remains immutable.
