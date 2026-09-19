# P8-F Implementation — bounded two-layer prefix correctness

Date: 2026-09-19

## Boundary

P8-F executes exactly decoder layers 0 and 1 at sequence length 4. It changes only one scientific variable relative to P8-E: consecutive decoder-layer composition.

It does not execute embedding, output norm, LM-head, decode, sampling, generation, layer 2+, or any performance trial.

## Graph binding and residency

The executable reuses the independently reconstructed P8-D logical-to-physical resolver from P8-E:

- exact 339 logical tensors;
- exact 19 weight arenas;
- exact 341 physical pieces;
- exactly two segmented logical tensors globally;
- every tensor used by layers 0 and 1 must resolve to one physical slice.

All 19 weight arenas remain simultaneously resident. GPU weight access uses only the resolver's arena buffer and slice-relative base. Direct GGUF payload pointers are used only by the independent CPU reference.

## Independent CPU prefix

CPU reference starts from the exact P8-E deterministic hidden input and computes CPU L0(input) -> CPU L0 output -> CPU L1(CPU L0 output).

Each layer uses the original packed GGUF bytes and the same independent CPU operations already qualified by P8-E: RMSNorm, packed Q4_K/Q6_K matmul, RoPE, causal GQA, residual addition and SwiGLU.

No GPU intermediate is used to construct CPU expected values.

## Direct GPU handoff

GPU execution is prepared as one 30-dispatch chain and submitted once.

Layer 0 uses the synthetic input buffer. Its fifteenth operation writes gpu[0].out.

Layer 1 is wired explicitly with layer1_input = &gpu[0].out, and every layer-1 operation that consumes the incoming hidden state uses that same buffer. No host read, correction, copy from CPU reference or resubmission occurs between the layers.

Each layer owns separate intermediate/checkpoint buffers so that all 34 observations survive the single submit without being overwritten.

## Exact per-layer chain

A shared operation builder appends exactly 15 dispatches for a requested layer:

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

It is called exactly twice: layer 0, then layer 1. Total execution is exactly 30 dispatches in one submit.

## KV evidence

One K cache and one V cache are allocated for exactly two layer regions at the frozen max context. KV store receives the actual layer index. After execution, rows 0..3 are read from each layer's own cache region and compared against that layer's independent CPU K/V reference.

## Correctness evidence

The runtime records 17 checkpoints per layer, 34 total. Every checkpoint retains the frozen P8-E numerical gates:

- finite values required;
- max_abs <= 0.02;
- RMSE <= 0.005.

The JSON identifies the first failing checkpoint as L0.<name> or L1.<name>.

A layer-0 numerical failure is a P8-E regression. If layer 0 passes and layer 1 first fails, the result localizes the new cross-layer composition obstruction.

## Provenance

The runner verifies all three authoritative P8-E artifacts byte-exact before static QA/build/run. The same 11 frozen shaders are rebuilt with pinned glslang 16.5.0 and fresh P8-F shader provenance.
