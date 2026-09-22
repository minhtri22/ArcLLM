# TTFT_M2 — P5 Atomic Package QA

**Date:** 2026-09-22  
**Result:** `PASS_M2_P5_ATOMIC_PACKAGE_QA`

## Exact source under QA

P5 evaluated the P4 package frozen at:

- branch: `research/arcllm-ttft-m2`
- source HEAD: `7e807caf7dd357f9c820f89f5a719a1e4a10139a`
- source tree: `09960203707653d22626ec2b39b7be20cac8e65a`

The branch was verified identical to that exact HEAD both before QA and immediately before formal evidence publication.

The isolated runtime could not resolve `github.com` for a direct network clone. No alternate source tree was substituted. The P5 working set was instead materialized from the authoritative GitHub connector using the exact committed package contents, and every path read by the QA tool was verified against its frozen Git blob before execution.

The package source identity therefore rests on:
1. connector verification that the branch remained at the exact P4 HEAD;
2. exact Git blob identity for every file consumed by P5;
3. the frozen P4 package bindings.

## Canonical P5 QA execution

Canonical QA tool:

- path: `tools/ttft_m2_atomic_package_qa.py`
- blob: `73b74f2dbc65f0e1bff7cd2c226b4112c34ecfdb`
- mode: `p5`

Observed:

```text
exit code = 0
result    = PASS
errors    = []
```

Raw QA evidence SHA256:

`C1C032DF3DED4F63FD9B34F5DD00B70F50DE7757AA78F65852628E9405D4B84D`

## Canonical positive fixture

`tests/fixtures/ttft_m2_atomic_package/positive_canonical.json`

blob:

`351c65d8987629062749540b730da13bb7caca4c`

Observed errors:

```text
[]
```

Result:

`PASS`

## Negative-fixture adjudication

All 10 preregistered negative cases were executed and rejected fail-closed by the exact P4 QA implementation:

| Fixture | Required failure observed |
|---|---|
| STALE_LOCK_VERSION | PASS |
| WRONG_BUNDLE_DESTINATION_FILENAME | PASS |
| WRONG_GIT_BLOB_FIELD | PASS |
| MISSING_EVIDENCE_MEMBER | PASS |
| DUPLICATE_EVIDENCE_MEMBER | PASS |
| RESULT_SCHEMA_VERSION_MISMATCH | PASS |
| RUNNER_LOCK_MISMATCH | PASS |
| MANIFEST_RUNNER_MISMATCH | PASS |
| PACKAGE_RESULT_MISMATCH | PASS |
| SAME_SOURCE_AUTHORITY_TAUTOLOGY | PASS |

For `RESULT_SCHEMA_VERSION_MISMATCH`, the mutation correctly produced both `RESULT_SCHEMA_VERSION_MISMATCH` and the downstream `PACKAGE_RESULT_MISMATCH`; the preregistered expected failure was present.

Therefore:

```text
required negative fixtures     10
executed                       10
expected rejection observed    10
fail-closed coverage           PASS
```

## Cross-file package QA

The actual committed package produced no cross-file errors.

P5 verified:
- exact static Git blobs;
- lock/runner identity;
- schema IDs and blobs;
- package/evidence manifest consistency;
- source → destination filename mappings;
- self-manifest runtime-exact binding;
- future P7 authorization runtime-exact binding;
- output bundle naming;
- absence of stale version tokens;
- non-tautological authority sources;
- zero-science restrictions.

Supplementary Draft 2020-12 meta-validation also passed for all five JSON schemas.

## Package freeze

The execution package is now frozen to the exact P4 package source:

`7e807caf7dd357f9c820f89f5a719a1e4a10139a`

P5 evidence/governance commits are outside that frozen execution package. No package member may now be mutated without explicit requalification.

This freeze **does not activate** the execution-stage repair budget and does not authorize BuildOnly.

## Zero-science accounting

```text
BuildOnly runner executions          0
diagnostic executable launches       0
target model loads                    0
GPU dispatches                        0
performance measurements              0
fresh TTFT observations               0
mechanism result                    NONE
```

The future P7 authorization artifact does not exist and the P7-gated runner was not executed.

## P5 conclusion

`PASS_M2_P5_ATOMIC_PACKAGE_QA`

The infrastructure-validity package has passed its atomic positive/negative QA and is frozen.

This result opens only:

`M2_P6_ZERO_SCIENCE_GOVERNANCE_ADJUDICATION`

It does **not** open P7 or BuildOnly.
