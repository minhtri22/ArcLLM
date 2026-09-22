# ArcLLM v1 — Intervention-001 Selection

**Derived from:**  
- `ARCLLM_V1_ARCHITECTURE_LEARNING_METHODOLOGY.md`  
- `ARCLLM_V1_ARCHITECTURE_REFRAME.md`  
- `ARCLLM_V1_HEADROOM_MAP.md`

**Status:** INTERVENTION FAMILY SELECTED / NO IMPLEMENTATION AUTHORIZED

## 1. Selected intervention

```text
I-001 — PERSISTENT DECODE EXECUTION PLANE
        / DECODE GRAPH COMPRESSION
```

Short name:

`I001-PDEP`

The intervention targets the **decode/post-TTFT execution topology**, not one individual math kernel.

## 2. Why this is Intervention-001

Historical Q2 shows the post-TTFT generation plane accounts for approximately:

```text
W-S  99.1% of ArcLLM E2E
W-C  83.8% of ArcLLM E2E
```

The coarse post-TTFT ArcLLM/baseline gap is approximately:

```text
W-S  39.98×
W-C  32.08×
```

P7 reports 469 dispatches per cached decode step and explicitly notes that decode had not received the optimization depth of prefill.

ANL64 independently demonstrates that ArcLLM-family decode behavior is not fixed:
- decode improved by ~1.22× to 3.20× against the exact safe ArcLLM reference;
- E2E latency improved by ~1.14× to 2.51×;
- the program failed because TTFT worsened, not because decode lacked recoverable headroom.

This combination makes decode the only area with:
- dominant E2E share;
- huge external gap;
- direct evidence of multi-x recoverability;
- incomplete maturity.

## 3. Why not TTFT first

For the historical 32-token workloads, TTFT occupies approximately:
- 0.9% of W-S E2E;
- 16.2% of W-C E2E.

Even reducing TTFT to zero would yield only approximately:
- 1.009× W-S E2E;
- 1.193× W-C E2E.

TTFT remains important as:
- a non-regression constraint;
- a future short-output regime;
- a later bottleneck after decode improves.

It is not the highest-headroom first intervention.

## 4. Why not another isolated prefill kernel

Historical ArcLLM already recovered large prefill component gains.

P7-L's remaining prefill profile also shows that individual untouched kernel families no longer dominate enough to match the decode-plane E2E headroom.

SA1's ~3.14× Q4 component result is scientifically valuable but has no demonstrated system-level carry-through.

Therefore an isolated kernel is not selected before the decode architecture plane.

## 5. Why not memory first

Working set is approximately 1.83× the matched baseline, which is a real concern.

However:
- private bytes are slightly lower than baseline;
- the working-set difference is not causally decomposed;
- its direct E2E performance carry-through is unknown.

Memory topology remains a high-value future architecture track, especially for integrated/shared-memory regimes, but current evidence does not rank it above decode for E2E performance.

## 6. Mechanism hypothesis

The selected hypothesis is deliberately narrower than "469 dispatches are bad".

### H-I001

> A material portion of ArcLLM's decode maturity debt is caused by fine-grained repeated execution topology: many small dispatch regions, synchronization boundaries, repeated scheduling/state setup, and weak cross-operator locality. A persistent GPU-resident decode execution plane that compresses the graph into larger reusable execution regions can reduce that recoverable cost and improve decode/E2E performance while preserving model semantics.

This does **not** assume host submission overhead is the dominant cost.

Historical P7-A evidence already warns against that simplistic interpretation.

The intervention targets the combined execution-topology cost:
- device scheduling of many small kernels;
- synchronization boundaries;
- pipeline/descriptor/state transitions;
- missed locality across adjacent operators;
- command lifecycle where material;
- inability to retain execution state across token steps.

## 7. Architecture delta

The intended v1 delta is:

```text
CURRENT DECODE
semantic graph
  ↓
hundreds of fine-grained Vulkan dispatches / token
  ↓
many execution boundaries

I001-PDEP
semantic graph IR
  ↓
decode-region partition
  ↓
persistent/reusable execution regions
  ↓
quant-specialized kernels inside regions
  ↓
fewer explicit execution boundaries / token
```

The intervention may eventually use:
- persistent command/execution objects;
- region fusion;
- GPU-resident control state;
- reduced descriptor/pipeline rebinding;
- quant-specific region lowering;
- state reuse across token steps.

