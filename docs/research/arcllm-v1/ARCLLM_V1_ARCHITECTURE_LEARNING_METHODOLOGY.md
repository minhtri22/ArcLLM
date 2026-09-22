# ArcLLM v1 — Architecture Learning & Convergence Methodology

**Program:** ArcLLM v1  
**Branch:** `research/arcllm-v1`  
**Status:** FOUNDATION / METHODOLOGY FROZEN FOR DERIVED DESIGN WORK  
**Parent evidence line:** ArcLLM terminal commit `e30199cd54be1e0e7390e2453c8935c847fe0dd1`

## 1. Why ArcLLM v1 exists

ArcLLM v0 established two facts that must be held together:

1. a custom native Vulkan LLM runtime on Intel Arc is technically feasible, including direct packed GGUF execution, whole-decoder/segmented residency, GPU-resident KV, and real 7B end-to-end inference;
2. the validated ArcLLM architecture did not demonstrate a practical matched end-to-end advantage against the pinned mature llama.cpp baseline.

The second fact is a valid negative result for the tested architecture state. It is **not** evidence that a future ArcLLM architecture can never outperform a mature external runtime.

A mature baseline contains accumulated implementation knowledge. A new architecture may contain unexploited structural advantages but still lose because it carries large maturity debt.

ArcLLM v1 therefore changes the central question from:

> Does this optimization PASS?

to:

> How much of the observed system gap is structural disadvantage, how much is maturity debt, and which architectural intervention has the largest credible end-to-end headroom?

## 2. Core model

ArcLLM v1 treats the observed matched gap as:

```text
ObservedGap
  = StructuralGap
  + MaturityDebt
  + MeasurementUncertainty
```

These terms are conceptual rather than assumed to be linearly identifiable from one benchmark.

### StructuralGap

Cost imposed by the architecture itself even after competent optimization.

Examples:
- unavoidable extra memory movement;
- required synchronization topology;
- execution decomposition that produces irreducible small work units;
- quant/layout constraints that block efficient hardware mapping;
- a measurement or execution boundary whose lower bound is inherently worse than the comparison architecture.

### MaturityDebt

Recoverable cost caused by incomplete implementation knowledge.

Examples:
- untuned kernels;
- excessive dispatch granularity;
- missing fusion;
- command lifecycle overhead;
- weak scheduling;
- missing persistent execution;
- conservative memory layouts;
- unexploited subgroup primitives;
- duplicated state transformations.

### MeasurementUncertainty

Gap caused by incomplete attribution or instrumentation.

A performance gap must not be classified as structural while a material part of it remains unattributed.

## 3. Research objective

The goal of ArcLLM v1 is not simply "beat llama.cpp".

The goal is:

> Identify and validate at least one execution regime in which an Intel-Arc/Vulkan-native architecture has a defensible structural advantage that can propagate into matched end-to-end advantage after maturity debt is reduced.

Possible regimes may include, but are not assumed to include:
- batch-1 low-latency decode;
- constrained memory;
- long-context execution;
- a specific quantization family;
- persistent local inference;
- heterogeneous/shared-memory execution;
- integrated-GPU-specific dataflow.

A regime is accepted only through matched end-to-end evidence.

## 4. Four hard scientific invariants

ArcLLM v1 deliberately keeps governance lighter than ArcLLM v0, but retains four non-negotiable rules.

1. **Exact provenance.** Every scientific execution must bind the exact code, model, build/runtime payload, environment contract, and evidence identity needed to reproduce the claim.
2. **No post-outcome mutation inside an experiment.** Thresholds, hypothesis, primary endpoints, workload membership, and adjudication rules cannot be altered after outcome exposure for that experiment.
3. **Matched baseline.** Practical advantage claims require a matched external baseline or an explicitly scoped internal comparison when the claim is only architectural/mechanistic.
4. **Historical results are immutable.** Old PASS/FAIL/negative results are not rewritten. New knowledge may explain them better, but it does not delete the historical protocol or verdict.

Everything else may evolve prospectively in a new epoch/study.

## 5. Architecture-learning loop

The canonical v1 loop is:

