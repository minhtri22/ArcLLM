# ArcLLM v1 — Architecture Learning & Convergence Methodology

**Program:** ArcLLM v1  
**Branch:** `research/arcllm-v1`  
**Status:** FOUNDATION / QA-AMENDED BEFORE I001 DISCRIMINATOR  
**QA note:** terminology, equations, evidence classes, and prior-art transfer limits clarified prospectively before any ArcLLM v1 performance execution  
**Parent evidence line:** ArcLLM terminal commit `e30199cd54be1e0e7390e2453c8935c847fe0dd1`

## 0. Scope, terminology provenance, and evidence classes

ArcLLM v1 intentionally combines established performance-analysis ideas with project-specific research vocabulary.

### Literature-derived concepts

The following concepts have established prior art and are used in their ordinary systems/performance sense:

- **Amdahl-style speedup ceiling:** the maximum system speedup is limited by the fraction not improved by an intervention.
- **Roofline-style bound reasoning:** performance analysis should distinguish compute capability, data movement, and achievable hardware ceilings rather than infer bottlenecks from FLOP counts alone.
- **IO-aware / locality-aware optimization:** wall-clock speed can be dominated by movement through the memory hierarchy even when arithmetic work is unchanged.
- **iteration / operation-level scheduling:** autoregressive inference performance depends on how repeated model iterations and heterogeneous operations are scheduled, not only on individual kernels.
- **paged / non-contiguous KV management:** memory-layout and KV-cache policy can materially change serving efficiency without changing model semantics.

### ArcLLM-v1 project terms

The following are **project-defined analytical terms**, not claims of standard terminology in the literature:

- `StructuralGap`
- `MaturityDebt`
- `MeasurementUncertainty`
- `Headroom Map`
- `Carry-Through Ratio (CTR)`
- `Architecture Epoch`
- `Persistent Decode Execution Plane (PDEP)`
- `decode graph compression`

These terms are operational tools for organizing ArcLLM evidence. They must not be cited as established laws or treated as directly measurable quantities unless a study defines an estimator for them.

### Evidence classes

ArcLLM v1 distinguishes four evidence classes:

1. **MEASURED:** directly observed in an exact execution.
2. **DERIVED:** arithmetic or deterministic transformation of measured evidence.
3. **BOUND:** theoretical or empirical ceiling/floor with explicit assumptions.
4. **HYPOTHESIS:** causal explanation not yet discriminated by an experiment.

Every Headroom Map claim should be traceable to one of these classes.

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

ArcLLM v1 partitions explanations of an observed matched gap into three project-defined classes:

```text
ObservedGap explanations
  ├─ StructuralGap
  ├─ MaturityDebt
  └─ MeasurementUncertainty
```

This is a **taxonomy, not an arithmetic identity**. The three classes need not be additive, independent, or directly identifiable from one benchmark. A study may leave part of the gap `UNRESOLVED` rather than force attribution.

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

### Operational term contract

| Term | ArcLLM-v1 operational meaning | What would strengthen the claim | What it does **not** mean |
|---|---|---|---|
| `execution regime` | a frozen combination of model, workload shape, batch/concurrency, context/output lengths, hardware/software environment, and comparison contract | exact matched reproduction | a broad hardware category or an informal usage scenario |
| `mature baseline` | a pinned external runtime with established optimization history, exact version/build, and matched workload/environment | multiple matched sessions and reproducible build identity | a theoretically optimal implementation |
| `StructuralGap` | residual disadvantage that remains after credible maturity headroom is bounded away or exhausted under the same semantic contract | lower-bound analysis plus failed high-headroom interventions | any gap that is merely large |
| `MaturityDebt` | cost plausibly recoverable by implementation/execution improvements without changing the semantic problem | causal intervention with carry-through or a defensible bound | a promise that the gap is recoverable |
| `MeasurementUncertainty` | material cost or attribution that cannot yet be assigned because instrumentation/bounds are insufficient | improved instrumentation or a discriminating experiment | random noise only |
| `headroom` | room between current measured cost and an explicit bound/reference under stated assumptions | tighter lower/upper bounds | expected speedup |
| `recoverable headroom` | the subset of headroom for which a mechanism and evidence make recovery plausible | causal A/B or mechanistic bound | total current gap |
| `structural advantage` | matched end-to-end benefit linked to a mechanism whose benefit is not merely an artifact of an immature comparator | causal mapping, bound analysis, reproduced matched E2E benefit | a single benchmark win |
| `structural ceiling` | evidence that remaining plausible maturity improvements are insufficient to meet the target under the frozen regime | tight bounds plus multiple failed high-value interventions | failure of one implementation |
| `material` | large enough to cross a threshold preregistered for the study in which the term is used | frozen numeric threshold | a universal fixed percentage |
| `confidence` | quality of evidence supporting a Headroom Map entry; LOW/MEDIUM/HIGH must be justified by provenance, replication, and causal specificity | independent or repeated evidence | statistical confidence interval unless explicitly stated |

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
S_{E2E}(f,s) = \frac{1}{(1-f) + f/s}
]

For an idealized infinite subsystem speedup:

[
S_{E2E,ceiling}(f) = \frac{1}{1-f}
]

These are Amdahl-style ceilings. They are **bounds under the stated cost partition**, not predictions of realized speedup and not proof that the partition is causally correct.

