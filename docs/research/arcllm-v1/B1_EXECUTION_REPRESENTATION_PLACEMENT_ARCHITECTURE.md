# B1 — Execution Representation Placement Architecture

**Program:** ArcLLM v1  
**Study:** `B1_EXECUTION_REPRESENTATION_PLACEMENT_ARCHITECTURE`  
**Status:** ARCHITECTURE PROBLEM FORMULATION FROZEN — NO PLACEMENT PERFORMANCE EXECUTION AUTHORIZED  
**Parent:** Q4-down 4-arm causal decomposition

## 1. Architecture state entering B1

ArcLLM now treats the two validated Q4-down interventions as independent execution primitives.

### A — COMPUTE PARALLELISM PRIMITIVE

- Primitive: Split-K32.
- Status: **VALIDATED**.
- Role: **default no-extra-representation execution primitive within the validated Q4-down domain**.
- W-S component latency: 38.373384 ms/token versus prior arm-0 100.581013 ms/token.
- W-C component latency: 37.835155 ms/token versus prior arm-0 213.333823 ms/token.
- This default is scoped to the frozen model/operator/shape/quant/hardware regime; it is not a universal transfer claim.

A is therefore the B1 comparison baseline. Arm 0 is historical evidence, not the current architecture choice.

### B — EXECUTION REPRESENTATION PRIMITIVE

- Primitive: EXEC148 / `Q4K_SERIAL_K_EXEC148_V0_1`.
- Status: **VALIDATED PERFORMANCE VALUE**.
- Role: **ARCHITECTURE PLACEMENT UNRESOLVED**.
- W-S component latency: 24.189140 ms/token.
- W-C component latency: 31.413020 ms/token.
- Current reference materialization: 231.6382 ms.
- Current incremental resident image: 549,527,552 bytes.
- Current implementation: CPU materializes directly into the final mapped Vulkan buffer on the existing UMA memory type.

Under that specific reference placement, the measured A↔B total-cost crossover is about 16.33 tokens for W-S and 36.07 tokens for W-C.

### A+B is not the target

The factorial AB arm answered a causal interaction question. It did not establish an architecture requirement to compose the primitives.

AB is slower than B in both frozen workloads and the interaction is antagonistic. B1 therefore assumes:

```text
DEFAULT / FALLBACK: A
OPTIONAL ALTERNATIVE: B under a placement/lifetime policy
NOT DEFAULT: A+B
```

## 2. Exact B1 question

> **Given that EXEC148 provides validated latency value but incurs materialization and residency costs, which system resource should create, host, and maintain the execution representation, and under what workload lifetime/reuse conditions does each placement dominate?**

This is a systems-architecture question, not a kernel-search question.

## 3. The placement tuple

B1 must never collapse "CPU/GPU/NPU/RAM" into one choice. Placement is a tuple:

```text
P =
  creator compute domain
  × residency memory domain
  × transfer / handoff path
  × lifetime policy
  × maintenance / eviction policy
```

This distinction is mandatory on the current host because the measured B image already resides in a Vulkan buffer that is simultaneously `HOST_VISIBLE|HOST_COHERENT|DEVICE_LOCAL` on the existing UMA memory type.

Changing the creator from CPU to GPU does not necessarily change the physical memory pool. Conversely, changing residency may add a copy even if creation becomes faster.

## 4. B1 cost model

Let `H` be the number of future decode tokens that reuse the same valid EXEC148 image before eviction or invalidation, and let `S` describe relevant runtime/resource state.

```text
Total_A(H,S)
  = H × L_A(S)
    + costs common to both paths

Total_B(P,H,S)
  = C_create(P)
  + C_transfer(P)
  + C_maintenance(P,H,S)
  + C_memory_pressure(P,H,S)
  + H × L_B(P,S)
```

A B placement dominates only when:

```text
Total_B(P,H,S) < Total_A(H,S)
```

and correctness, capacity, accessibility and resource-contention constraints all pass.

Scientific validation wall time is excluded from runtime materialization cost.

