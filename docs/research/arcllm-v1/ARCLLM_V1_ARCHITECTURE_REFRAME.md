# ArcLLM v1 — Architecture Reframe

**Derived from:** `ARCLLM_V1_ARCHITECTURE_LEARNING_METHODOLOGY.md`  
**Methodology blob:** `d41a357840435f52a682792b9a6608e4abaab312`  
**Status:** ARCHITECTURE REFRAME / SPECIFICATION ONLY  
**No intervention implementation authorized**

## 1. Reframed system question

ArcLLM v1 is not a continuation whose goal is to tune the old ArcLLM graph until it eventually beats llama.cpp.

It is a successor architecture-learning runtime.

The system question is:

> Can an Intel-Arc/Vulkan-native execution architecture expose a structural advantage in at least one defined regime, and can that advantage survive the full path from hardware mechanism to matched end-to-end behavior?

The architecture must therefore optimize for two outcomes simultaneously:

1. efficient inference;
2. efficient learning about why inference is or is not efficient.

## 2. Legacy ArcLLM status

Historical ArcLLM code and results are **evidence/reference**, not immutable v1 architecture.

Allowed:
- reuse a proven tensor parser;
- reuse exact packed quant semantics;
- reuse validated residency logic;
- reuse kernels whose behavior is supported;
- reuse evidence contracts and test fixtures;
- reuse model/baseline identities for matched comparison.

Not assumed:
- old execution graph is optimal;
- 441 prefill dispatches are desirable;
- 469 decode dispatches per step are desirable;
- old command-buffer lifecycle is desirable;
- one generic execution family should cover Q4 and Q6;
- historical architecture boundaries should define v1 module boundaries.

## 3. Architecture principles

### A1 — Separate semantic model from execution topology

Model semantics must be represented independently from the concrete Vulkan dispatch graph.

The same semantic operator graph may be lowered into different execution plans.

This is required to compare:
- many-small-dispatch execution;
- fused execution;
- persistent execution;
- quant-specialized execution;
- alternative residency/scheduling policies.

### A2 — Separate prefill and decode execution planes

Historical evidence shows that prefill and decode have very different geometry and maturity.

ArcLLM v1 must not force them into one shared optimization topology.

Canonical split:

```text
MODEL SEMANTICS
      ↓
COMMON GRAPH IR
      ↓
 ┌──────────────┬──────────────┐
 │ PREFILL PLAN │ DECODE PLAN  │
 └──────────────┴──────────────┘
```

Shared semantics and weights are allowed. Execution policy is independently evolvable.

### A3 — Quant-specific execution portfolios

Q4 and Q6 are separate execution families by default.

A quant family may define:
- data unpack/dequant path;
- subgroup topology;
- reduction order;
- accumulation type;
- tile geometry;
- fusion legality;
- numerical oracle.

Cross-quant sharing must be demonstrated rather than assumed.

### A4 — Residency is a first-class architecture object

ArcLLM already established that whole-decoder/segmented residency is feasible.

v1 turns residency into a planning object that exposes:
- weight placement;
- KV placement;
- working-set lifetime;
- reuse distance;
- upload/download boundaries;
- allocation geometry;
- memory-pressure cost.

### A5 — Execution policy is explicit

Scheduling choices must not be hidden inside kernel implementations.

Execution policy owns:
- command reuse;
- persistent work;
- fusion boundaries;
- dispatch grouping;
- synchronization;
- prefetch;
- pipeline selection;
- specialization.

This allows a mechanism to be changed without rewriting model semantics.

### A6 — Observability is architectural

Every execution plan must expose enough telemetry to update the cost model.

Required logical telemetry:
- operator and kernel identity;
- dispatch count;
- barrier/sync count;
- device time where available;
- host record/submit/wait time;
- bytes read/written estimates;
- residency transitions;
- temporary allocation/lifetime;
- model-stage timing;
- matched end-to-end timing.

No intervention should require ad-hoc instrumentation reconstruction after the fact.

## 4. Proposed v1 logical architecture

```text
┌──────────────────────────────────────────────┐
│ L6  OBSERVABILITY + COST MODEL               │
│     measured vs predicted cost / headroom    │
└──────────────────────────────────────────────┘
                      ↑
┌──────────────────────────────────────────────┐
│ L5  MODEL RUNTIME                            │
│     generation / logits / KV lifecycle       │
└──────────────────────────────────────────────┘
                      ↑
┌───────────────────────┬──────────────────────┐
│ L4-P PREFILL POLICY   │ L4-D DECODE POLICY   │
│ batching / fusion     │ persistence / reuse  │
└───────────────────────┴──────────────────────┘
                      ↑
┌──────────────────────────────────────────────┐
│ L3  KERNEL PORTFOLIOS                        │
│ Q4 / Q6 / attention / norm / elementwise     │
└──────────────────────────────────────────────┘
                      ↑
┌──────────────────────────────────────────────┐
│ L2  EXECUTION GRAPH IR                       │
│ semantic ops + dependency/lifetime metadata  │
└──────────────────────────────────────────────┘
                      ↑
┌──────────────────────────────────────────────┐
│ L1  TENSOR / QUANT / RESIDENCY MODEL         │
│ packed layout / segments / KV / buffers      │
└──────────────────────────────────────────────┘
                      ↑
┌──────────────────────────────────────────────┐
│ L0  HARDWARE SUBSTRATE                       │
│ Intel Arc / Vulkan / memory / queues         │
└──────────────────────────────────────────────┘
```

