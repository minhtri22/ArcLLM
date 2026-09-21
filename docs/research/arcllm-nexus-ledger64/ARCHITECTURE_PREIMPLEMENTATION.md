# ANL64 — Architecture Preimplementation Specification

**Date:** 2026-09-21  
**Architecture family:** ArcLLM + NEXUS / Ledger64  
**Status:** PREIMPLEMENTATION / P2 DESIGN INPUT  
**Implementation:** NOT AUTHORIZED

## 1. Architectural thesis

ANL64 separates runtime planning from numerical execution:

```text
GGUF + hardware capabilities
        ↓
NEXUS-derived planner / control plane
        ↓
immutable execution plan
        ↓
heterogeneous executor bank
        ↓
ArcLLM exact-model Vulkan data plane
```

The primary novelty is not "Ledger64 makes LLM sparse". It is:

> use an explicit, evidence-backed control-plane representation to select and coordinate multiple semantics-preserving executors rather than forcing one universal kernel/dataflow across heterogeneous quant/operator regimes.

## 2. Initial component boundaries

### Control plane

Candidate responsibilities:
- classify tensor/operator by role, quant type and shape;
- select legal executor;
- define dependency order;
- bind immutable descriptors;
- encode residency/prefetch hints;
- expose evidence/provenance identity for the selected plan.

### Ledger64-derived representation candidate

Candidate properties inherited from LD64 semantics:
- 64-wide region/lane descriptors;
- region queue + active mask where a dynamic subset exists;
- direct region/lane indexing;
- no full-universe discovery scan;
- no K sort;
- no payload repack merely to satisfy layout.

Its use is conditional on P2 finding a natural LLM control-plane mapping.

### Data plane

Initial executor families:

```text
Q4_K  → Q4_FAST candidate
        historical Q4 subgroup32 result may seed design

Q6_K  → Q6_SAFE baseline-correct path initially
        no requirement to reuse Q4 mechanism

Attention/KV → exact existing path initially

Other ops → exact existing path initially

Fusion → disabled until separately justified/admitted
```

This lets architecture-level composition be studied without first requiring a new Q6 optimization.

## 3. Why heterogeneous executors are admissible

Prior art establishes multi-backend and workload-dependent selection as a mature systems pattern. ArcLLM's own SA1 evidence also directly warns against a universal unchanged Q4/Q6 mechanism.

Therefore ANL64 does not need to test whether heterogeneous dispatch is possible in principle. It must test whether the exact ANL64 composition is correct and useful.

## 4. P2 questions that must be answered before code

### A. Natural Ledger64 mapping
What exactly occupies one Ledger64 lane/region in ANL64?

The answer must:
- correspond to real useful work;
- preserve dependency semantics;
- avoid metadata larger/more expensive than the represented work;
- avoid a reconstruction pass.

### B. Planner cost
Planning must be either:
- model-load/static-plan cost; or
- bounded per-token cost proven small enough by static upper bound.

A control plane that adds significant per-token discovery is disallowed.

### C. Executor composition
For each operator family, specify:
- executor identity;
- quant/shape applicability;
- correctness reference;
- fallback path;
- provenance label;
- resource/residency requirements.

### D. Static upper bound
Before implementation, freeze an upper-bound model:

```text
T_new =
  T_Q4 / S_Q4
+ T_Q6 / S_Q6
+ T_attention / S_attention
+ T_other / S_other
+ T_control
+ T_transition
```

P2 must show that a plausible, non-outcome-driven parameter envelope can move a material fraction of the previously observed E2E deficit.

Historical Q4 speedup may be used only as bounded prior evidence, not as a guaranteed integrated speedup.

### E. Failure mode
If the upper-bound model cannot justify material E2E movement without assuming unproven speedups for most of the graph, ANL64 stops before implementation.

## 5. Forbidden shortcuts

- claiming Ledger64 performance from LD64-2;
- using NEXUS sparse EL-L4 speedups as LLM speedups;
- making Q4-only success an E2E claim;
- changing GGUF model semantics;
- quantization-contract change without a new explicit program decision;
- silently copying CUDA code and calling it Vulkan transfer;
- opening multiple executor variants and tuning to PASS;
- implementation before P2 architecture QA.

## 6. Planned finite path

```text
P0 ORIGIN/GOVERNANCE          COMPLETE
P1 PRIOR ART/SOURCE REVIEW    COMPLETE
        ↓
P2 ARCHITECTURE + UPPER BOUND
        ↓ PASS only
P3 ZERO-SCIENCE COMPATIBILITY
        ↓ PASS only
P4 BOUNDED IMPLEMENTATION
        ↓
P5 COMPONENT INTEGRATION
        ↓ PASS only
P6 MATCHED E2E CONFIRMATION
        ↓
P7 FINAL ADJUDICATION
```

## 7. Current decision

P2 design is authorized.

Kernel/source implementation, target execution and scientific measurement remain forbidden.
