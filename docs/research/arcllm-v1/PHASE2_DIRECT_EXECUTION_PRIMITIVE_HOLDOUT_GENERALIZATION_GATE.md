# Phase 2 Direct-Execution Primitive Holdout Generalization Gate

## Scientific question

Can frozen capability/acquisition semantics v2 represent an independently validated direct-execution primitive family whose preferred primitive requires no new representation, no acquisition action, and no residency lifetime?

## Holdout

The holdout is closed I002: Q4_K FFN gate/up subgroup32 Split-K decode execution.

I002 predates the v2 redesign and was not used to create the distinction between reuse-amortized acquisition and mandatory-feasibility acquisition.

Frozen I002 evidence includes real-model semantic equality for 20/20 measured pairs, approximately 2.20x global decode geomean speedup, approximately 2.00x global E2E geomean median speedup, and TTFT non-regression PASS.

The candidate changes execution work partitioning at 56 decode gate/up nodes. It does not create a new represented tensor image, require additional representation residency, or perform an acquisition action.

## Correct oracle

Inside the bounded I002 domain:

- if the validated direct primitive is executable, route it immediately with lifecycle NONE;
- if it is unavailable, route the independently valid baseline fallback;
- future reuse is irrelevant;
- no acquisition is needed;
- no representation residency is needed.

Outside the bounded I002 evidence scope, route the baseline fallback.

## Frozen-v2 counterexample

An evidence-faithful registration can express the preferred direct primitive, the baseline fallback, and zero acquisition paths.

However, the frozen v2 engine only enters preferred-primitive reuse through a runtime state with `resident=true`. A nonresident preferred primitive then falls through to acquisition scanning. With no acquisition paths, the engine selects the fallback.

Therefore an available direct execution primitive is not selectable unless one of the following unsupported workarounds is used:

1. set `resident=true` even though no represented residency exists;
2. invent an acquisition action;
3. make the preferred primitive its own fallback and lose the actual baseline fallback.

The first is semantic overloading, the second invents a mechanism absent from evidence, and the third loses a required decision branch.

## Interpretation

If reproduced on the frozen v2 files, this is a holdout failure. It means v2 successfully covers the two family classes used in its redesign, but readiness is still coupled to representation residency/acquisition.

The missing generic concept is direct execution readiness independent of representation residency.

No backend binding is authorized by this gate.
