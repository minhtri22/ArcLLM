# P8-E Implementation — bounded single-layer 7B correctness

Date: 2026-09-19

## Boundary

P8-E executes exactly decoder layer 0 at the frozen 7B geometry and sequence length 4. It does not execute embedding, output norm, LM-head, decode, sampling, generation, a second layer, or any performance trial.

The executable includes the already-qualified P8-C Vulkan/runtime and packed-quant CPU helpers, then independently rebuilds the P8-D 339-tensor logical binding map and exact 19-arena / 341-piece physical plan.

## GPU binding rule

All 19 frozen weight arenas are resident. Every layer-0 tensor must resolve to exactly one P8-D slice. GPU operations receive only the resolved arena buffer plus slice-relative base. Direct GGUF payload offsets are not used for GPU binding.

The two segmented vocab tensors are not part of the executed layer.

## CPU reference

CPU reference values are generated directly from the original mapped GGUF tensor bytes, independently of GPU intermediate buffers. The path uses the existing packed Q4_K/Q6_K CPU math plus RMSNorm, RoPE, causal GQA, residual addition and SwiGLU helpers.

Input is frozen by the P8-E contract:

`x[i] = 0.17*sin((i+11)*0.009) + 0.03*cos((i+5)*0.017)`

for the flat 4 x 3584 hidden input.

## Exact GPU chain

The prepared chain contains exactly 15 dispatches in one submit:

1. attention RMSNorm
2. Q projection
3. K projection
4. V projection
5. Q RoPE
6. K RoPE
7. layer-0 KV store
8. causal GQA attention
9. attention output projection
10. attention residual
11. FFN RMSNorm
12. fused P7-L gate+up
13. SwiGLU
14. P7-G FFN down
15. FFN residual

The V projection and FFN-down shader are selected from Q4_K or Q6_K according to the exact target tensor format. All other frozen format constraints are checked before dispatch.

## Observations

The runtime preserves separate buffers for every checkpoint and compares them to the independent CPU reference with max_abs <= 0.02 and RMSE <= 0.005. It records both K and V cache rows for positions 0..3 and the final layer output.

P8-E PASS additionally requires exactly 15 dispatches, one submit, exactly one executed decoder layer and executed layer index 0.

## Provenance

The runner verifies byte-exact P8-D authoritative evidence before build/run and recompiles the 11 required frozen shader sources with pinned glslang 16.5.0, writing fresh P8-E shader provenance.