```text
SYSTEM GOAL
    ↓
ARCHITECTURE + HARDWARE MODEL
    ↓
DATAFLOW / COST MODEL
    ↓
MATCHED BASELINE PROFILE
    ↓
GAP DECOMPOSITION
    ↓
STRUCTURAL GAP vs MATURITY DEBT vs UNKNOWN
    ↓
HEADROOM MAP
    ↓
SELECT MAX-VALUE INTERVENTION
    ↓
BOUNDED IMPLEMENTATION
    ↓
COMPONENT MEASUREMENT
    ↓
CARRY-THROUGH MEASUREMENT
    ↓
MATCHED END-TO-END MEASUREMENT
    ↓
UPDATE KNOWLEDGE MODEL
    ↓
CONTINUE / SPECIALIZE / REDESIGN
```

Experiments exist to reduce uncertainty in this loop. They are not the organizing center of the project.

## 6. Headroom Map

ArcLLM v1 maintains a living Headroom Map.

Each major subsystem records:

| Field | Meaning |
|---|---|
| Current cost | measured current ArcLLM cost |
| E2E share | fraction of current end-to-end cost |
| External reference | matched baseline or relevant reference |
| Lower bound | theoretical/empirical lower bound when defensible |
| Recoverable headroom | plausible maturity-debt reduction |
| Structural concern | evidence of an architecture-level ceiling |
| Confidence | quality of evidence supporting the estimate |
| Next discriminator | cheapest experiment that reduces decision uncertainty |

No subsystem is optimized merely because it is slow.

The first prioritization question is:

> If this subsystem became much better, how much end-to-end performance could the system possibly gain?

## 7. Amdahl-first intervention rule

Before implementation, every performance intervention must state an upper-bound carry-through estimate.

For subsystem share (f) and hypothetical subsystem speedup (s):

[
S_{E2E,max} = rac{1}{(1-f) + f/s}
]

When (s 	o infty):

[
S_{E2E,ceiling} = rac{1}{1-f}
]

An intervention with negligible theoretical E2E ceiling should not outrank an intervention that targets a dominant subsystem with credible recoverable headroom.

This rule prevents repeated investment in locally interesting but system-irrelevant micro-optimizations.

## 8. Carry-Through Ratio

ArcLLM v1 explicitly measures whether a component win survives integration.

For an intervention with a predicted integrated gain and an observed end-to-end gain:

[
CTR = rac{Observed E2E Improvement}{Predicted E2E Improvement}
]

CTR is diagnostic, not a universal score.

Low CTR indicates that a local win is being absorbed by other costs such as:
- synchronization;
- memory traffic;
- scheduling;
- graph fragmentation;
- lifecycle overhead;
- a newly dominant bottleneck.

A component PASS without carry-through is knowledge, not system victory.

## 9. Learning curves instead of one-time snapshots

ArcLLM v1 tracks three maturity curves across architecture epochs:

1. **Performance maturity** — matched gap versus baseline;
2. **Correctness maturity** — breadth and stability of supported execution regimes;
3. **Knowledge maturity** — fraction of dominant cost that is causally attributed.

A new architecture is not stopped merely because it loses today.

A stop/redesign case becomes strong when all are true:

- the matched gap remains material;
- the cost/lower-bound model shows insufficient recoverable headroom;
- multiple high-value causal interventions fail to improve the convergence trajectory.

This is evidence of a structural ceiling rather than merely low maturity.

## 10. Architecture epochs

ArcLLM v1 uses **Architecture Epochs**, not open-ended phase proliferation.

An epoch contains:

```text
Architecture hypothesis
Predicted bottleneck change
Cost-model prediction
Bounded implementation
Correctness qualification
Component measurement
Carry-through measurement
Matched E2E measurement
Knowledge-model update
```

An epoch may change architecture, workloads, or mechanisms prospectively. It may not retroactively modify an already observed experiment.

## 11. Researchable-runtime architecture requirement

ArcLLM v1 must be designed as a researchable runtime.

Logical layers:

```text
L0 HARDWARE MODEL
   Intel Arc / Vulkan / queues / subgroups / memory hierarchy

L1 TENSOR + QUANT MODEL
   packed Q4/Q6/... layouts, segmentation, numerical semantics

L2 EXECUTION GRAPH IR
   operators, dependencies, residency, lifecycle, scheduling boundaries

L3 KERNEL PORTFOLIO
   quant-specific and shape-specific execution families

L4 EXECUTION POLICY
   fusion, persistence, command reuse, scheduling, prefetch, specialization

L5 MODEL RUNTIME
   prefill, decode, KV, logits, generation

L6 OBSERVABILITY + COST MODEL
   device time, host lifecycle, bytes moved, dispatch/barrier census,
   predicted versus actual cost
```

