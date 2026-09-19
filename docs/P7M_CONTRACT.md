# P7-M Contract — post-P7-L timestamp re-profile

P7-L is frozen as a genuine PASS: fused gate+up achieved 1.242926134x same-run median pp512 speedup with exact top1/logits agreement.

P7-M is measurement only.

## Exact graph
- Prefill: exact P7-L winner.
- FFN gate+up: one fused Q4_K dispatch per layer, row8 x token16 x K32.
- FFN down: frozen P7-G tile16 Q4_K/Q6_K.
- Q/K/V/O projections: frozen tile8.
- Attention, RoPE, KV, RMSNorm, SwiGLU, residuals and LM head unchanged.
- Decode: unchanged legacy P7 decode graph; no fused/tiled FFN shader in decode.

## Measurement
Timestamp every prefill dispatch and every dispatch in one cached decode step. Report whole-chain ticks, summed dispatch ticks, barrier/unattributed ticks, category shares, and top individual ops.

## Gate
No throughput threshold. PASS requires inherited regressions plus fused gate/up regression PASS, supported timestamps, finite outputs, and nonzero chain/per-dispatch ticks. P7-M cannot close P7.

The next optimization family must be selected only from P7-M attribution.
