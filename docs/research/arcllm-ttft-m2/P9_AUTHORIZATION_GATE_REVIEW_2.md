# TTFT_M2 — P9 Explicit Fresh Mechanism-Identification Authorization Gate — Review 2

**Date:** 2026-09-22  
**Result:** `PASS_M2_P9_AUTHORIZATION_GATE_REVIEW_2`

## Decision

`AUTHORIZE_M2_P9_SCIENTIFIC_HARNESS_IMPLEMENTATION_AND_BUILDONLY_ONLY`

P9 is now opened, but only in an **implementation / zero-science BuildOnly** state.

Scientific execution remains blocked.

## Why Review 2 passes

The two required prerequisites now exist:

1. P8L demonstrated that the exact frozen M2 infrastructure package executes correctly on the local Windows substrate.
2. P9A created a fresh, canonical M2 scientific design rather than silently inheriting M1 templates.

Bindings:

```text
P8L adjudication
b1eda822a3580370ebdeda704af8a169c4991dec

P9A adjudication
0eccfd37a85738c1d05babaf36f46b66c6b17219

P9A canonical design manifest
abce0545cec33367f9b3a82f15d5ffd4fb026f64
```

Canonical P9 design members:

```text
source revalidation   17759ae80ce0cbbfb4d2dedd88c0aabe8216ac9c
hypotheses             55192815d02637cc6e469794565ba8134ff63ae1
causal design          9d1960ee253e59f1835ea0d320b40e9975039ed3
target environment     5b79f7b42b238471e0d4bb3ee802bab7ffe83185
falsification / STOP   1ed4187934be119611f9fd1f0ce9f8e61bf88116
```

The design is therefore scientifically ready to be implemented without changing hypotheses, factors, workloads, endpoints, thresholds, session orders, or STOP logic.

## Why execution is not yet authorized

P9A froze the **scientific design**, not an executable scientific package.

There is currently no fresh M2-canonical binding for:

- diagnostic harness implementation;
- build tool;
- shader-build package;
- resulting executable SHA256;
- session/order orchestration runner.

The historical M1 implementation remains useful only as methodological / infrastructure evidence:

```text
M1 diagnostic source
88d97ddb497bfddcec191358f1e21d982c5efccf

M1 build tool
1c65d419fb664889bcefc452c509628eff6858a6

M1 shader compile tool
144a48b7ac14ab1e44427cf0df10b13a2a82966e

M1 static test
66212bd0fb369e9866f98c3462c7f5c13e2d2594

M1 lock v0.2
e2eb6170933ee074bfb3446661b7d69d3d40eaa9

M1 BuildOnly runner
4f7500bb2b0e3bc0b334a3f41c1244fcdc55d545
```

The M1 lock/runner are not M2 canonical authority and must not be directly promoted to scientific execution.

This is especially important because the historical M1 execution package terminated with a provenance defect.

## Authorized by this gate

Review 2 authorizes only:

- fresh M2 scientific-harness implementation bound to the P9A canonical design;
- BuildOnly compilation;
- static H-ART artifact-identity qualification;
- execution-package QA and provenance freeze.

Review 2 does **not** authorize:

```text
diagnostic executable launch  false
target model load             false
GPU dispatch                  false
performance measurement       false
fresh TTFT observation        false
mechanism adjudication        false
```

No scientific outcome has been exposed.

## Required implementation boundary

The next package must:

1. bind the exact P9A canonical blobs;
2. create M2-native implementation/build/orchestration provenance;
3. preserve the 2×2 SP/SF/QP/QF design exactly;
4. preserve W-S/W-C, 2 sessions, frozen A/B order, 5 attempts/cell and 80 planned observations;
5. preserve the 1.10 materiality threshold;
6. enforce H-ART static-first behavior;
7. produce a zero-science BuildOnly result;
8. fail closed on provenance or package inconsistency.

A separate explicit execution authorization is required after BuildOnly qualification.

## Next admissible step

`M2_P9B_CANONICAL_SCIENTIFIC_HARNESS_IMPLEMENTATION_AND_BUILDONLY_QUALIFICATION`

P9 is open for implementation only. Model/GPU/timing execution remains forbidden.
