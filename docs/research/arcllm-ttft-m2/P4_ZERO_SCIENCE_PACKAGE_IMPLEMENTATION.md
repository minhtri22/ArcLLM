# TTFT_M2 — P4 Zero-Science Package Implementation

**Date:** 2026-09-22  
**Result:** `M2_P4_ZERO_SCIENCE_INFRASTRUCTURE_PACKAGE_IMPLEMENTED`

## Authorization

P4 was implemented only after the explicit bounded authorization:

- `config/arcllm_ttft_m2_p4_implementation_authorization_v0.1.json`
- blob `6ad37531ca81227e65420b1261780ebfafbca97a`

The authorization permits package materialization and static/synthetic QA infrastructure only. It does not authorize execution.

## Implemented atomic package

Canonical execution lock:

- `config/arcllm_ttft_m2_execution_lock_v0.1.json`
- blob `9af4c7b4c96354223e1f671a43af64b215072330`

Runner source:

- `run_ttft_m2_buildonly.ps1`
- blob `4722e86a01453d973ee2122b49229b80bf7d84f6`

The runner is deliberately P7-gated. Without a future exact `M2_P7_BUILDONLY_AUTHORIZED` artifact it fails closed before package execution.

Manifests:

- evidence template blob `bf496e00e4cf0bff86582e0649c6c26bc28b6f60`
- package manifest blob `4629910255580322706b01f318ad9a80044208d7`

Schemas:

- success result `1c216e1bf6e71677d82b658197f2e90302f53f9e`
- failure result `7cc8947790f9a11d76af144707128fc1df78f7ae`
- evidence manifest `a6cbf83ffe3b0e81f6d5c03257bba7ba8fbc9d18`
- package manifest `ec8d8c00162cce0de9fd564c32ea1e34ac679b59`
- adjudicator input `713ba835364132ceb903e309570b3ca8d36c4c6a`

Atomic QA tool:

- `tools/ttft_m2_atomic_package_qa.py`
- blob `73b74f2dbc65f0e1bff7cd2c226b4112c34ecfdb`

Fixture index:

- `tests/fixtures/ttft_m2_atomic_package/index.json`
- blob `9735e97d5ea264642725466e50ae26a00e8a4951`

The canonical positive fixture plus all ten required negative fixtures are materialized.

## Provenance model

P4 avoids self-hash cycles without weakening provenance.

Static committed members are bound by exact Git blob.

The package manifest cannot contain its own blob without an impossible fixed-point, so its package-member entry is `RUNTIME_REQUIRED_EXACT_GIT_BLOB`; P5/P6 will externally bind its final committed blob.

The future P7 authorization uses the same exact runtime Git-binding mode because it does not exist at P4.

Generated result/evidence artifacts are schema-bound runtime outputs.

The adjudicator input is intentionally a sidecar generated **after** the ZIP hash is known. It is not placed inside the ZIP it hashes.

## Negative fixtures implemented

The QA harness has fail-closed fixtures for:

1. stale lock version;
2. wrong bundle destination filename;
3. wrong Git blob field;
4. missing evidence member;
5. duplicate evidence member;
6. result-schema version mismatch;
7. runner/lock mismatch;
8. manifest/runner mismatch;
9. package/result mismatch;
10. same-source authority tautology.

## Scientific boundary

P4 did not run the atomic QA as P5 and did not execute the runner.

Accounting remains:

```text
BuildOnly runner executions          0
diagnostic executable launches       0
target model loads                    0
GPU dispatches                        0
performance measurements              0
fresh TTFT observations               0
scientific mechanism result           NONE
```

No TTFT_M1 mechanism result was inherited. No M1 2x2 design was silently canonicalized. The P4 lock explicitly records `scientific_payload.diagnostic_harness_bound = false`.

Execution-stage repair budget remains dormant and consumed count remains zero.

## P4 conclusion

P4 implementation is complete as a zero-science infrastructure package.

This is **not** a P5 PASS.

Next:

`M2_P5_ATOMIC_PACKAGE_QA_POSITIVE_AND_NEGATIVE_FIXTURES`
