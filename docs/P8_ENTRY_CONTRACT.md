# P8 Entry Contract — 7B memory-planned runtime

P8 starts only after P7 is closed and the 7B target artifact is frozen.

## Goal

Scale the frozen P7 production architecture to a 7B-class GGUF model under the Intel Arc 140V UMA memory budget without changing quantization semantics.

## Frozen inheritance from P7

- direct packed Q4_K / Q6_K;
- no full weight expansion;
- <=256 MiB weight arenas;
- GPU-resident KV;
- tiled attention projections;
- P7-L fused Q4_K gate+up where tensor types permit;
- P7-G FFN-down tile16;
- correctness-first regression gates;
- provenance and append-only lineage.

## Required before implementation

1. Exact 7B model file selected.
2. SHA256 frozen.
3. GGUF tensor inventory and quantization mix recorded.
4. Resident-weight bytes and arena packing simulated before allocation.
5. KV memory budget computed for the intended max context.
6. Total planned GPU-visible memory checked against the observed target-machine UMA budget with explicit safety margin.

No P8 implementation should silently substitute a different 7B artifact after these are frozen.

## P8 first executable milestone

Memory-plan-only bring-up:
- parse frozen 7B GGUF;
- produce deterministic tensor/arena placement;
- prove no tensor crosses an arena boundary;
- prove planned total residency fits the frozen budget;
- do not run full inference until the memory plan passes.

Performance optimization is not part of the first P8 gate.
