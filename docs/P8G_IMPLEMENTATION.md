# P8-G Implementation — bounded four-layer prefix correctness

Date: 2026-09-19

## Scope

P8-G extends the already validated P8-F execution path from two consecutive decoder layers to exactly four consecutive layers: 0, 1, 2 and 3.

No kernel, quantization, memory-plan, numerical gate, sequence length, graph-binding rule or submission model is changed.

## CPU reference

The independent CPU reference uses original GGUF bytes and computes the exact chain:

CPU L0(input) -> CPU L1(L0) -> CPU L2(L1) -> CPU L3(L2).

GPU intermediates never construct CPU expected values.

## GPU composition

The same frozen 15-operation per-layer builder from P8-F is retained.

It is invoked explicitly for:
- layer 0 with the original deterministic input buffer;
- layer 1 with gpu[0].out;
- layer 2 with gpu[1].out;
- layer 3 with gpu[2].out.

All three boundaries are therefore direct GPU hidden-state handoffs. No CPU correction, host replacement, intermediate re-upload or second submit is inserted.

The final prepared chain contains exactly 60 dispatches and is executed with one submit.

## Binding and residency

The P8-D/P8-F resolver is reconstructed unchanged:
- 339 logical tensors;
- 19 resident weight arenas;
- 341 physical pieces;
- two globally segmented logical tensors.

Every weight tensor used by executed layers 0..3 must resolve to exactly one physical slice.

## KV and checkpoint evidence

K/V cache storage is allocated for four layer regions. KV store receives the actual layer index, and rows 0..3 are independently read from each layer region after execution.

Each layer retains its own intermediate buffers, preserving the same 17 observations from P8-E/P8-F.

Total evidence:
- 17 checkpoints per layer;
- 68 checkpoint records;
- max_abs <= 0.02;
- RMSE <= 0.005;
- finite values required.

## Failure localization

If L0 or L1 fails, P8-F has regressed.

If L0/L1 pass and the first failure occurs in L2 or L3, that is the new bounded deeper-prefix composition obstruction.

No full 28-layer inference, embedding, output norm, LM-head, decode, generation or performance trial is constructed.
