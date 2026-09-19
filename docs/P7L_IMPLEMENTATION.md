# P7-L Implementation

The runtime descriptor path is generic in buffer count, so gate+up fusion requires no Vulkan runtime plumbing change.

The new Q4_K shader binds gate weights, up weights, one shared activation input, and two outputs. It keeps the proven P7-G 8-row x 16-token x K32 geometry and 8x8 workgroup. For each K32 tile it dequantizes both weight tiles, loads the 16x32 activation tile once, and accumulates gate/up for both token halves. FFN down remains the frozen P7-G tiled kernel.

Optimized prefill dispatch count is baseline minus 28: each layer replaces two GEMM dispatches with one fused dispatch.
