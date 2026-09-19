# P7-K Contract - FFN row-tile16 A/B

P7-J is frozen as a genuine performance negative. P7-G token-tile16 remains the baseline.

## Single hypothesis
Keeping token tile=16, K tile=32 and workgroup=8x8 unchanged, doubling only FFN output-row tile from 8 to 16 will reuse each loaded activation tile across twice as many output rows and improve end-to-end pp512 wall time.

## Frozen scope
- Baseline FFN: P7-G row8 x token16.
- Optimized FFN: row16 x token16, same K32 and same 8x8 workgroup; each lane computes two rows x two tokens.
- Q/K/V/O projections remain frozen tile8.
- Attention, RoPE, KV, RMSNorm, residuals, SwiGLU and LM head unchanged.
- Decode unchanged.
- Packed Q4_K/Q6_K and residency architecture unchanged.

## Correctness gates
- Real Q4_K FFN regression and first real Q6_K FFN-down regression, batch=9.
- max_abs <= 0.02 and RMSE <= 0.005.
- Three live pp512 A/B trials: finite logits, max_abs <= 0.02, RMSE <= 0.005, exact top1 every trial.

## Performance gate
Three interleaved live pp512 trials. PASS requires baseline_median_wall / optimized_median_wall >= 1.10x. Historical throughput is context only, not a gate.

P7 remains OPEN regardless of outcome.
