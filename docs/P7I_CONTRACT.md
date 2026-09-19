# P7-I Contract — FFN token tile32 A/B

P7-H measurement is frozen PASS. P7-I tests exactly one prefill optimization hypothesis: increase only FFN gate/up/down packed-GEMM token reuse from tile16 to tile32.

## Frozen baseline
- baseline is the P7-G winner: attention projections tile8, FFN tile16.
- causal attention, RoPE, KV store, RMSNorm, residuals, SwiGLU and LM head are unchanged.
- decode path is unchanged from P7-A and is not tiled.
- all 338 weights and KV remain resident; packed Q4_K/Q6_K remain source-of-truth.

## Optimized arm
Only the 84 prefill FFN GEMM dispatches change from token tile16 to token tile32. Workgroup remains 8x8; each lane computes four token outputs.

## Correctness gates
- inherited q4/q6, prefill-attention and cached-attention regressions must PASS.
- tile32 Q4_K and Q6_K regressions use batch=25 to exercise all four per-lane token accumulators plus a tail.
- tile32 regression max_abs <= 0.02 and RMSE <= 0.005.
- every live pp512 A/B trial must preserve exact top-1 and logits max_abs <= 0.02 / RMSE <= 0.005.

## Performance gate
Three interleaved pp512 A/B trials. PASS requires live same-run median wall speedup >= 1.10x. Gate is pre-registered and must not be changed after execution. tg128 is unchanged and run only as non-regression.

P7 remains OPEN regardless of P7-I outcome.