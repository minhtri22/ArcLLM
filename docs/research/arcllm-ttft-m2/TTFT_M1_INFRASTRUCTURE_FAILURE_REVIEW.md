# TTFT_M2 — TTFT_M1 Infrastructure Failure Review

## Scope

This review uses TTFT_M1 only as infrastructure/governance evidence. It does not convert any M1 hypothesis into a mechanism result.

## Failure A — H-ART operationalization tautology

The pre-run audit discovered that the first H-ART implementation effectively compiled the same 16 common shader sources with the same compiler twice and compared the outputs. That mostly tested compiler determinism rather than the intended historical SAFE-vs-candidate artifact identity.

Classification:

`STATIC_GATE_OPERATIONALIZATION_TAUTOLOGY`

No BuildOnly execution or science had occurred. The correction consumed M1's single P6 BuildOnly repair allowance.

### M2 requirement derived

An atomic package is not valid merely because its files are internally type-correct. Zero-science QA must also verify that each gate operationalizes the intended contrast against its frozen authority source. Positive fixtures must demonstrate the intended authority binding; negative fixtures must include a tautological/same-source comparison and require rejection.

## Failure B — stale post-lock runner provenance

M1 lock v0.2 was frozen at blob:

`e2eb6170933ee074bfb3446661b7d69d3d40eaa9`

The canonical runner was blob:

`4f7500bb2b0e3bc0b334a3f41c1244fcdc55d545`

The runner correctly set its preflight lock path to v0.2 and validated v0.2. However, its result construction still resolved:

`config/arcllm_ttft_m1_p6_implementation_lock_v0.1.json`

for the `implementation_lock_blob` field, and its packaging copied the v0.2 source using the destination filename:

`arcllm_ttft_m1_p6_implementation_lock_v0.1.json`

Classification:

`POST_LOCK_RUNNER_PROVENANCE_STALE_REFERENCE`

The returned evidence would therefore have contained stale/ambiguous authoritative-lock provenance.

No runner had been executed. Repairing the defect was prohibited because the single execution-stage repair allowance was already consumed. TTFT_M1 stopped before science.

## Root governance weakness

The defect was not that the lock or runner was individually unreadable. The weakness was compositional: components were frozen and QA'd without mechanically proving all cross-file/version relationships as one closed object.

## TTFT_M2 requirements

Before any BuildOnly authorization, M2 must mechanically establish all of the following as equal/consistent:

```text
runner authoritative lock version
= success result reported lock version
= failure result reported lock version
= resolved Git blob for authoritative lock
= evidence manifest lock member
= package manifest lock member
= destination filename label
= QA expected lock version
= adjudicator input lock binding
```

Equivalent consistency checks are required for authorization, runner, schemas, manifest, package membership, source blobs, destination labels, and adjudicator input.

The QA suite must fail closed on at least:

- stale lock version;
- wrong bundle destination filename;
- wrong Git blob field;
- missing evidence member;
- duplicate evidence member;
- result-schema version mismatch;
- runner/lock mismatch;
- manifest/runner mismatch;
- package/result mismatch;
- same-source/tautological authority comparison.

## Repair-budget correction

During P4/P5 package development, defects found by zero-science QA are development defects and do **not** consume the execution-stage repair budget.

Only after:
1. P4 package implementation completes;
2. P5 atomic package QA passes;
3. P6 zero-science governance adjudication passes;
4. exact package is frozen; and
5. explicit P7 BuildOnly authorization is granted,

does the one-repair execution-stage budget become active.

## Scientific boundary

This review establishes no TTFT mechanism result. H-ART, H-DPIPE, H-PRECOND, H-STATE-INTERACTION and H-NULL remain unadjudicated.