## 5. Candidate placement classes

### P0 — CPU direct to current UMA execution buffer

This is the measured reference, not the presumed winner.

Known:
- correctness;
- current creation wall time;
- current resident bytes;
- current B component latency.

Still unresolved:
- production lifetime;
- multi-session reuse;
- eviction policy;
- opportunity cost under memory pressure.

### P1 — GPU creates EXEC148 in place

Question:

> Can the GPU create the representation where it will execute without consuming more critical-path GPU time than it saves?

Before any benchmark, B1.1 must establish:
- exact transformation algorithm;
- source/destination accessibility;
- synchronization and overlap rules;
- whether creation steals time from prefill/decode;
- exact tuple correctness.

### P2 — NPU creates or maintains EXEC148

This is **capability-gated**, not assumed valuable.

The useful question is not "the NPU is idle, can we use its TOPS?" It is:

> Can the NPU read the source Q4_K, perform the exact byte/bit transformation, and make the resulting image available to Vulkan at a handoff cost low enough to change the A-vs-B decision?

Required first:
- usable programming/runtime path;
- source and destination memory visibility;
- exact transformation feasibility;
- coherency/copy boundary;
- optimistic creation+handoff bound.

Nominal TOPS alone is not evidence.

### P3 — Offline prematerialized EXEC148 sidecar

Creation need not happen during inference at all.

A versioned sidecar could move the transform to model packaging/install time:

```text
Q4_K model
   +
EXEC148 sidecar
   ↓
load / mmap / validate identity
   ↓
B execution
```

This trades runtime materialization against:
- persistent storage;
- load bandwidth;
- versioning;
- model/quant/kernel compatibility;
- cold/warm cache behavior.

This candidate is architecture-relevant because it could eliminate runtime creator compute entirely.

### P4 — Other capability-proven engine

DMA, DSP/audio hardware, or another engine may enter B1 only after a capability proof establishes:
- programmable transform or useful data movement;
- access to the required source/destination path;
- a plausible lower bound that could alter the decision.

An idle device is not automatically a useful resource.

## 6. Architecture output

B1 is not trying to name one permanent winner.

Its target output is a policy surface:

```text
operator / model state
        +
reuse horizon
        +
representation residency
        +
memory pressure
        +
resource availability / contention
        ↓
policy
   ├─ choose A
   └─ choose B with placement P
```

Examples of eventual policy states:

```text
EXEC148 absent + short horizon
    → A

EXEC148 already resident + valid + low pressure
    → B

long expected reuse + cheap creation/handoff
    → B

high memory pressure / likely eviction
    → A

resource used for B creation would block critical decode
    → A or another B placement
```

These are policy forms, not yet validated thresholds.

## 7. Kill-before-build rule

B1 follows the ArcLLM architecture-learning method: a placement must first survive a capability and optimistic-bound survey.

Kill a candidate before implementation if:
1. it cannot access the required source/destination data;
2. it cannot preserve the exact EXEC148 tuple;
3. its unavoidable copy/coherency path eliminates plausible advantage;
4. even an optimistic creation+handoff bound cannot beat A for any relevant reuse horizon.

No failed candidate may be rescued by post-hoc changes to EXEC148 semantics, A, the workload, or the decision metric.

## 8. Next scientific step

The next step is:

`B1.1_PLACEMENT_CAPABILITY_AND_BOUND_SURVEY`

No new performance execution is authorized.

B1.1 asks:

> For P0/P1/P2/P3/P4, which placement classes are physically/software feasible on the current platform, what exact data path would each require, and what optimistic lower bound on creation+handoff cost determines whether a runtime experiment is worth opening?

Expected output:
- feasibility matrix;
- data-path map;
- optimistic cost-bound ledger;
- shortlist of only placements whose evidence could change the A-vs-B architecture decision.

Explicitly closed during B1.1:
- new EXEC148 implementation;
- GPU/NPU performance benchmark;
- counter campaign;
- Token-XRay science run;
- soundcard/DSP experiment without capability proof;
- A retest;
- AB retest.
