# Q6CB-1 — Q6 Correctness-Boundary Mechanism Study

**Status:** SPECIFICATION ONLY — NO FRESH EXECUTION AUTHORIZED  
**Program ID:** Q6CB-1  
**Branch:** `research/q6-correctness-boundary`  
**Date:** 2026-09-21  
**Parent scientific state:** SA1 formally closed as `SA1_CLOSED_ASYMMETRIC_Q4_PASS_Q6_CORRECTNESS_FAIL`

## 1. Origin

SA1 established an asymmetric bounded result:

- frozen Q4_K component path: correctness PASS and reproducible performance PASS;
- unchanged Q6_K extension: shader compile PASS, native BuildOnly PASS, then frozen correctness failure at `candidate_cpu`;
- Q6 performance was never authorized or measured;
- target-model integration was not run.

The SA1 Q6 failure is valid evidence and remains immutable. Q6CB-1 exists only to explain the **mechanism of the numerical correctness boundary** observed after changing from serial FP32 accumulation to subgroup-32 split-K reduction under Q6_K.

## 2. Why Q6CB-1 is not an SA1 rescue

Q6CB-1 is explicitly non-rescuing:

1. SA1's Q6 classification cannot be changed by any Q6CB-1 outcome.
2. No Q6CB-1 result can retroactively convert `Q6_STAGE_FAIL_CORRECTNESS` to PASS.
3. SA1 thresholds, fixtures, evidence, hashes, source state and adjudication remain immutable.
4. Q6CB-1 contains no performance objective and cannot authorize Q6 timing.
5. Q6CB-1 contains no target-model integration objective.
6. A future implementation that repairs or replaces the SA1 mechanism would be a separate successor study with a new hypothesis and authorization; it is not part of Q6CB-1.
7. Failure to identify a mechanism is an admissible final result.

## 3. Scientific question

> **What causal mechanism explains the Q6_K correctness boundary observed when the frozen SA1 dataflow changes accumulation from serial FP32 order to subgroup-32 split-K partial accumulation and subgroup reduction?**

The program does **not** assume in advance that reduction topology is the cause.

## 4. Competing mechanism families

### H-RTCI — Reduction-topology × conditioning interaction

Q6_K value/scale structure and cancellation make FP32 non-associativity materially larger under the split-K reduction tree than under serial FP32 accumulation.

Predicted signature:

- a deterministic CPU emulation of the exact split-32 reduction tree reproduces the excess error direction/magnitude pattern;
- the signal persists when packed dequantization is replaced by mathematically equivalent pre-expanded values;
- error increases systematically in preregistered high-cancellation / high-scale-heterogeneity strata.

### H-PDI — Packed-dequant access interaction

The excess error requires the direct packed Q6_K dequant/access path interacting with lane partitioning; reduction topology alone is insufficient.

Predicted signature:

- split-32 using pre-expanded canonical FP32 weights does not show the same excess error;
- direct-packed split-32 does;
- independent dequant reconstruction invariants remain internally coherent, excluding a trivial format decoder bug.

### H-DSA — Device/subgroup arithmetic contribution

The device/compiler realization of subgroup arithmetic contributes error not reproduced by a deterministic software emulation of the same tree.

Predicted signature:

- CPU fixed-tree split-32 remains within its preregistered error envelope;
- GPU subgroupAdd path exceeds it on the same canonical values;
- pre-expanded GPU split-32 preserves the discrepancy, arguing against packed-dequant as the sole cause.

### H-SEM — Hidden semantic/reference defect

A latent mismatch in packed Q6 semantics, fixture construction, reference construction or indexing survives static review and explains the observed boundary.

Predicted signature:

- an independently specified canonical dequantization/reference path disagrees with the SA1-compatible interpretation before reduction topology is introduced; or
- invariant checks expose row/block/scale/value reconstruction disagreement.

Existing SA1 static evidence weighs against H-SEM, but Q6CB-1 must retain it as a falsifiable alternative rather than declaring it impossible.

### H-NSB — No stable fresh boundary

The SA1 correctness boundary does not reproduce on independently frozen fresh fixtures under a valid causal harness.

Predicted signature:

- fresh split-32 observations do not separate from serial/reference error under preregistered decision rules.