An intervention with negligible theoretical E2E ceiling should not outrank an intervention that targets a dominant subsystem with credible recoverable headroom.

This rule prevents repeated investment in locally interesting but system-irrelevant micro-optimizations.

## 8. Carry-Through Ratio

`Carry-Through Ratio (CTR)` is an **ArcLLM-v1 diagnostic**, not a standard systems metric.

For an intervention with a preregistered predicted integrated improvement and an observed matched end-to-end improvement, define:

[
CTR = \frac{Observed\ E2E\ improvement}{Predicted\ E2E\ improvement}
]

The numerator and denominator must use the same direction convention; for latency, convert both to speedup or fractional reduction before taking the ratio.

CTR is meaningful only when the prediction was frozen before the integrated outcome. It must not be manufactured after seeing the result.

Low CTR indicates that a local win may be absorbed by other costs such as:
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

ArcLLM v1 uses a qualitative prioritization heuristic:

[
Priority \propto \frac{E2E\ Share \times Plausible\ Recoverable\ Headroom \times Evidence\ Quality}{Implementation\ Risk}
]

This expression is **not a calibrated numerical score**. It is a checklist forcing each intervention to expose the terms that justify priority.

A high-priority intervention should:
- target a large E2E share;
- have evidence that a material part is plausibly recoverable;
- have a causal mechanism rather than a generic optimization idea;
- be measurable at component and E2E levels;
- have a clear falsification condition.

When a term is unknown, the next action should usually be a discriminator that reduces uncertainty rather than immediate implementation.

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

## 17A. Prior-art foundation and transfer limits

ArcLLM v1 uses the following works as **conceptual prior art**, not as empirical evidence for Intel Arc 140V or Vulkan.

| Work | Concept imported | ArcLLM-v1 use | Transfer limit |
|---|---|---|---|
| Gene M. Amdahl, *Validity of the Single Processor Approach to Achieving Large Scale Computing Capabilities*, AFIPS 1967, DOI 10.1145/1465482.1465560 | serial/unimproved fraction limits total speedup | Amdahl-first E2E ceiling | does not identify ArcLLM subsystem shares |
| Williams, Waterman, Patterson, *Roofline*, CACM 2009, DOI 10.1145/1498765.1498785 | relate arithmetic work, memory traffic, and hardware bounds | cost/lower-bound discipline | original model is not a complete model of LLM/Vulkan scheduling |
| Dao et al., *FlashAttention*, NeurIPS 2022, DOI 10.52202/068431-1189 | IO-aware algorithm design; reduce memory-hierarchy traffic rather than FLOPs alone | locality/data-movement reasoning | attention-specific and evaluated on different hardware/software |
| Dao, *FlashAttention-2*, ICLR 2024 | work partitioning, occupancy, communication overhead can leave headroom after a strong v1 | motivates maturity-aware re-partitioning | NVIDIA/A100 results do not transfer numerically to Intel Arc |
| Kwon et al., *Efficient Memory Management for LLM Serving with PagedAttention*, SOSP 2023 / arXiv:2309.06180 | KV layout/fragmentation policy can affect system throughput | memory/residency architecture | serving/batching regime differs from ArcLLM batch-1 focus |
| Yu et al., *Orca*, OSDI 2022 | autoregressive iteration-level scheduling matters at system level | separates scheduling policy from model semantics | distributed serving scheduler, not a decode-kernel result |
| Agrawal et al., *Sarathi-Serve*, OSDI 2024 | prefill/decode composition can create stalls and latency/throughput tradeoffs | requires integrated TTFT/decode constraints | multi-request serving results do not establish ArcLLM causes |
| Ye et al., *FlashInfer*, MLSys 2025 | customizable attention, scheduling, KV formats, graph-compatible execution | supports explicit execution-policy / specialization design | CUDA-centric serving engine; mechanism must be revalidated on Vulkan |
| Zhu et al., *NanoFlow*, OSDI 2025 | operation-level scheduling and intra-device overlap can improve end-to-end utilization | motivates execution-topology decomposition | high-throughput server regime differs from local batch-1 |
| Holmes et al., *DeepSpeed-FastGen*, arXiv:2401.08671 | prompt/generation composition and persistent/non-persistent serving choices affect latency/throughput | adjacent evidence for system-level phase coupling | not an Intel Arc or Vulkan causal result |

The literature supports the **need to model memory, scheduling, work partitioning, and end-to-end carry-through**. It does not validate `MaturityDebt`, `PDEP`, or any ArcLLM-v1 mechanism by itself.

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


## 16A. Dev-host execution governance — 2026-09-23

ArcLLM v1 may run on a shared development machine.

Dedicated-idle-machine requirements and permanent one-shot execution locks are not hard scientific invariants.

Prospective DEV_HOST rules:
- ambient CPU/RAM/GPU/process load is metadata, not an automatic blocker;
- every execution attempt is append-only and receives a unique identity;
- manual full-collection rerun is allowed under the same frozen payload;
- no selective cell rerun;
- no candidate/threshold/workload mutation between attempts;
- first complete valid collection is primary confirmatory evidence;
- later complete collections are replication/robustness evidence and cannot replace or erase the primary result;
- operationally failed/partial attempts remain provenance but do not permanently close the study.

This preserves the four hard invariants: exact provenance, no post-outcome mutation inside an experiment, matched comparison, and immutable historical results.