L6 observes all lower layers and feeds the Headroom Map.

## 5. Execution Graph IR

The IR is the central architecture change.

Each semantic node should eventually carry, at minimum:
- semantic op;
- input/output tensor identities;
- quant family;
- shape;
- dependency set;
- residency requirement;
- lifetime class;
- correctness/oracle contract;
- available kernel families;
- candidate fusion group;
- execution-policy annotations.

The IR must not initially encode a single winner.

Its role is to make alternative lowerings comparable.

## 6. Prefill plane

Historical ArcLLM invested heavily in prefill and demonstrated substantial component gains.

Therefore v1 prefill is treated as a relatively mature but still noncompetitive plane.

Primary questions:
- which cost remains structural versus maturity debt?
- how much of the 441-dispatch topology is necessary?
- do quant-specific fused projection/FFN paths carry through at model level?
- is the TTFT gap primarily compute, memory movement, graph fragmentation, or lifecycle interaction?

Prefill is not selected for Intervention-001 by this document.

## 7. Decode plane

Historical evidence shows decode is both:
- dramatically behind the mature matched baseline;
- less optimized than prefill;
- capable of multi-x movement under successor execution changes.

Therefore the decode plane must be independently architectable.

Candidate policy capabilities may include:
- prepared graph reuse;
- command reuse;
- persistent dispatch/execution;
- fused operator regions;
- GPU-resident control state;
- quant-specific decode kernels;
- reduced synchronization boundaries.

This list is an architecture capability set, not an intervention decision.

## 8. Correctness architecture

Correctness is not a post-build gate.

Every execution family must specify:
- production arithmetic semantics;
- acceptable oracle;
- accumulation semantics;
- numerical tolerance;
- state handoff invariants;
- exact semantic guard where possible.

Historical P8-G and SA1 evidence shows that a numerically "better" oracle or a mechanism from another quant family can produce misleading attribution.

## 9. Comparison architecture

The runtime must support three comparison planes.

### Plane A — local mechanism

Candidate kernel/policy versus exact local reference.

Purpose:
- causal mechanism qualification.

### Plane B — integrated ArcLLM

New architecture epoch versus previous ArcLLM epoch.

Purpose:
- measure carry-through and convergence.

### Plane C — matched external baseline

Current ArcLLM epoch versus pinned mature baseline.

Purpose:
- determine practical regime advantage.

No Plane-A result may substitute for Plane-C evidence.

## 10. Cost model boundary

The cost model must separate at least:

```text
compute
memory traffic
dispatch / graph granularity
barriers / synchronization
host record/submit lifecycle
residency / allocation
CPU-GPU interaction
unattributed
```

A cost term may remain UNKNOWN.

UNKNOWN is preferable to incorrectly labeling the gap as structural.

## 11. Architecture maturity states

For every major subsystem:

- PROVEN_CORRECT;
- PERFORMANCE_UNCHARACTERIZED;
- MATURITY_DEBT_LIKELY;
- STRUCTURAL_CONCERN;
- STRUCTURAL_LIMIT_SUPPORTED;
- SPECIALIZED_ADVANTAGE_SUPPORTED.

These are knowledge states, not permanent labels.

## 12. Epoch interface

Each future architecture epoch must declare:

```text
Parent epoch
Changed architecture object
Mechanism hypothesis
Predicted cost term affected
Predicted E2E carry-through
Correctness boundary
Matched baseline boundary
Falsification condition
Knowledge update on PASS
Knowledge update on FAIL
```

This prevents implementation from becoming the hypothesis.

## 13. Baseline timing

Matched baseline comparison enters early.

A minimally correct integrated epoch should be compared before deep optimization.

The baseline is then revisited after each material architecture epoch.

This creates a convergence curve instead of a single terminal comparison.

## 14. Architecture success

ArcLLM v1 architecture is not required to dominate globally.

A valid success may be:

> In regime R, architecture mechanism M creates a reproducible structural advantage A, survives integration with high carry-through, and produces matched end-to-end advantage.

That is enough to justify further maturation.

## 15. Architecture stop/redesign condition

A subsystem or whole architecture should be redesigned when:
- matched gap remains large;
- dominant costs are causally attributed;
- lower-bound/headroom analysis says remaining maturity debt cannot close the gap;
- high-value interventions fail to improve the convergence trajectory.

A single baseline loss is insufficient.

## 16. Derived next artifact

The next required artifact is:

`ARCLLM_V1_HEADROOM_MAP.md`

It must use historical ArcLLM evidence to quantify:
- where the end-to-end time currently goes;
- which gaps are already known to be recoverable;
- which gaps remain unknown;
- which intervention family has the largest theoretical E2E headroom.

No intervention is implemented in this architecture-reframe step.
