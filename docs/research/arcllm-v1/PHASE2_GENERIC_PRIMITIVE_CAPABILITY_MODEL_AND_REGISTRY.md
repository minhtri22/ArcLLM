# Phase 2 Generic Primitive Capability Model and Registry

## Decision

ArcLLM does not treat the validated A/B/P1 family as universal. It treats that family as the first fully evidenced registration in a generic primitive registry.

The architecture generalizes the **machinery**, not the scientific claim.

## Separation

```
validated evidence
      ↓
family registration bundle
      ↓
generic primitive registry
      ↓
generic policy engine (next)
      ↓
backend binding
```

The generic registry core contains no A/B/P1/P3, Q4_K, EXEC148, W-S/W-C, residency-byte constant, or family-specific threshold.

All such facts live in the reference family registration.

## Generic descriptors

A family can register opaque domains, reuse metrics, evidence sets/profiles, capabilities, primitives, acquisition paths, acquisition thresholds, resident preferences, lifecycle rules, validation state, and provenance.

The registry does not infer scientific validity. It records the validation state supplied by frozen evidence and rejects structurally invalid bundles.

## Evidence state

PASS and FAIL knowledge are retained. An acquisition path may be present as `DISABLED_BY_EVIDENCE`; it is not deleted merely because it is inactive.

This lets the registry preserve scientific lineage while a future generic policy engine selects only admissible states.

## Scaling rule

Adding another primitive family must not require:

- adding new public enum cases;
- editing generic registry lookup/validation code;
- creating a new package;
- hard-coding new thresholds into the core.

It requires a new registration bundle with unique opaque IDs, valid descriptor relations, and provenance.

A zero-science synthetic second-family registration is the structural acceptance test for this rule.

## Legacy Phase 2 v1 package

The existing hard-coded package API v1 remains frozen as a reference compatibility artifact. It is no longer the extension point for new primitive families.

No new family-specific enums should be added there.

## Boundary

This step does not yet implement the generic policy engine. Registry data is now sufficient for that engine to query candidates, thresholds, resident preference, lifecycle semantics, evidence state, and provenance without knowing family-specific names.

Next: `PHASE2_GENERIC_POLICY_ENGINE_OVER_REGISTRY`.
