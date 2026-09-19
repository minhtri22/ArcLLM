# P7-J Contract — FFN block-aware vec4 dequant A/B

P7-I is frozen as a genuine performance negative. P7-G FFN token tile16 remains the baseline.

## Single hypothesis
Keeping the proven 8-row x 16-token x K32 FFN tile unchanged, reorganizing packed Q4_K/Q6_K dequantization so each invocation decodes four contiguous weights from shared block metadata / packed words will reduce repeated metadata and byte-extraction work enough to improve end-to-end pp512 wall time.

## Frozen scope
- Baseline FFN: P7-G token-tile16 kernels.
- Optimized FFN: same tile geometry and accumulation order; only packed dequant loader is vec4/block-aware.
- Q/K/V/O prefill projections remain frozen tile8.
- Attention kernel, RoPE, KV, residuals, RMSNorm, SwiGLU and LM head are unchanged.
- Decode is unchanged.
- Whole-model residency, GPU KV, packed Q4_K/Q6_K source bytes remain unchanged.

## Correctness gates
- Real Q4_K FFN regression and first real Q6_K FFN-down regression, batch=9.
- max_abs <= 0.02 and RMSE <= 0.005.
- Three live pp512 A/B trials: finite logits, max_abs <= 0.02, RMSE <= 0.005, exact top1 every trial.

## Performance gate
Three interleaved live pp512 trials. PASS requires baseline_median_wall / optimized_median_wall >= **1.10x**. This threshold is pre-registered and may not be lowered after execution. Historical throughput is context only, not a gate.

P7 remains OPEN regardless of outcome.
