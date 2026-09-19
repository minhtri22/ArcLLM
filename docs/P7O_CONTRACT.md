# P7-O Contract — FFN-down K64 A/B

P7-N is frozen as a genuine performance negative: correctness PASS, same-run pp512 speedup 1.051612109x, below the pre-registered 1.10x gate. P7-L remains the baseline.

P7-M attribution shows FFN-down is the second-largest prefill family at 30.42145996% of device chain time.

## Single hypothesis
Keep the exact P7-L graph but double only the prefill FFN-down K tile from 32 to 64. For K=8960 this halves tiled K iterations and internal workgroup barrier pairs from 280 to 140 while keeping row8, token16, local_size 8x8, direct packed Q4_K/Q6_K and output semantics unchanged.

## Frozen scope
- Baseline: exact P7-L winner.
- Optimized: only FFN-down Q4_K/Q6_K kernels use K64.
- Gate+up fusion remains exact P7-L.
- SwiGLU remains a separate frozen dispatch.
- Attention projections, attention kernel, LM head, residency and decode unchanged.
- Baseline and optimized prefill each have 441 dispatches; decode remains 469.

## Correctness gates
Real frozen Q4_K and Q6_K FFN-down regressions, batch=9: max_abs <= 0.02 and RMSE <= 0.005. Three live pp512 A/B trials must retain finite outputs, exact top1, logits max_abs <=0.02 and RMSE <=0.005.

## Performance gate
PASS requires median baseline_wall / optimized_wall >= 1.10x across three interleaved live pp512 trials.

P7 remains OPEN regardless of outcome.
