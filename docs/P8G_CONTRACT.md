# P8-G Contract — bounded four-layer prefix correctness

## Parent evidence

P8-F is a genuine PASS on the exact frozen 7B target.

Authoritative P8-F SHA256:
- p8f_shader_provenance: 9686F747B6A4D7380F4621B1A3EEC09B82DE7832461B1A9C80548D21D7D70A41
- p8f_two_layer_results: F172E1B0B00558BDE53EB6994BDC1E5BFFC417F96A33FA1C53B0983BB494AE98
- p8f_summary: 1F3EF56BC723FE2D8D8E567ADCB89BC23AE212CB0BBBB51CBC5B83E81DF9FC70

P8-F proved direct GPU composition across layer 0 -> layer 1 with 34/34 checkpoints PASS, exact 30 dispatches, one submit, exactly two executed layers, and no embedding/LM-head/decode/generation path.

## Scientific question

Does the already validated two-layer prefix remain numerically correct when prefix depth is doubled to four consecutive decoder layers, while every inter-layer hidden state remains GPU-resident and unmodified?

## Single hypothesis

The frozen decoder path remains compositionally stable across a bounded four-layer prefix.

Relative to P8-F, the only new variable is prefix depth: layers [0,1] -> layers [0,1,2,3]. Sequence length, input, kernels, quantized weights, graph binding, arena plan, numerical gates and submission model remain unchanged.

## Frozen execution scope

Execute exactly layers [0,1,2,3].

Architecture:
- model layers: 28
- executed layers: [0,1,2,3]
- executed layer count: 4
- hidden: 3584
- q_heads: 28
- kv_heads: 4
- head_dim: 128
- kv_dim: 512
- ffn: 18944
- max_ctx: 4096
- sequence length: 4
- position base: 0

Initial input is exactly the P8-E/P8-F deterministic synthetic hidden state.

All three GPU layer boundaries must be direct: L0 output -> L1 input -> L2 input -> L3 input. No CPU correction, host-side replacement, intermediate re-upload, or expected-value injection is permitted.

All 19 frozen weight arenas remain resident. Every tensor used by layers 0..3 must resolve through the P8-D binding resolver and must be single-piece.

## Per-layer operation chain

Each layer executes the frozen 15-operation P8-E/P8-F chain unchanged:
1. attention RMSNorm
2. Q projection
3. K projection
4. V projection
5. Q RoPE
6. K RoPE
7. KV store
8. causal GQA
9. attention output projection
10. attention residual
11. FFN RMSNorm
12. fused P7-L gate+up
13. SwiGLU
14. P7-G FFN down
15. FFN residual

P8-G therefore executes exactly 60 compute dispatches in one submit.

No embedding, output_norm, LM-head, decode step, token sampling, generation, layer 4+, or performance trial is permitted.

## Independent CPU reference

CPU reference computes the same four-layer prefix sequentially from original GGUF tensor bytes:
CPU L0 -> CPU L1 -> CPU L2 -> CPU L3.

GPU layer outputs must never construct CPU expected values. CPU outputs must never be injected into GPU execution.

## Correctness observations

Record the same 17 checkpoints per layer, 68 total.

For every checkpoint:
- finite values required;
- max_abs <= 0.02;
- RMSE <= 0.005.

Mandatory gates:
- all 68 checkpoint records PASS;
- all four final layer outputs PASS;
- K/V rows for positions 0..3 PASS for all four layers;
- direct GPU handoff across all three inter-layer boundaries;
- exact 60 dispatches;
- exact one submit;
- exactly four executed layers [0,1,2,3].

## Failure interpretation

- package/build/compiler/environment failure -> infrastructure only;
- binding/format mismatch before dispatch -> integration obstruction;
- failure in L0 or L1 -> P8-F regression;
- L0/L1 PASS and first failure in L2 or L3 -> genuine deeper-prefix composition failure localized to that checkpoint;
- no gate may be relaxed after evidence.

## Next

PASS -> P8-H bounded eight-layer prefix correctness design.

FAIL -> adjudicate the first failing P8-G checkpoint.

Full 28-layer prefill/decode/generation remains forbidden after P8-G PASS.