If H-NSB is supported, Q6CB-1 closes without attempting to tune fixtures until a boundary appears.

## 5. Causal-identification contract

A future Q6CB execution, if separately authorized, must bind all compared arms to the **same mathematical Q6_K weight values and input vector** for each fixture.

The minimum causal arm set is:

1. **R64** — canonical high-precision CPU reference after canonical Q6_K dequantization.
2. **S32** — serial FP32 accumulation reference preserving the baseline accumulation topology.
3. **T32-CPU** — deterministic software emulation of the exact 32-way lane partition and frozen reduction tree.
4. **T32-GPU-PACKED** — GPU subgroup-32 split-K using direct packed Q6_K dequantization.
5. **T32-GPU-EXPANDED** — GPU subgroup-32 split-K over pre-expanded canonical FP32 weights, diagnostic only.

No arm is a performance candidate. Timing is outside scope.

Required causal contrasts:

- **Topology contrast:** T32-CPU vs S32.
- **Device contrast:** T32-GPU-EXPANDED vs T32-CPU on the same canonical values.
- **Packed-path contrast:** T32-GPU-PACKED vs T32-GPU-EXPANDED.
- **Conditioning interaction:** topology contrast across independently preregistered conditioning strata.
- **Semantic invariant:** canonical packed reconstruction vs independent canonical decoder before any dot-product reduction.

The future execution contract must freeze exact numeric tolerances and classification rules **before any fresh outcome is observed**. Q6CB-1 specification does not reuse or mutate SA1 correctness thresholds to make a causal hypothesis pass.

If more than one mechanism signature is simultaneously supported, adjudication must return `MULTIFACTOR`; it must not force a single winner. If no signature satisfies its frozen criteria, return `UNRESOLVED_MECHANISM` or `BOUNDARY_NOT_REPRODUCED` as appropriate.

## 6. Fresh-data exclusions

SA1 evidence is consumed historical evidence, not fresh Q6CB confirmatory data.

The future Q6CB primary/confirmatory sets must exclude:

- exact SA1 Q6 cells `Q6_H3584_R512_BIAS` and `Q6_H18944_R3584_NOBIAS`;
- SA1 correctness banks 0 and 3;
- exact SA1 input/weight fixtures, raw outputs and random seeds as primary or confirmatory observations;
- any fixture selected because it is known post hoc to exceed the SA1 correctness threshold;
- any threshold chosen after viewing fresh Q6CB outcomes;
- any rerun of SA1 Q6 preflight as a source of Q6CB fresh evidence.

SA1 artifacts may be used only for origin/provenance and to state the historical boundary.

Before future execution, Q6CB must freeze:

- deterministic fixture-generator specification;
- independent seed derivation rule;
- conditioning strata definitions;
- identification and confirmatory partitions;
- exact fixture counts;
- exact decision tolerances;
- exact evidence schema.

Identification and confirmatory partitions must both be frozen before the first scientific execution.

## 7. Conditioning strata requirement

The fresh fixture generator must span at least the following independently specified numerical regimes:

- low cancellation / bounded dynamic range;
- high cancellation;
- scale heterogeneity;
- mixed-sign high dynamic range;
- neutral random control.

The generator must not search for cases that fail. Its job is to sample from frozen strata, not to mine counterexamples after outcomes are visible.

## 8. Falsification gates

### F0 — Invalid execution/provenance

Infrastructure, source binding, fixture identity, device mismatch or evidence corruption invalidates the affected attempt. F0 is not a scientific FAIL.

### F1 — Fresh boundary existence

If the preregistered fresh set does not reproduce a stable excess-error boundary for split-32 relative to serial/reference under the frozen rule:

`BOUNDARY_NOT_REPRODUCED`

Close Q6CB-1. Do not search for harsher fixtures.

### F2 — H-RTCI signature

H-RTCI requires the frozen topology contrast to reproduce in T32-CPU and persist through the packed-vs-expanded control while varying predictably with conditioning strata.

Failure of those required signatures falsifies H-RTCI as the sufficient explanation.

### F3 — H-PDI signature

H-PDI requires a material direct-packed vs pre-expanded contrast under the same split-32 topology while semantic reconstruction invariants remain valid.

