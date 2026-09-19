# P7-L Contract — fused FFN gate+up A/B

P7-K is frozen as a genuine performance negative: row16 preserved correctness but achieved 1.068446103x, below the pre-registered 1.10x gate. P7-G row8/token16 remains the baseline.

## Single hypothesis
Fusing the two Q4_K FFN gate and up GEMMs into one dispatch per layer, while sharing the same activation tile load, will reduce duplicated activation traffic, barriers and dispatch count enough to improve end-to-end pp512 wall time.

## Frozen scope
- Baseline: exact P7-G FFN row8 x token16.
- Optimized: gate+up only are fused; row8, token16, K32 and local_size 8x8 remain unchanged.
- FFN down remains exact P7-G tile16 Q4_K/Q6_K.
- Attention projections remain frozen tile8.
- Attention, RoPE, KV, RMSNorm, SwiGLU, residuals, LM head and decode remain unchanged.
- 56 gate/up GEMM dispatches become 28 fused dispatches; no other semantic op changes.

## Correctness gates
- Real layer-0 Q4_K fused gate and up outputs each: max_abs <= 0.02, RMSE <= 0.005.
- Inherited kernel regressions PASS.
- Three live pp512 A/B trials: finite logits, max_abs <= 0.02, RMSE <= 0.005, exact top1.

## Performance gate
Three interleaved live pp512 trials. PASS requires baseline_median_wall / optimized_median_wall >= 1.10x. Historical absolute throughput is context only.

P7 remains OPEN regardless of outcome.
