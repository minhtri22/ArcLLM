# Phase 2 Real Second-Family Generalization Gate

## Question

Can the frozen Generic Primitive Registry plus Generic Policy Engine represent and make the correct decisions for a real second primitive family, materially different in operator, representation, acquisition and lifetime, without modifying the frozen generic machinery?

## Candidate selection

I002 was rejected as the primary test candidate despite being a real carry-through PASS. It is too close to the first family for a strong generalization test: it is another Q4_K subgroup Split-K execution intervention and does not introduce an independent representation-acquisition/lifetime model.

The selected candidate is the historical P8 segmented-residency / graph-binding family.

## Frozen P8 evidence

- P8-A contiguous 256 MiB arena plan: **FAIL**. Total capacity passes, but `token_embd.weight` and `output.weight` violate the single-tensor arena contract.
- P8-A2 segmented plan: **PASS**.
- P8-B exact 5,347,770,372-byte simultaneous residency: **PASS**.
- P8-C segmented embedding/LM-head GPU access: **PASS**.
- P8-D graph-level segmented tensor binding: **PASS**.
- P8-E one-layer correctness: **PASS**.
- P8-F two-layer correctness: **PASS**.
- P8-G larger-prefix correctness: **FAIL**.
- Later P8-G6 diagnostics preserve the P8-G verdict as FAIL; full inference remains forbidden.

The family is therefore a bounded validated representation/residency/access family, not a full-inference performance primitive.

## Why it is structurally different

The first registered family has an active no-extra-representation fallback A and an optional B representation whose acquisition is amortized by future reuse.

P8 is different:

1. Under the frozen 256 MiB arena cap, the contiguous representation is not a validated fallback; it failed.
2. Segmentation is required for feasibility, not selected because a reuse horizon makes it profitable.
3. Acquisition means constructing/allocating/copying a multi-arena residency plan, not creating an optional per-operator optimized representation.
4. Lifetime is model/graph residency.
5. The validated surface spans embedding, LM-head and graph binding, with a bounded correctness frontier.

## Falsification test

### Encoding A — evidence-faithful disabled fallback

Register the failed contiguous representation as `DISABLED_BY_EVIDENCE`, and the segmented representation as the validated preferred primitive.

The frozen generic policy rejects the capability as `REGISTRY_POLICY_INCOMPLETE` because it requires the capability fallback primitive itself to be active/validated.

### Encoding B — segmented representation as its own fallback

This avoids the active-fallback check, but creates a semantic error. When the segmented representation is absent and future reuse is unknown or zero, the frozen generic engine returns the segmented primitive with no acquisition action.

For P8, that representation cannot be used before the residency acquisition is performed. Acquisition is a feasibility prerequisite and has no evidence-backed reuse threshold.

The engine only reaches the acquisition path after positive/known reuse metadata is supplied, which would be an invented P8 condition.

## Gate interpretation

This is a real counterexample to the current abstraction. It does not invalidate the registry or generic policy work already completed for amortized optional-representation families. It establishes that the abstraction is not yet general across mandatory feasibility-enabling representation families.

No frozen generic file is modified during this gate.

## Missing abstraction

A redesign must be able to express at least:

- capability with **no validated fallback execution primitive**;
- acquisition trigger class `MANDATORY_FOR_FEASIBILITY`, distinct from reuse-amortized acquisition;
- unavailable/not-ready as a legitimate decision rather than routing a nonresident fallback;
- evidence-bounded capability scope so P8-G/full-inference FAIL is not promoted into a broader PASS.

The redesign must then rerun both:
- first-family exhaustive equivalence; and
- this P8 real-family gate.

Only after both pass should backend binding proceed.