If the packed-path contrast is absent, H-PDI is falsified as the sufficient explanation.

### F4 — H-DSA signature

H-DSA requires GPU split-32 error not reproduced by the deterministic CPU fixed-tree arm on the same values, with the discrepancy persisting in the expanded-weight GPU control.

If CPU and GPU fixed-tree behavior agree within the frozen cross-executor envelope, H-DSA is falsified as the sufficient explanation.

### F5 — H-SEM signature

H-SEM requires an independently specified semantic/reference invariant failure before topology comparison.

If all semantic invariants pass, H-SEM is falsified for the tested scope.

### F6 — Multifactor / unresolved

If multiple sufficient signatures pass, classify `MULTIFACTOR`.  
If the fresh boundary exists but no mechanism signature satisfies its frozen rule, classify `UNRESOLVED_MECHANISM`.

No classification authorizes optimization.

## 9. Finite roadmap

### Q6CB-0 — Specification and zero-science QA

Current stage.

Deliverables:

- this program specification;
- machine-readable contract;
- explicit SA1 immutability/fresh-data policy;
- zero-science QA.

Scientific execution: **forbidden**.

### Q6CB-1 — Causal harness implementation lock

Only after Q6CB-0 PASS.

Allowed work:

- implement the five-arm correctness-only causal harness;
- implement canonical independent Q6 decoder/reference;
- implement deterministic fixture generator;
- unit/static tests;
- compile/BuildOnly tooling;
- evidence packaging.

Forbidden:

- fresh fixture execution;
- GPU dispatch on scientific fixtures;
- timing;
- target-model loading;
- adaptive fixture search.

This stage must end in a static-equivalent QA and exact implementation lock.

### Q6CB-2 — Frozen identification collection

Only after separate explicit execution authorization.

Run exactly the frozen identification partition once under the locked harness. No tuning.

Adjudicate F0/F1 and the preregistered mechanism-selection rule.

### Q6CB-3 — Frozen confirmatory collection

Only if the Q6CB-2 outcome meets the predeclared rule that permits confirmation.

The confirmatory partition must already have been frozen before Q6CB-2.

No contract or threshold changes are allowed between Q6CB-2 and Q6CB-3.

### Q6CB-4 — Final adjudication and close

Allowed final classes:

- `BOUNDARY_NOT_REPRODUCED`
- `H_RTCI_SUPPORTED`
- `H_PDI_SUPPORTED`
- `H_DSA_SUPPORTED`
- `H_SEM_SUPPORTED`
- `MULTIFACTOR`
- `UNRESOLVED_MECHANISM`
- `INVALID_F0` only when evidence genuinely invalidates execution

Q6CB-1 closes after final adjudication. Any repair/optimization successor requires a new program and cannot alter SA1 or Q6CB history.

## 10. Scope prohibitions

Throughout Q6CB-1:

- no SA1 source/evidence mutation;
- no Q6 performance timing;
- no target-model integration;
- no Q3 reopen;
- no kernel search;
- no workgroup/subgroup-size search;
- no threshold search;
- no candidate performance comparison;
- no cooperative-matrix optimization;
- no fusion optimization;
- no pre-dequant cache optimization;
- no claim that one mechanism is causal before adjudication;
- no tuning to PASS.

## 11. Relationship to ArcLLM central governance

Q6CB-1 is a bounded post-SA1 mechanism-identification program. It does not reopen the previously closed ArcLLM architecture verdict and does not substitute a microbenchmark for end-to-end evidence.

Its only allowed scientific contribution is to determine whether the Q6 correctness boundary has a reproducible causal mechanism. Any later claim about architecture advantage, target-model feasibility or end-to-end performance must return to the separate ArcLLM Q1/Q2/Q3 governance path and requires independent authorization.

## 12. Current authorization state

```text
specification_write              = true
zero_science_QA                  = true
causal_harness_implementation    = false
fresh_fixture_generation_run     = false
scientific_CPU_execution         = false
scientific_GPU_execution         = false
performance_timing               = false
target_model_load                = false
SA1_rerun                        = false
```

The next gate after this document is **zero-science QA of the specification**, not execution.