L6 is part of the architecture, not an optional debugging layer.

## 12. Quant specialization principle

SA1 demonstrated that a strong Q4 mechanism did not automatically generalize to Q6 correctness.

ArcLLM v1 therefore assumes:

```text
quant format
  → numerical geometry
  → memory representation
  → subgroup/reduction constraints
  → specialized execution family
```

A universal kernel is not the default design objective.

Shared abstractions are welcome only when they do not erase the execution semantics needed for a specific quant family.

## 13. Baseline policy

A mature baseline is a measuring instrument, not a guillotine.

Baseline comparison is used to:
- quantify the current gap;
- identify which subsystem dominates that gap;
- measure convergence rate;
- distinguish local wins from system wins.

A baseline loss does not by itself terminate an architecture.

A baseline win in one metric does not by itself establish system advantage.

## 14. Knowledge ledger

Every study must update an append-only knowledge ledger with one of:

- SUPPORTED_MECHANISM;
- FALSIFIED_MECHANISM;
- MATURITY_DEBT_IDENTIFIED;
- STRUCTURAL_LIMIT_SUPPORTED;
- MEASUREMENT_GAP;
- INTEGRATION_FAILURE;
- NEGATIVE_BOUNDARY;
- UNRESOLVED.

The knowledge ledger records **what the project learned**, not only whether a gate passed.

## 15. Intervention selection

Interventions are ranked qualitatively from four terms:

[
Priority propto
E2E Share
	imes Recoverable Headroom
	imes Evidence Confidence
div Implementation Risk
]

This is not a frozen numerical score. It is a decision framework.

A high-priority intervention should:
- target a large E2E share;
- have evidence that a material part is recoverable;
- have a causal mechanism rather than a generic optimization idea;
- be measurable at component and E2E levels;
- have a clear falsification condition.

## 16. No self-rescue, but continued learning is allowed

ArcLLM v1 distinguishes:

**Forbidden self-rescue**
- changing an experiment's gate after seeing the result;
- deleting a failed cell;
- changing the primary endpoint post hoc;
- reclassifying an old negative as a PASS.

**Allowed scientific evolution**
- opening a new epoch with a new architecture;
- registering a new mechanism;
- changing workloads when the new regime is explicitly the research object;
- improving instrumentation;
- replacing an oracle prospectively;
- using previous negatives as hypothesis-generating evidence.

Scientific flexibility is allowed. Outcome rewriting is not.

## 17. What historical ArcLLM contributes to v1

ArcLLM v1 inherits evidence, not conclusions beyond their scope.

Strong inherited evidence includes:
- direct packed GGUF execution viability;
- whole-decoder and segmented residency feasibility;
- 7B end-to-end feasibility;
- GPU-resident KV feasibility;
- quant-specific component wins and failures;
- P8 numerical/oracle-semantic lessons;
- matched Q2/Q3 performance gap;
- ANL64 evidence that decode/E2E can improve materially while TTFT worsens;
- TTFT-M2 evidence that the preregistered TTFT mechanisms did not stably replicate;
- governance evidence separating infrastructure failure from scientific failure.

Historical data become priors and constraints for Headroom analysis. They do not automatically become v1 claims.

## 18. First derived artifacts

After this methodology, and in this order, ArcLLM v1 must create:

1. `ARCLLM_V1_ARCHITECTURE_REFRAME.md`
2. `ARCLLM_V1_HEADROOM_MAP.md`
3. `ARCLLM_V1_INTERVENTION_001_SELECTION.md`

No v1 intervention implementation is authorized by this methodology document itself.

## 19. Success condition for ArcLLM v1

ArcLLM v1 succeeds scientifically if it can do one of two things:

### A. Demonstrate a structural advantage

A new architecture produces a reproducible matched end-to-end advantage in a clearly defined regime, with causal carry-through from mechanism to system result.

### B. Establish a structural ceiling

The architecture-learning process shows that the remaining gap cannot plausibly be closed by available maturity headroom, after well-targeted causal interventions.

Both outcomes are useful.

The failure mode to avoid is neither losing nor obtaining a negative result.

The real failure mode is continuing to optimize without knowing whether the architecture still has credible system-level headroom.
