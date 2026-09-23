# ArcLLM v1 — I002 Bounded Implementation Package

**Date:** 2026-09-23  
**Intervention:** `I002-Q4-GU-SG32`  
**Status:** STATIC IMPLEMENTATION COMPLETE / LOCAL ZERO-SCIENCE BUILD PREFLIGHT REQUIRED / SCIENCE LOCKED

## 1. Bounded implementation

The package reuses the exact historical SA1 candidate:

`shaders/sa1_q4k_subgroup_splitk.comp`

blob:

`56999d88dc1bef6486e7e1908982f6de4b0f9f6a`

historical SPIR-V SHA256:

`B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569`

No new kernel variant or parameter search was introduced.

The candidate substitution is limited to:

```text
28 decode layers
× {ffn_gate, ffn_up}
= 56 decode nodes/token
```

Prefill and every other decode family remain on the exact Q2-safe path.

## 2. T0 package

T0 consists of:

- exact candidate source/SPIR-V provenance;
- static proof of subgroup32 mechanism identity;
- exact 56-node scope;
- byte-identical prefill builder relative to Q2;
- explicit exclusion of Q/K/V, O projection, FFN-down and LM-head substitutions;
- historical exact-device subgroup32 capability evidence;
- Windows native build preflight;
- science authorization hard-disabled in the execution lock.

## 3. T1 harness

`src/arcllm_v1_i002_t1_transfer.cpp`

Frozen real-model sample matrix per session/workload:

```text
layers       0 / 13 / 27
decode pos   0 / 15 / 30
operators    gate / up
= 18 comparisons/cell
4 cells
= 72 comparisons total
```

For each target pair:

1. execute upstream baseline state;
2. execute baseline gate/up with per-op Vulkan timestamps;
3. execute exact SA1 candidate gate/up with same input/weights;
4. compare real outputs using max_abs <=0.02 and RMSE <=0.005;
5. restore baseline gate/up result before continuing full-model state.

Arm order alternates, with session B parity reversed.

This lets T1 test both real-model correctness transfer and G1 component speedup without changing downstream model state.

## 4. T2/T3 harness

`src/arcllm_v1_i002_t3_paired.cpp`

T3 builds two otherwise-identical decode templates:

- baseline;
- candidate with only 56 gate/up shader/dispatch substitutions.

Before any measured pair, the runner executes one baseline/candidate full-model guard and requires identical generated token IDs. Failure stops before T3 measurement.

Measured T3 then uses:

```text
A/W-S
A/W-C
B/W-C
B/W-S

5 measured pairs/cell
alternating pair order
20 total pairs
```

Frozen endpoints:
- decode latency/throughput;
- TTFT;
- E2E latency;
- token semantic equality.

## 5. Stage authorization separation

The package intentionally has no science authorization file.

The science runners require:

`config/arcllm_v1_i002_science_authorization.json`

and fail closed if absent.

Further:

- T1 requires `t1_execution_authorized=true`;
- T3 requires `t3_execution_authorized=true`.

Therefore a T1 authorization cannot accidentally authorize T3.

T3 may only be authorized after independent T1 adjudication.

## 6. Current QA state

Static source/provenance QA:

`PASS`

Local Windows compile/build preflight:

`REQUIRED`

Fresh target-model execution:

`NOT AUTHORIZED`

The package cannot be called fully qualified until the zero-science local build preflight returns a valid PASS bundle.
