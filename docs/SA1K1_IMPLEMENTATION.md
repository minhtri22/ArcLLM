# SA1-K1 Implementation

**Date:** 2026-09-21  
**Parent implementation lock:** `60b0fdf91ffe859c918055afa9e3b1071142f6c4`  
**Scope:** Q4 only.

## Implemented mechanism

Exactly one successor shader exists: `shaders/sa1_q4k_subgroup_splitk.comp`.

The shader keeps the frozen direct-packed Q4_K format and W/X/B/Y interface. A 128-thread workgroup is forced to four full 32-lane subgroups by pipeline creation. Each subgroup owns one output row; lanes stride K by 32, accumulate FP32 partial sums, use subgroup arithmetic reduction, and lane 0 writes the row.

There is no shared memory, global scratch, pre-dequantization, cooperative matrix, fusion, staging, subgroup-size variant or geometry search.

## Component harness

`src/sa1_component_benchmark.cpp` has two modes.

- `preflight`: banks 0 and 3, all five frozen Q4 shapes, baseline/candidate/CPU correctness only. It creates **zero timestamp queries**, records zero measured pairs and never evaluates the performance gate.
- `measure`: the already-preregistered A/B timing protocol. This mode requires an external execution-authorization file and is unreachable through the preflight runner.

The harness loads no GGUF/model data.

## Build identity

Both the frozen P7 baseline shader and the candidate are compiled with pinned glslang 16.5.0 targeting Vulkan 1.2. Baseline SPIR-V must reproduce `2EFD94ACDA45555AF1C082C916AF7C3F1AA1BE46AD7868B7C4BE868816CAEE4A` or BuildOnly fails.

Native build uses the existing Vulkan SDK 1.4.357.0 path when available.

## Current boundary

Implementation does **not** authorize measurement.

The next target-local step is `run_sa1_component_preflight.ps1`, which performs static QA, exact hardware/driver/AC qualification, shader compile, native BuildOnly and correctness-only dispatches. The returned ZIP must be independently adjudicated before `config/sa1_q4_execution_authorization.json` may exist.
