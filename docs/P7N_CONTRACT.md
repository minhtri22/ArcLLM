# P7-N Contract — fuse gate+up+SwiGLU A/B

P7-M froze the post-P7-L attribution: prefill ffn_gate_up remains the largest family at 44.4034% of device chain time, ffn_down is 30.4215%, and barrier/unattributed is only 0.0243%.

## Single hypothesis
Starting from the exact P7-L winner, compute SwiGLU inside the fused Q4_K gate+up kernel and write final s directly. Eliminating gate/up intermediate writes, their later reads, and one SwiGLU dispatch per layer will improve end-to-end pp512 wall time.

## Frozen scope
- Baseline: exact P7-L winner, 441 prefill dispatches.
- Optimized: exact same row8 x token16 x K32 gate/up math, but applies SiLU(gate)*up before store; 413 prefill dispatches.
- FFN down remains exact P7-G tile16 Q4_K/Q6_K.
- Attention projections remain tile8.
- Attention, RoPE, KV, RMSNorm, residuals, LM head and decode remain unchanged.
- Decode remains 469 dispatches and does not use the P7-N fused shader.

## Correctness gates
- Real layer-0 Q4_K fused final-SwiGLU output: max_abs <= 0.02, RMSE <= 0.005 versus CPU reference.
- Inherited kernel regressions PASS.
- Three live pp512 A/B trials: finite logits, max_abs <= 0.02, RMSE <= 0.005, exact top1.

## Performance gate
Three interleaved live pp512 trials. PASS requires baseline_median_wall / optimized_median_wall >= 1.10x. Historical throughput is context only.

P7 remains OPEN regardless of outcome.
