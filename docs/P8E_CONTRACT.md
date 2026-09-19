# P8-E Contract — bounded single-layer 7B graph correctness bring-up

## Parent evidence

P8-D is a genuine PASS on the exact frozen 7B target.

Authoritative P8-D evidence SHA256:
- p8d_shader_provenance.json: FABD3DAE027DB5AA69E037FE179BCD0C4F5A16C5D3AAFF0E08A938101AC8F454
- p8d_graph_binding_results.json: 53C373BD3BA1A9BD31B45702CED08E2EB39C7F2B7B099E052EC8C5153A824ABD
- p8d_summary.json: 74624BFAC44F4E5B9A6F08AC972508A353DAB96E0B6D4B92E8C658ABB1651D27

P8-D proved the exact 339-tensor graph census, 19-arena / 341-piece physical map, zero missing or ambiguous bindings, exactly two segmented logical tensors, exhaustive span/global coverage equivalence, endpoint numerical correctness, and zero decoder-layer execution.

## Scientific question

Can exactly one Qwen2 decoder layer at the frozen 7B dimensions execute through the P8-D graph-binding resolver using the frozen P7-L production kernels and match an independent CPU reference, without opening multi-layer or generation scope?

## Single hypothesis

The existing P7-L decoder-layer math is dimensionally and semantically valid at the frozen 7B geometry when every layer-0 tensor is obtained through the P8-D binding descriptor.

No kernel optimization, quantization change, tensor rewrite, arena-cap change, context-policy change, or graph restructuring is permitted.

## Frozen execution scope

P8-E executes **layer 0 only**.

Architecture:
- layers in model metadata: 28
- executed layer: 0
- hidden: 3584
- q_heads: 28
- kv_heads: 4
- head_dim: 128
- kv_dim: 512
- ffn: 18944
- max_ctx: 4096
- sequence length: 4
- position base: 0

The input hidden state is synthetic and deterministic, so embedding is excluded from this hypothesis. For flat element index `i` across the 4 x 3584 input:
`x[i] = 0.17*sin((i+11)*0.009) + 0.03*cos((i+5)*0.017)`.

All 19 frozen weight arenas remain resident. Layer-0 weights must be resolved through the P8-D binding descriptor. Direct GGUF payload offsets are allowed only for the independent CPU reference, never for GPU graph binding.

## Frozen layer operation chain

Exactly these semantic operations execute, in this order:

1. attention RMSNorm
2. Q projection
3. K projection
4. V projection
5. Q RoPE
6. K RoPE
7. layer-0 KV store
8. causal GQA attention
9. attention output projection
10. attention residual add
11. FFN RMSNorm
12. fused FFN gate+up using the frozen P7-L kernel
13. SwiGLU
14. FFN down using the frozen P7-G down path
15. FFN residual add

This is exactly 15 compute dispatches in one submit.

No embedding, output_norm, LM-head, decode step, token sampling, generation, second decoder layer, or performance trial is permitted.

## Frozen tensor-format contract

For layer 0:
- attn_norm, ffn_norm and Q/K/V biases: F32;
- attn_q, attn_k, attn_output, ffn_gate and ffn_up: Q4_K;
- attn_v and ffn_down: Q4_K or Q6_K exactly as encoded by the target;
- no dequantized full-weight copy may be introduced.

A target-format mismatch is a pre-dispatch contract obstruction, not a numerical scientific FAIL.

## Independent CPU reference

The CPU path reads the original GGUF tensor bytes directly and independently computes the same layer-0 sequence using:
- packed Q4_K/Q6_K CPU dot/dequant math;
- RMSNorm;
- RoPE;
- causal grouped-query attention;
- residual addition;
- SiLU-gated FFN.

GPU intermediate values must never be used to construct the CPU expected outputs.

## Correctness observations

Record and compare at least these checkpoints:
- attention RMSNorm output;
- Q, K and V projection outputs;
- Q and K after RoPE;
- layer-0 K/V cache rows written for positions 0..3;
- causal attention output;
- attention output projection;
- post-attention residual;
- FFN RMSNorm output;
- gate and up outputs;
- SwiGLU output;
- FFN down output;
- final layer output.

For every floating-point checkpoint:
- finite values required;
- max_abs <= 0.02;
- RMSE <= 0.005.

The final layer output gate is mandatory even if all internal checkpoints pass.

KV store additionally requires the written layer-0 rows to correspond exactly to the same positions 0..3 used by the executed sequence.

## Execution gate

P8-E PASS requires all of:
- exact target SHA/size;
- byte-exact authoritative P8-D evidence;
- exact P8-D 19-arena / 341-piece binding plan;
- exact frozen architecture and sequence parameters;
- all layer-0 required bindings present and unambiguous;
- no layer-0 tensor uses a segmented binding;
- all tensor-format requirements pass before dispatch;
- all listed checkpoint numerical gates pass;
- exact 15 dispatches;
- exact one submit;
- exactly one decoder layer executed;
- executed layer index exactly 0;
- no embedding, LM-head, decode, generation, or performance path executes.

## Failure interpretation

- Package/build/compiler/environment failure -> infrastructure repair only.
- Target-format mismatch before dispatch -> contract/implementation obstruction; do not reinterpret as numerical model failure.
- Binding failure after P8-D authoritative evidence -> graph integration regression; stop before numerical adjudication.
- First numerical checkpoint failure -> genuine bounded single-layer correctness failure localized to that checkpoint; do not continue to more layers.
- Final layer output failure with earlier checkpoints passing -> composition/residual-path failure.

## Next

PASS -> P8-F bounded multi-layer-prefix correctness design.

FAIL -> adjudicate the first failing P8-E checkpoint.

Full 28-layer prefill/decode and generation remain forbidden after P8-E PASS.
