# TTFT_M2 — Atomic Package QA Contract

## Principle

The future TTFT_M2 execution package is not a collection of independently trusted files. It is one closed object whose cross-file relationships must be mechanically proven before freeze.

The minimum object is:

```text
runner
+ execution/build lock
+ success result schema
+ failure result schema
+ evidence manifest schema
+ package manifest schema
+ bundle member list
+ source -> destination filename map
+ Git blob provenance
+ preflight references
+ postflight/result references
+ version labels
+ output bundle name
+ adjudicator input contract
+ atomic QA tool + fixtures
```

## Why this is mandatory

TTFT_M1 showed that a runner can correctly read the new lock in preflight while still emitting stale provenance and stale destination labels later in the same file. File-by-file QA therefore does not establish package validity.

## Cross-file invariants

P5 must mechanically prove that the authoritative lock/version/blob observed by the runner is the same lock/version/blob:

- reported by success result;
- reported by failure result;
- copied into evidence;
- named by the evidence manifest;
- named by the package manifest;
- labeled in the destination filename;
- expected by the adjudicator-input contract;
- expected by the QA suite.

The same principle applies to runner identity, authorization identity, schema IDs/versions, source blobs, required bundle members and output bundle name.

Any stale or contradictory reference is a package failure.

## Required negative fixtures

P4 must implement synthetic/static fixtures for at least:

1. stale lock version;
2. wrong destination filename;
3. wrong Git blob;
4. missing evidence member;
5. duplicate evidence member;
6. result-schema mismatch;
7. runner/lock mismatch;
8. manifest/runner mismatch;
9. package/result mismatch;
10. authority comparison accidentally using the same source on both sides.

P5 passes only if every negative fixture is rejected fail-closed and the canonical positive fixture passes.

## Zero-science boundary

Atomic QA must not load the target model, launch the diagnostic executable, dispatch GPU work, collect performance timing, or create a scientific outcome. Synthetic fixtures and static repository inspection are sufficient.

## Repair-budget timing

Defects discovered in P4/P5 are package-development defects. They are fixed and QA is rerun without consuming the execution-stage repair budget.

The execution-stage budget becomes live only after:
- exact package PASS at P5;
- zero-science governance PASS at P6;
- exact package freeze; and
- explicit BuildOnly authorization at P7.

## Freeze discipline

The package cannot be declared frozen while any member or reference remains provisional. After freeze, any execution-stage infrastructure repair must follow the separately preregistered one-repair rule and full replay semantics.

## Current state

This document is a P3 specification. No scientific runner has been created by P3 and no execution is authorized.

Next:

`M2_P4_ZERO_SCIENCE_PACKAGE_IMPLEMENTATION`
