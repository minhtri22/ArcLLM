# P7-I Implementation

P7-I reuses the proven P7-G A/B harness. The baseline arm is upgraded from P7-E/tile8 to the frozen P7-G/tile16 graph. The optimized arm introduces only `p7i_ffn_q4k_tiled32.comp` and `p7i_ffn_q6k_tiled32.comp` for FFN gate/up/down. Attention projections stay on P7-C tile8 kernels and decode stays on the original untiled packed GEMM path.
