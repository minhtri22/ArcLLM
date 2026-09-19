# P7-O Implementation

P7-O introduces two prefill-only FFN-down kernels:
- p7o_ffn_down_q4k_k64.comp
- p7o_ffn_down_q6k_k64.comp

Both preserve P7-G/P7-L row8 x token16 and workgroup 8x8. K tile changes 32 -> 64. Shared weights grow from 8x32 to 8x64 floats and shared activations from 16x32 to 16x64 floats. Dequantization and accumulation order remain scalar within the K tile; no vec4 dequant, row-tile, token-tile, fusion or decode change is mixed into this experiment.

FFN K=8960 is exactly divisible by 64.
