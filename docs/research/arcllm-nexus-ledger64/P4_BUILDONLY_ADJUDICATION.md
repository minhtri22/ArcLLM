# ANL64 P4 BuildOnly Adjudication

**Date:** 2026-09-21  
**Verdict:** `P4_BUILDONLY_PASS`

The returned BuildOnly bundle was independently inspected before any P5 work.

## Integrity

Returned bundle SHA256:

```text
F336519DF14EAEC95F43A5CFC7680C9309F6560B6A60BCEEBA949A41ECEAACE2
```

The archive contains exactly seven expected evidence files. The Git-blob identities of the bundled authorization, repaired implementation lock, repaired static QA, and repair adjudication independently reproduce the repository blobs bound at the exact execution head.

Run head:

```text
aa284326a9322a14888967fa4a9a9dc626f2286a
```

Bound identities:

```text
authorization     c0a9c77d09c7da98103fc949ede182f117bd6329
lock v0.2        60b1428ad0f919df4fb3a993ccc289f5d5a003c2
runner           f33179a96f83868acc77e808a6fa508311e2fd7e
static QA v0.2   c3faaf04ef7f91adadd9dce93b95cf6d6cebe3ba
repair record    c50f9bb906293827c7657838a86d2d64a2a8a099
```

The source blobs recorded by the native build match the exact repository state:
- ANL64 planner `157be15c...`;
- ANL64 runtime `dbcb7afe...`;
- GGUF source `bda721b3...`;
- tensor store `0eb52588...`.

Legacy Q2 and inherited Q4 shaders remain unchanged.

## Shader BuildOnly

All 17 shaders compiled using:
- glslang 16.5.0;
- pinned compiler asset SHA256 `06B71298...`;
- Vulkan 1.2 target.

Critical reproduction:

```text
Q4_FAST
B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569
```

This exactly reproduces the closed SA1 Q4 candidate SPIR-V identity.

The frozen Q4-safe shader also reproduces:

```text
2EFD94ACDA45555AF1C082C916AF7C3F1AA1BE46AD7868B7C4BE868816CAEE4A
```

## Native BuildOnly

Native build PASS.

```text
anl64_p4.exe SHA256
1F45DA9D8CACE3CF78FE31B7B7041B6027CB41E180F7FC99E50E1127EA4F5451

bytes
449024
```

The binary itself is not present in the returned ZIP. This is acceptable for this BuildOnly gate because the exact verified runner fail-closed on executable existence, independently recomputed the binary SHA256, checked it against the native-build manifest, and only then created the bundle. No P4 decision relies on executable behavior.

## Exact target

The build was produced on the bound target:
- Windows build 26200;
- Intel Core Ultra 7 258V;
- Intel Arc 140V GPU;
- driver 32.0.101.8860.

## Zero-science boundary

The bundle explicitly records:

```text
executable_launched      false
model_loaded             false
gpu_dispatch             false
performance_measurement  false
scientific_outcome       false
```

Therefore P4 does not consume P5/P6 evidence.

## Repair accounting

The earlier static-QA failure is retained as historical evidence and remains classified:

```text
BUILDONLY_STATIC_QA_FALSE_NEGATIVE_NO_SCIENCE_CONSUMED
```

Exactly one bounded P4 BuildOnly QA repair was consumed. Production/runtime/shader blobs did not change.

## Final P4 adjudication

```text
static package QA         PASS
shader BuildOnly          PASS
native BuildOnly          PASS
exact provenance          PASS
zero-science boundary     PASS
production immutability   PASS
--------------------------------
P4_BUILDONLY_PASS
```

P4 is now closed as:

`P4_IMPLEMENTATION_AND_BUILDONLY_CLOSED_PASS`.

No performance or end-to-end advantage claim is made.

The only next stage that may open is P5 component-integration validation **specification first**. P5 execution remains unauthorized until its correctness/reference contract passes zero-science QA and receives a separate explicit authorization.
