# ArcLLM v1 — I002 Zero-Science Build Preflight Adjudication

**Date:** 2026-09-23  
**Program:** `I002-Q4-GU-SG32`  
**Preflight HEAD:** `cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1`  
**Returned ZIP SHA256:** `9AC9C320206A6AF1B99A56DED3474F309A4FFC3A5E3ADD7C212C84A0107531E5`  
**Result:** `PASS_I002_ZERO_SCIENCE_PACKAGE_BUILD`

## 1. Bundle integrity

Returned files:

- `arcllm_v1_i002_execution_lock_v0.1.1.json`
- `I002_PREFLIGHT_REPORT.json`
- `i002_shader_provenance.json`
- `q2_arcllm_shader_provenance.json`

File SHA256:

```text
lock
B165EDA3C32E6214E97B02256F536F145E0E3E3B40C55E05D157AE8C0CA5672C

preflight report
9485E07C8F430C5C78E368725B3DBC74A3FDDD549C810806CE317D9AC0D53C09

I002 shader provenance
9EBAD2448C1F835A47838EDC8430C3E24F3A70C6B9CC4F4185A24330FBA77923

Q2 shader provenance
B94F0F142028A43900AB82CF82E7168F17EFD0C88C0C779CA3C619A5B15F24E9
```

The provenance hashes referenced by the report reproduce exactly from the returned files.

## 2. Exact execution identity

Preflight report:

```text
git_head = cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1
branch   = research/arcllm-v1
```

Repository audit at that HEAD:

```text
critical blobs = 23
mismatches     = 0
```

The returned lock is the exact `v0.1.1` scientific package contract.

## 3. Zero-science state

The returned report records:

```text
science_executed    = false
model_loaded        = false
gpu_dispatches      = 0
timing_observations = 0
```

and:

`fresh_target_model_execution_authorized = false`

Therefore the build/preflight step consumed no I002 scientific observations.

## 4. Candidate provenance

Exact candidate:

```text
source blob
56999d88dc1bef6486e7e1908982f6de4b0f9f6a

SPIR-V SHA256
B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569
```

The returned I002 provenance independently states that the compiled candidate SPIR-V hash equals the historical SA1 SPIR-V hash.

Thus the bounded package uses the exact frozen SA1 subgroup32 mechanism rather than a new derivative.

## 5. Baseline shader provenance

The returned Q2 provenance contains exactly 16 compiled baseline shaders.

The report's Q2 provenance SHA256 equals the independently recomputed hash of the returned Q2 provenance file.

No baseline shader substitution is introduced by preflight.

## 6. Native build qualification

The local Windows preflight successfully produced:

```text
T1 executable
SHA256 = 3D3F6A5141BB505637E0429948D83FB3A73570C583CD1EB818320CD6A9A00C5E
bytes  = 452608

T3 executable
SHA256 = 5B067B047FD069C16C2669EDF52248552FA698B391413FDE91748422CA7FE9BD
bytes  = 443392
```

This closes the compile/build uncertainty left after source-only QA.

## 7. T0 decision

All T0 package requirements available to preflight pass:

- exact candidate provenance;
- exact 56-node static scope;
- exact prefill/non-target static guards;
- Python syntax/static QA;
- exact baseline shader provenance;
- exact SA1 candidate shader compile/hash;
- native Windows build of T1 and T3 harnesses;
- science remained locked.

Formal result:

`PASS_I002_ZERO_SCIENCE_PACKAGE_BUILD`

## 8. Authorization consequence

The full implementation package is now qualified for the **next stage only**:

`T1 REAL-MODEL COMPONENT TRANSFER`

T3 is not authorized.

The T1 authorization must bind:

- implementation parent HEAD `cf3580c4...`;
- T1 executable SHA256 above;
- exact candidate SPIR-V SHA256;
- exact target model SHA/bytes;
- exact environment;
- unchanged T1 implementation/source blobs.

A science-authorization commit may be a descendant of the implementation parent. Therefore the T1 runner must bind the implementation payload by exact blob/executable hashes rather than require the authorization commit itself to equal the preflight implementation HEAD.