The exact implementation mechanism is **not frozen by this selection artifact**.

It must be chosen by the next cost-model study.

## 8. What remains unchanged conceptually

I001 must preserve:
- exact model semantics;
- packed quant identity;
- GPU-resident KV semantics;
- greedy generation semantics;
- matched model/workload identities for comparisons;
- production numerical contracts.

I001 must not silently use:
- reduced model depth;
- reduced vocabulary;
- CPU model-math fallback;
- teacher forcing;
- a weaker external baseline.

## 9. Theoretical E2E headroom

Using historical Q2 cost shares:

| Decode/post-plane improvement | W-S predicted E2E ceiling | W-C predicted E2E ceiling |
|---:|---:|---:|
| 2× | ~1.98× | ~1.72× |
| 3× | ~2.94× | ~2.27× |
| 5× | ~4.82× | ~3.04× |
| 10× | ~9.22× | ~4.08× |

These are Amdahl envelopes, not expected outcomes.

They establish that I001 has materially larger system headroom than TTFT-only work or a small residual prefill kernel family.

## 10. Required pre-implementation discriminator

No PDEP code should be written yet.

The next study must decompose the current decode step into a cost model capable of separating:

```text
kernel compute
memory traffic / locality
dispatch granularity
barriers / synchronization
pipeline / descriptor lifecycle
host record / submit / wait
persistent-state opportunity
unattributed
```

The study must estimate how much cost can plausibly be removed by graph compression/persistence.

If the attributable recoverable share is small, I001 must be falsified **before** a major implementation effort.

## 11. Required measurement architecture

The next specification must support:

### Existing execution
- exact 469-dispatch decode-step census;
- per-family device time;
- dispatch-duration distribution;
- barrier/synchronization census;
- host lifecycle timing;
- bytes/lifetime estimates where defensible.

### Candidate region model
Before implementation:
- proposed region partition;
- dispatch-boundary reduction;
- theoretical lower/upper cost bounds;
- predicted carry-through into decode and E2E.

After implementation, if authorized:
- local region measurement;
- full decode throughput;
- E2E carry-through;
- TTFT non-regression;
- matched external baseline.

## 12. TTFT constraint learned from ANL64

ANL64 proved that decode/E2E can improve while TTFT worsens enough to invalidate the overall practical claim.

Therefore I001 must treat TTFT as an explicit integrated constraint.

The next experiment must preregister a TTFT non-regression/materiality rule before timing.

The exact threshold is not chosen here.

## 13. Carry-through requirement

I001 is not successful merely because:
- dispatch count falls;
- one region becomes faster;
- decode microbenchmark improves.

The required chain is:

```text
execution-topology cost reduced
        ↓
decode throughput improves
        ↓
E2E improves
        ↓
TTFT remains acceptable
        ↓
matched external gap contracts
```

Every transition must be measured.

## 14. Falsification routes

I001 should be abandoned or redesigned if the next bounded studies show any of:

1. dispatch/region fragmentation is not a material part of decode cost;
2. the credible recoverable share is too small to create meaningful E2E carry-through;
3. persistence/region fusion creates unacceptable correctness/numerical instability;
4. local gains disappear after integration;
5. decode gains reproduce ANL64-style TTFT harm without a separable design;
6. matched external gap does not contract after a mature integrated implementation.

The exact numeric gates belong to the next preregistered study.

## 15. Alternative interventions retained

Not selected first, but retained in the Headroom Map:

### I-002 candidate — memory / working-set topology
Reason: ~1.83× working-set gap and potential integrated-GPU structural opportunity.

### I-003 candidate — residual prefill architecture
Reason: ~9–10× TTFT gap remains, but current 32-token E2E share is smaller and old TTFT mechanisms are unsupported.

### I-004 candidate — quant-specialized execution portfolios
Reason: Q4 has strong local evidence; Q6 requires separate numerical architecture.

These are not authorized automatically if I001 fails. The Headroom Map must be updated first.

## 16. Next scientifically valid step

Create a specification-only study:

`ARCLLM_V1_I001_DECODE_COST_MODEL_AND_MECHANISM_DISCRIMINATOR.md`

Its purpose is to decide whether PDEP has enough **causally attributable recoverable decode cost** to justify implementation.

No ArcLLM v1 performance code should be changed before that discriminator is frozen.
