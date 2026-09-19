# P8-D Implementation — graph binding resolver

Date: 2026-09-19

## Implementation boundary

P8-D adds no decoder kernel and changes no P7-L shader. It introduces one graph-level logical-to-physical binding representation and routes the already-qualified segmented embedding/LM-head endpoint probes through it.

## Resolver

`TensorBindingDescriptor` owns one or more `P8DBindingSlice` records.

- The graph census is generated explicitly from the frozen Qwen2 7B structure: 3 top-level tensors plus 28 blocks x 12 tensors = 339.
- The exact P8-A2 <=256 MiB row-aligned segmentation rule is recomputed independently from GGUF tensor geometry.
- The recomputed arena table must equal the frozen 19-arena table.
- Every graph tensor is resolved exhaustively.
- Only `token_embd.weight` and `output.weight` may resolve to two slices.
- Every other graph tensor must resolve to exactly one slice.
- The resolver requires 341 physical pieces total and proves per-tensor span equivalence plus global contiguous payload ownership through all 4,677,120,000 weight bytes.

## Endpoint execution

All 19 weight arenas are allocated from the frozen target bytes. The two endpoint operations obtain their Vulkan buffers from the graph binding descriptors:
- `bindings.at("token_embd.weight")`
- `bindings.at("output.weight")`

No endpoint buffer is constructed from a direct hard-coded payload offset.

The P8-C shaders are reused unchanged because the four segmented slices begin at arena byte base zero under the frozen P8-A2 plan. P8-D asserts that invariant before dispatch.

## Parent evidence

The runner verifies byte-exact SHA256 for:
- P8-A2 authoritative segment plan;
- P8-C authoritative access results;
- P8-C authoritative summary.

The P8-C shader-provenance raw file was not supplied previously, so P8-D verifies its frozen hash record but does not fabricate parent provenance bytes. P8-D generates fresh shader provenance for the same two unchanged shader sources.

## Execution limit

Exactly two endpoint dispatches in one submit are permitted.

`decoder_layer_dispatches = 0` is an explicit runtime gate.

No prefill, decode, KV-attention, generation, or full-model inference executes.
