# Phase 2 Capability / Acquisition Semantics Redesign Gate

## Scientific question

Can a minimal generic redesign cover both already-evidenced family classes without family-specific policy code?

The two classes are:

1. optional, reuse-amortized representation with a validated fallback (A/B/P1 family);
2. mandatory feasibility-enabling representation with no validated fallback (P8 segmented family).

## Candidate semantics

The redesign changes only the dimensions exposed by the failed real-family generalization gate.

A capability has one preferred primitive and an optional fallback primitive.

An evidence profile may have no reuse metric.

Each acquisition path declares one trigger class:

- `REUSE_AMORTIZED`: selection uses the profile's evidence-backed reuse metric and threshold;
- `MANDATORY_FOR_FEASIBILITY`: acquisition is required to make the capability routable and is explicitly forbidden from carrying a reuse threshold.

The engine can return no route with:

- `NOT_READY` when a required representation cannot currently be acquired;
- `OUTSIDE_VALIDATED_CAPABILITY` when the request is outside the frozen evidence scope.

## Non-goals

This gate does not bind any backend, execute any model, reopen placement research, invent a global resource threshold, or promote P8 beyond its frozen correctness frontier.

The existing registry/policy v1 files remain immutable during the gate. Candidate v2 files are isolated until adjudication.

## First-family requirement

The full 114,688-state matrix is evaluated through both frozen v1 and candidate v2. The following fields must match exactly: status, route, lifecycle, acquisition path, threshold metadata, requested residency bytes, preservation flag, and post-acquisition identity-validation requirement.

This prevents the P8 fix from silently changing B1 behavior.

## P8 requirement

P8 is registered with:

- no fallback primitive;
- no reuse metric;
- no reuse threshold;
- one mandatory feasibility acquisition path;
- exact 5,347,770,372-byte validated residency request;
- bounded evidence scope no broader than the frozen PASS frontier through P8-F.

When absent and resources/path are available, P8 must ACQUIRE even when future reuse is unknown or zero. When acquisition is unavailable it must return NOT_READY with no route. When outside the bounded evidence scope it must return OUTSIDE_VALIDATED_CAPABILITY with no route. A valid resident representation remains usable without inventing zero-reuse eviction semantics.

## Adjudication

PASS means one generic semantic model covers both currently demonstrated family classes without modifying frozen v1 or adding family-specific branches.

PASS does not establish universality across all future primitive families. It only repairs the exact abstraction class falsified by the real P8 counterexample.
