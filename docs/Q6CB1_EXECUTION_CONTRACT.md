# Q6CB-1 Execution Contract v0.1

**Status:** FROZEN CANDIDATE — ZERO-SCIENCE QA REQUIRED  
**Fresh science:** NOT AUTHORIZED  
**Parent implementation/evidence lock:** `237a4174b409bf2e2200fa96fe1a7dbccfa76bab`

## Purpose

This document freezes the complete Q6CB fresh-data design before any Q6CB scientific fixture is generated or executed. It serves only the already-frozen causal question: whether the Q6_K correctness boundary has a reproducible causal mechanism among H-RTCI, H-PDI, H-DSA, H-SEM, or no stable fresh boundary.

Nothing in this contract reopens SA1 or authorizes performance work.

## Frozen fresh design

Two fresh K-length regimes are used:

- `K4096_R64`: `n=4096, rows=64`;
- `K16384_R64`: `n=16384, rows=64`.

Both K lengths are multiples of the 256-value Q6 block and neither duplicates an SA1 Q6 cell. Rows are held at 64 because the studied mechanism is per-output-row accumulation/reduction; row count supplies independent row observations without reproducing an SA1 shape.

For each partition:

```text
2 shapes
× 5 frozen conditioning strata
× 3 independently derived seeds
= 30 fixtures
= 1,920 output rows
```

Identification and confirmatory partitions are both frozen now. Confirmatory data are not generated unless the later Q6CB-2 adjudication opens the predeclared Q6CB-3 gate.

## Seed derivation

For every fixture the seed label is:

```text
ArcLLM|Q6CB1|v0.1|{PARTITION}|{SHAPE_ID}|{STRATUM}|R{REPLICATE}
```

Compute SHA-256 over UTF-8 and interpret digest bytes 0..7 as unsigned little-endian uint64.

The exact ordered seed lists are stored in the machine contract. The canonical 60-seed schedule has SHA-256:

```text
C6C47227DA52D41F485A21319DEAB81514A6A7555D0EFAD8113D75F304BA273E
```

No seed duplicates another Q6CB seed or any of the four consumed SA1 seeds.

## Numerical contract

No SA1 absolute threshold is reused.

For each fixture:

```text
S     = max(RMS(R64), 1.0)
delta = S × 2^-20
```

For an arm A to show a material excess over arm B, both conditions must hold:

```text
RMSE(A,B)   >= 4 × delta

RMSE(A,R64) >= 2 × max(RMSE(B,R64), delta)
```

The `2^-20` floor is sixteen binary32 unit-roundoffs relative to the reference scale; the separate `4×delta` contrast requirement prevents a ratio from becoming positive only because the comparator error is near zero. The 2× excess-error criterion requires a large directional separation rather than ordinary rounding variation.

The same formulas apply to both shapes, all five strata and both partitions. No outlier deletion is allowed.

## Stable boundary rule — F1

Per fixture, the boundary signature is:

```text
material_excess(T32_GPU_PACKED, S32)
```

A partition reproduces a stable boundary only when all are true:

1. at least two of five strata group-pass for `K4096_R64`;
2. at least two of five strata group-pass for `K16384_R64`;
3. the two shapes share at least one group-passing stress stratum.

A shape/stratum group passes only if at least 2/3 frozen replicates pass.

Failure at identification closes Q6CB as `BOUNDARY_NOT_REPRODUCED`; no harsher fixture search is permitted.

## Mechanism signatures

### H-RTCI

H-RTCI requires both the deterministic CPU topology contrast and the pre-expanded GPU topology to show material excess over S32 in both `HIGH_CANCELLATION` and `SCALE_HETEROGENEITY`, for both shapes.

Additionally, within each shape:

```text
median R_TOPOLOGY_CPU over
  HIGH_CANCELLATION + SCALE_HETEROGENEITY
------------------------------------------------ >= 1.5
max(median R_TOPOLOGY_CPU over
  LOW_CANCELLATION + NEUTRAL_RANDOM_CONTROL, 1.0)
```

This makes the conditioning interaction necessary rather than treating any topology difference as RTCI.

### H-PDI

H-PDI requires semantic reconstruction to remain exact and:

