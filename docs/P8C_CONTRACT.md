# P8-C Contract — segmented embedding/LM-head GPU access correctness

## Parent evidence

P8-B PASS establishes that the exact P8-A2 plan can be simultaneously resident on the target Arc 140V. Full weight byte compare, KV allocation, working-buffer allocation and memory-budget gates all pass.

Authoritative P8-B SHA256:
- p8b_residency_results.json: 127E25B9CA8F8B4C74CDEBAFB6559CAFF078EFFE7F1513BD1A7429FD0E69B3B2
- p8b_summary.json: 078FE23DF5B31B3D69162BADC6285D608F0EB1178430E38A56DD0FF827EC2B5C

## Question

Can GPU kernels address the two-segment vocab tensors correctly across their exact segment boundaries and reproduce CPU reference values from the original GGUF bytes?

## Frozen probes

Embedding: token_embd.weight, Q4_K, hidden=3584, row_bytes=2016, boundary row=133152. Probe token rows: 0,1,133150,133151,133152,133153,152062,152063.

LM head: output.weight, Q6_K, hidden=3584, row_bytes=2940, boundary row=91304. Probe vocab rows: 0,1,91302,91303,91304,91305,152062,152063.

Both probes explicitly include boundary-1 and boundary.

## GPU path

Exactly two compute dispatches are allowed:
1. segmented Q4_K embedding probe: binds both embedding segments simultaneously, selects segment from global token row, converts to local row, and dequantizes the full 3584-element row;
2. segmented Q6_K LM-head probe: binds both output segments simultaneously, selects segment from global vocab row, converts to local row, and computes a full 3584-element dot product against one deterministic FP32 input vector.

CPU references are read from the original mapped GGUF tensor bytes, not from resident GPU output.

Before either dispatch, the executable must independently verify mapping equivalence for every preregistered probe row:

```text
segment_source_offset + local_row * row_bytes
==
tensor_source_offset + global_row * row_bytes
```

This pre-dispatch address-translation gate is independent of the numerical CPU↔GPU comparison.

## Correctness gate

- pre-dispatch mapping equivalence passes for all selected embedding and LM-head rows;
- all outputs finite;
- embedding compares all 8*3584 = 28672 values;
- LM-head compares all 8 selected logits;
- max_abs <= 0.02 for each probe;
- RMSE <= 0.005 for each probe;
- exact two GPU dispatches in one submit;
- boundary-adjacency probes present exactly as preregistered.

No performance gate. P8-C does not run transformer layers, KV attention, prefill or decode.

PASS permits only P8-D graph integration. Full 7B inference remains forbidden.