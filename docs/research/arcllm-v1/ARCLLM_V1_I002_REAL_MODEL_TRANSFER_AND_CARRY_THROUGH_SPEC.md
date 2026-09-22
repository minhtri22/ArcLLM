# ArcLLM v1 — I002 Real-Model Transfer and Carry-Through Specification

**Intervention:** `I002-Q4-GU-SG32`
**Parent adjudication commit:** `e4e643262a36e35ef79cb6a190a6a46019dbe39d`
**Status:** FROZEN SPECIFICATION / IMPLEMENTATION NOT YET AUTHORIZED

## 1. Question

Does the already validated SA1 subgroup32 split-K Q4_K mechanism transfer from deterministic component fixtures into the exact 7B model's dominant decode `ffn_gate` / `ffn_up` family, and does that component transfer produce material decode/E2E carry-through without TTFT harm?

This is not a new kernel search. The mechanism and geometry are fixed by SA1.

## 2. Evidence basis

I001R exact 7B: `ffn_gate_up` median device-chain share = **59.6727%**, with all four cell medians between 57.81% and 64.74%.

SA1 exact shape `Q4_H3584_R18944_NOBIAS` against `p7_q4k_gemm_2d.comp`: 5.33376× in process A, 5.50431× in process B, correctness PASS.

## 3. Frozen architecture delta

Allowed change: decode only; 28 layers; `ffn_gate` + `ffn_up`; 56 nodes/token; Q4_K only; replace `p7_q4k_gemm_2d` with the exact SA1 subgroup32 split-K mechanism.

Forbidden: prefill, Q/K/V, attention output, FFN-down, SwiGLU, RMSNorm, LM-head, model, quantization, KV/generation semantics, subgroup/local-size/tile search, alternate candidate shader.

## 4. Candidate identity

Preferred source: `shaders/sa1_q4k_subgroup_splitk.comp`, historical blob `56999d88dc1bef6486e7e1908982f6de4b0f9f6a`.

Mechanism: subgroup size 32; local_size_x=128; four subgroups/workgroup; one subgroup/output row; K split across 32 lanes; subgroupAdd; lane 0 stores output.

## 5. Stage T0 — static transfer qualification

Before model execution prove: exact baseline shape n=3584, rows=18944, batch=1, no bias; all 56 target nodes are Q4_K; no non-target substitution; buffer/push semantics and output layout match; subgroup32 available; exact candidate hashes bound; all non-candidate runtime/model/shader provenance unchanged.

T0 failure blocks fresh model execution.

## 6. Stage T1 — real-model component correctness transfer

Use real model weights and baseline-produced activations.

Frozen sample matrix: layers {0,13,27}; decode positions {0,15,30}; operators {gate,up}; workloads {W-S,W-C}. Total 36 comparisons.

For each comparison baseline and candidate receive the same frozen activation and exact real tensor binding.

Correctness gates inherited from SA1: all finite; candidate vs baseline max_abs <= 0.02; RMSE <= 0.005.

Any failure => `FAIL_I002_REAL_MODEL_TRANSFER_CORRECTNESS`; no performance rescue.

## 7. Stage T2 — full-model semantic guard

Only after T1 PASS. Exact 7B model; 441 prefill dispatches unchanged; 469 decode semantic nodes/step unchanged; only 56 shader/dispatch-geometry substitutions; finite logits; all 32 generated token IDs candidate == baseline for each paired inference; no CPU model-math fallback; no teacher forcing.

Final hidden/logit hashes are recorded but need not be bit-identical because subgroup reduction changes FP32 accumulation order.

## 8. Stage T3 — paired carry-through measurement

Cells: A/W-S, A/W-C, B/W-C, B/W-S. Per cell: 1 warmup per arm + 5 measured baseline/candidate pairs. Pair order alternates; A begins baseline→candidate, B candidate→baseline. No outlier deletion or selective rerun.

Primary metrics: decode latency and throughput. Secondary: E2E latency, TTFT, candidate gate/up timestamp speedup.

## 9. Frozen performance gates

G1 component transfer: median gate/up component speedup >= 1.50× in every session/workload cell.

G2 decode carry-through: candidate/reference decode latency <= 0.90 in every cell, and global geometric-mean decode speedup >= 1.25×.

The 1.25× aggregate gate is the approximate Amdahl carry-through expected when a 59.67% family receives only the minimum accepted 1.50× component improvement.

G3 TTFT non-regression: candidate/reference TTFT <= 1.10 independently in all four cells.

G4 E2E direction: candidate/reference E2E latency < 1.00 in all four cells. E2E magnitude is descriptive; decode is primary because I002 changes decode only.

## 10. Adjudication

T0 FAIL → INVALID_TRANSFER_PACKAGE
T1 FAIL → FAIL_I002_REAL_MODEL_TRANSFER_CORRECTNESS
T2 FAIL → FAIL_I002_MODEL_SEMANTICS
G1 FAIL → FAIL_I002_COMPONENT_TRANSFER
G1 PASS + G2 FAIL → FAIL_I002_CARRY_THROUGH
G2 PASS + G3 FAIL → FAIL_I002_TTFT_COUPLING
G1/G2/G3 PASS + G4 FAIL → FAIL_I002_E2E_INTEGRATION
All PASS → PASS_I002_REAL_MODEL_CARRY_THROUGH

## 11. No rescue

After fresh outcome exposure: no subgroup/local-size/tile search; no gate+up fusion; no threshold mutation; no workload/session deletion; no switch to ANL64 140-node substitution; no Q/K/O additions; no alternate kernel.

## 12. Why no broader lower-bound study first

I001R localizes the dominant exact-7B family, while SA1 already gives an exact-shape causal intervention with >5× recoverable component cost from work partitioning. The remaining high-value uncertainty is transfer into real model state and system carry-through, not whether the shape is improvable in principle.

## 13. Current authorization

This specification authorizes implementation planning, zero-science static QA design, and exact candidate provenance binding only. It does not authorize fresh target-model execution.

Next implementation package must provide: 56-node-only integration; T0 proof; T1 real-model transfer harness; paired T3 runner; exact-head lock; zero-science QA. Fresh I002 execution may be authorized only after those pass.