```text
material_excess(T32_GPU_PACKED, T32_GPU_EXPANDED)
```

to group-pass in at least two strata per shape, with at least one common stress stratum across shapes.

### H-DSA

H-DSA requires:

```text
material_excess(T32_GPU_EXPANDED, T32_CPU)
```

to group-pass in at least two strata per shape, with at least one common stress stratum across shapes.

### H-SEM

The semantic invariant independently compares generated intent against packed reconstruction for q, scale and d bits.

H-SEM becomes a candidate only when the same mismatch class is nonzero in at least 2/3 replicates in one stratum and that same mismatch class/stratum replicates in both K-length shapes.

A single isolated mismatch is not enough to establish H-SEM.

## Identification → confirmation gate

Q6CB-2 adjudication order is fixed:

```text
F0
 ↓
F1
 ↓
F2 / F3 / F4 / F5
 ↓
F6
```

Outcomes:

```text
F0
→ INVALID_F0; no scientific adjudication

F1 FAIL
→ BOUNDARY_NOT_REPRODUCED; CLOSE

F1 PASS + no mechanism candidate
→ UNRESOLVED_MECHANISM; CLOSE

F1 PASS + >=1 mechanism candidate
→ confirmatory eligible
→ still requires a separate explicit Q6CB-3 authorization
```

The identification candidate set is frozen before confirmation. A mechanism that appears only in confirmatory data cannot be promoted.

## Confirmatory adjudication

The confirmatory partition uses exactly the same metrics, thresholds and group rules.

```text
confirmatory F1 FAIL
→ BOUNDARY_NOT_REPRODUCED

supported_set =
  identification candidates ∩ confirmatory candidates

|supported_set| = 0
→ UNRESOLVED_MECHANISM

|supported_set| = 1
→ corresponding H_*_SUPPORTED

|supported_set| > 1
→ MULTIFACTOR
```

Every valid Q6CB-4 classification is terminal.

## F0 and reruns

F0 includes provenance/hash mismatch, fixture/seed/order mismatch, target environment mismatch, corrupt evidence, selective replay, or execution without the future explicit authorization.

A valid scientific fixture is never selectively rerun.

If a genuine F0 occurs, governance permits at most one infrastructure-only repair for that stage. The entire frozen partition—not selected fixtures—must then be repeated under the unchanged scientific contract. A second F0 terminates as `STOP_INFRASTRUCTURE_UNSTABLE`.

## Frozen target environment

Hard causal/runtime identity is bound to the previously qualified target:

- Intel Core Ultra 7 258V;
- Intel Arc 140V GPU (16GB);
- Intel vendor ID 32902;
- Windows GPU driver `32.0.101.8860`;
- Vulkan driver info `101.8860`;
- Vulkan device API >= 1.2;
- compute subgroup size 32 with basic + arithmetic operations;
- exact executable and both SPIR-V SHA-256 identities.

Windows build, Vulkan loader/exact device-API version, AC/battery state, power scheme and memory census are recorded descriptively rather than used as hard F0 gates. This is a correctness-only mechanism study, so non-causal timing/power state must not create an artificial invalidation.

No model is loaded and no timing is scientifically used.

## Evidence contract

Every fixture must retain the raw five-arm vectors, semantic invariant, fixture/seed identity, raw file SHA-256, packed/x hashes, process exit code and canonical ordinal.

Each partition must produce a manifest binding the execution contract, implementation evidence lock, executable/SPIR-V hashes, target environment, all 30 raw result hashes and a declaration that no valid fixture was selectively rerun or inspected to alter the contract.

Adjudication must emit all derived fixture metrics, all group gates and every F0–F6 decision.

## Current authorization boundary

```text
execution-contract specification = allowed
zero-science QA                = allowed

fresh fixture generation       = FORBIDDEN
CPU scientific execution       = FORBIDDEN
GPU scientific dispatch        = FORBIDDEN
performance timing             = FORBIDDEN
target-model loading           = FORBIDDEN
execution authorization        = ABSENT
```

The only next action is independent zero-science QA of this contract. A PASS may freeze the execution contract, but still does not authorize Q6CB-2.
