# ANL64 P1 — Prior-Art and Source Review

**Date:** 2026-09-21  
**Status:** REVIEWED / IMPLEMENTATION STILL BLOCKED

## Purpose

ANL64 does not re-prove mechanisms already well established elsewhere. It instead separates:

```text
mechanism established elsewhere
        ↓
transfer assumptions
        ↓
Intel Arc / Vulkan compatibility
        ↓
ANL64 local effect
```

Only the final two require new local evidence when hardware/model/runtime assumptions differ.

## Findings

### 1. Static universal decode dataflow is not a justified default

FlashDecoding++ identifies flat autoregressive GEMM under-utilization and shape/hardware-dependent dataflow as first-class inference problems. This supports the architecture principle that one static executor need not be optimal across all decode shapes.

This aligns with ArcLLM's own evidence: Q4 and Q6 did not support one unchanged split-K mechanism.

**Disposition:** `TRANSFERABLE_INVARIANT`; local Arc/Vulkan effect still required.

### 2. Quantized GEMM should co-design layout, dequantization and work partitioning

Marlin demonstrates an aggressively co-designed low-bit matmul path: asynchronous/global loading, double buffering, dequant/compute overlap, offline layout reshaping, geometry-aware work partitioning and latency hiding.

QServe/OmniServe independently reinforces the same high-level lesson: low-bit arithmetic alone is insufficient when dequantization/layout overhead dominates.

**Disposition:** mechanism family is established. ANL64 should not re-prove that such co-design is useful in principle. It must test which subset is legal/effective for Q4_K/Q6_K on Intel Arc Vulkan.

### 3. Multi-backend / conditional selection is an established runtime architecture pattern

FlashInfer exposes multiple optimized kernel backends and workload-specific execution. vLLM similarly treats fusion as conditional on hardware and workload rather than universally profitable.

**Disposition:** ANL64 may directly adopt the architectural pattern of a planner selecting among legal executors. It does not need a scientific experiment proving that software can dispatch different kernels for different operator classes.

The scientific question is whether the selected ANL64 composition improves the exact target workload.

### 4. Vulkan quant-aware source exists and should be used as a reference

llama.cpp's Vulkan `mul_mat_vec.comp` is a source-verified example of quant-aware dequant/vectorized matrix-vector execution.

**Disposition:** use as reference/baseline prior art. Do not treat its implementation as automatically optimal, and do not copy performance claims.

### 5. Event Ledger has positive NEXUS evidence, but its speedup mechanism is sparse-frontier specific

EL-L4 established that direct authoritative frontier maintenance can avoid full-N discovery and deliver fresh E2E savings in tested sparse NEXUS regimes.

This is useful to ANL64 only where an analogous dynamic work-selection problem actually exists.

**Disposition:** the *representation/control-plane principle* is transferable. The measured sparse speedup is not.

### 6. Ledger64 semantics are admissible; Ledger64 performance is not

LD64-0 established a region queue + 64-bit mask representation that avoids full-N/full-R scan, K sort and payload repack. LD64-1 established semantic equivalence for the tested QE/QA/RE/RA representations.

LD64-2 performance remained scientifically `UNRESOLVED` because preregistered measurement sanity failed.

**Disposition:**
- region/lane representation and semantic construction: `TRANSFERABLE_INVARIANT`;
- dense-LLM speed benefit: **NOT ESTABLISHED**;
- any ANL64 use of Ledger64 must first identify what object the 64-lane regions represent (operators, tensor tiles, execution descriptors, or another locally justified unit).

## P1 synthesis

The prior art and internal evidence jointly support a new architecture hypothesis:

```text
do not force one universal quant executor

planner/control plane
        ↓
operator/quant/shape classification
        ↓
choose one of multiple semantics-preserving executors
        ↓
prepared exact-model data plane
```

NEXUS/Ledger64 is initially admitted as a **control-plane representation candidate**, not as a direct dense-compute replacement.

## What ANL64 does not need to prove again

ANL64 does not need a fresh experiment merely to prove that:

- shape-specific kernels can exist;
- a runtime can select different backends;
- low-bit GEMM can benefit from layout/dequant/pipeline co-design;
- operator fusion can be conditionally selected;
- a region queue/mask representation can preserve the tested Ledger64 semantics.

## What ANL64 still must prove locally

Before any E2E claim:

1. exact mapping from GGUF operator/tensor work to ANL64 control-plane descriptors;
2. semantic preservation of the planner/executor composition;
3. Intel Arc/Vulkan compatibility of every borrowed mechanism;
4. Q4/Q6 correctness under their own executor contracts;
5. cost of planning/metadata/residency management;
6. component effect under frozen local workloads;
7. matched real-model E2E effect.

## Decision

`P1_PRIOR_ART_AND_SOURCE_TRANSFER_REVIEW = COMPLETE`.

Implementation remains blocked until the transfer-admissibility contract and P2 architecture/upper-bound specification are frozen.
