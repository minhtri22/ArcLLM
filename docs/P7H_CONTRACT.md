# P7-H Contract — post-FFN-token-tile16 GPU re-profile

P7-G is frozen as PASS. P7-H is measurement only.

## Frozen optimized graph
- pp512 keeps P7-E tiled packed GEMM for Q/K/V/O projections at token tile 8.
- pp512 keeps P7-G FFN gate/up/down token tile 16.
- causal attention kernel, RoPE, KV store, RMSNorm, residuals, SwiGLU and LM head are unchanged.
- decode graph is unchanged from P7-A onward.
- all 338 weights and KV remain resident; packed Q4_K/Q6_K remain source-of-truth.

## Measurement
Timestamp every dispatch plus whole-chain ticks. Report semantic categories, top individual operations and barrier/unattributed share for one optimized pp512 pass and one unchanged cached-decode step.

## Gate
PASS requires inherited kernel regressions, supported timestamps, finite outputs and nonzero device timing evidence. No throughput threshold is applied. P7 remains OPEN. The only allowed decision after P7-H is selecting exactly one next bottleneck family from the measured graph.
