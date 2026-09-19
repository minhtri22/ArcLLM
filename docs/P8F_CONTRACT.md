# P8-F Contract — bounded two-layer prefix correctness

## Parent evidence

P8-E is a genuine PASS on the exact frozen 7B target.

Authoritative P8-E evidence SHA256:
- p8e_shader_provenance.json: 73916DE149A413B835541B95F01FEAF2B87DDDE03C039C3D1ABC0E2A3A115861
- p8e_single_layer_results.json: 992A986081FAFC81AC2E6E1A38063384434DE3138CAE469A04A57FED57BDA52B
- p8e_summary.json: EBC4C8088B0992A30D72973DC7485CCF7AA0618B354509A506CC9FDE294B5B9D

P8-E proved layer-0 execution at the frozen 7B geometry through the P8-D binding resolver. All 17 CPU-vs-GPU checkpoints passed, including layer-0 K/V cache rows and final layer output. Exact execution was 15 dispatches, one submit and one executed decoder layer.

## Scientific question

Does correctness remain within the same frozen numerical gates when the output of decoder layer 0 is consumed directly as the input of decoder layer 1, with both layers executed through the same P8-D graph-binding resolver and frozen P7-L/P7-G kernels?

## Single hypothesis

The validated single-layer path composes correctly across one real decoder-layer boundary.

The only new variable relative to P8-E is consecutive composition of layers 0 and 1. Sequence length, deterministic input, kernels, quantized weights, numerical gates, arena plan and graph-binding semantics remain unchanged.

## Frozen execution scope

P8-F executes exactly the prefix **layers 0 and 1**.

Architecture:
- layers in model metadata: 28
- executed layers: [0, 1]
- executed layer count: 2
- hidden: 3584
- q_heads: 28
- kv_heads: 4
- head_dim: 128
- kv_dim: 512
- ffn: 18944
- max_ctx: 4096
- sequence length: 4
- position base: 0

The initial hidden input is byte-for-byte the same deterministic P8-E synthetic input:

`x[i] = 0.17*sin((i+11)*0.009) + 0.03*cos((i+5)*0.017)`.

Layer 1 must consume the GPU output of layer 0 directly. No host-side replacement or CPU-corrected hidden state may be inserted between layers.

All 19 frozen weight arenas remain resident. Every required tensor for layers 0 and 1 must resolve through the P8-D binding descriptor. Direct GGUF payload offsets are allowed only for the independent CPU reference.

## Frozen per-layer operation chain

Each executed layer performs exactly the same 15 semantic operations frozen by P8-E:

1. attention RMSNorm
2. Q projection
3. K projection
4. V projection
5. Q RoPE
6. K RoPE
7. KV store for that layer
8. causal GQA attention
9. attention output projection
10. attention residual add
11. FFN RMSNorm
12. fused FFN gate+up using the frozen P7-L kernel
13. SwiGLU
14. FFN down using the frozen P7-G path
15. FFN residual add

Therefore P8-F executes exactly **30 compute dispatches in one submit**.

No embedding, output_norm, LM-head, decode step, token sampling, generation, layer 2+, or performance trial is permitted.

## Tensor-format contract

For both layers 0 and 1:
- attn_norm, ffn_norm and Q/K/V biases: F32;
- attn_q, attn_k, attn_output, ffn_gate and ffn_up: Q4_K;
- attn_v and ffn_down: Q4_K or Q6_K exactly as encoded by the frozen target;
- no dequantized full-weight copy may be introduced.

Format is validated independently for each layer before dispatch.

## Independent CPU reference

The CPU path reads the original GGUF tensor bytes directly and computes layers 0 then 1 sequentially.

Critically:
- CPU layer 1 consumes CPU layer-0 output;
- GPU layer 1 consumes GPU layer-0 output;
- GPU intermediates must never be used to construct CPU expected values;
- CPU outputs must never be injected into the GPU prefix.

This preserves a true end-to-end two-layer composition test.

## Correctness observations

Record the same 17 observations for **each layer**, for 34 total checkpoint records:
- attention RMSNorm;
- Q/K/V projections;
- Q/K after RoPE;
- K/V cache rows at positions 0..3 for that layer;
- causal attention;
- attention output projection;
- post-attention residual;
- FFN RMSNorm;
- gate/up;
- SwiGLU;
- FFN down;
- final layer output.

For every floating-point checkpoint:
- finite values required;
- max_abs <= 0.02;
- RMSE <= 0.005.

Mandatory prefix gates:
- layer-0 final output PASS;
- layer-1 final output PASS;
- both layers' K/V rows PASS;
- layer-1 input is the unmodified layer-0 GPU output.

## Execution gate

P8-F PASS requires all of:
- exact target SHA/size;
- byte-exact authoritative P8-E evidence;
- exact P8-D 19-arena / 341-piece binding plan retained;
- graph binding has no missing or ambiguous tensor for layers 0 and 1;
- all layer-0 and layer-1 weight tensors resolve to one piece;
- all target-format requirements pass before dispatch;
- all 34 checkpoint records pass;
- exact 30 dispatches;
- exact one submit;
- exactly two decoder layers executed;
- executed layer indices exactly [0, 1];
- direct GPU hidden-state handoff from layer 0 to layer 1;
- no embedding, output_norm, LM-head, decode, generation or performance path.

## Failure interpretation

- Package/build/compiler/environment failure -> infrastructure repair only.
- Pre-dispatch tensor-format or binding mismatch -> contract/integration obstruction.
- Layer-0 failure -> P8-E regression; stop and adjudicate before interpreting multi-layer composition.
- Layer-0 PASS followed by first layer-1 checkpoint failure -> genuine cross-layer composition/prefix correctness failure localized to that checkpoint.
- Both internal layer-1 checkpoints PASS but layer-1 final output FAIL -> layer-1 composition/residual-path failure.

No gate may be relaxed after evidence.

## Next

PASS -> P8-G larger bounded-prefix correctness design.

FAIL -> adjudicate the first failing P8-F checkpoint.

Full 28-layer prefill/decode and generation remain forbidden after P8-F PASS.
