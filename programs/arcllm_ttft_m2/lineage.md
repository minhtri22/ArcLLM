# ARCLLM_TTFT_M2 — lineage

Policy: `APPEND_ONLY_WITHIN_TTFT_M2`  
Created: 2026-09-22  
Branch: `research/arcllm-ttft-m2`

This lineage is fresh. It does not continue or append `programs/arcllm_ttft_mechanism/lineage.md`.

## ORIGIN

Git history base:

`86ec48ce0cfff28fa43c23c1d8b57b8f881d249e`

Immutable parent evidence:

### ANL64
- terminal HEAD `0e40b3affe4f6add9ce23921687b2659017c95d3`
- terminal status `ANL64_PROGRAM_CLOSED_VALID_NEGATIVE_TTFT_BLOCKED`
- terminal P7 adjudication blob `79df1758f23f724890eb302907c0b2d3045df735`
- terminal P7 document blob `dd0ab0389251e77ccc2d654e77a897781426be34`

### TTFT_M1
- terminal HEAD `86ec48ce0cfff28fa43c23c1d8b57b8f881d249e`
- terminal status `TTFT_M1_PROGRAM_CLOSED_INFRASTRUCTURE_STOP_NO_MECHANISM_RESULT`
- P10 adjudication blob `3fd5ed7909f409e3b7419da8ec50cfc6041e1f48`
- P10 document blob `219724e91a8b228dc11197600def4abd19e3d0c4`
- terminal infrastructure adjudication blob `de89ae36950a9bee2262c7f055cad95a7d9e3106`
- closed M1 lineage blob `6ab8d3aa4c36c58f2ae898bbbc9f2efc443f7980`

Parent mechanism result inherited: **NONE**.

## 2026-09-22 — M2-P0 ORIGIN / INDEPENDENCE / GOVERNANCE

TTFT_M2 opened as a new independent program. Neither parent is reopened or reclassified.

Execution authorization remains zero:
- diagnostic-science implementation: false
- BuildOnly: false
- model load: false
- GPU dispatch: false
- timing: false

The execution-stage repair budget is dormant during package development.

## 2026-09-22 — M2-P1 INFRASTRUCTURE FAILURE REVIEW

TTFT_M1 terminated before BuildOnly because two infrastructure defects occurred under a one-repair execution-stage budget.

The first was `STATIC_GATE_OPERATIONALIZATION_TAUTOLOGY`: H-ART largely compiled the same common shader set twice and therefore did not operationalize historical SAFE-vs-candidate artifact identity.

The second was `POST_LOCK_RUNNER_PROVENANCE_STALE_REFERENCE`: after lock v0.2, runner preflight used v0.2 but result/package generation still resolved/labeled v0.1.

M2 requirement derived: package-development QA must detect semantic operationalization defects and cross-file provenance/version defects before the execution-stage repair budget activates.

## 2026-09-22 — M2-P2 METHOD TRANSFER ADMISSIBILITY

TTFT_M1 mechanism outcomes are non-transferable because none were observed.

M1 prior-art, source mapping, finite-hypothesis structure, factorial causal-design ideas and falsification logic may be consulted only as prior methodological evidence. The 2x2 design is not automatically canonical in M2.

M1 repair-budget timing is explicitly rejected: pre-freeze package-development defects do not consume the future execution-stage repair budget.

## 2026-09-22 — M2-P3 ATOMIC PACKAGE SPECIFICATION

The complete future execution package is defined as one QA object: runner + lock + result/failure schemas + evidence/package manifests + bundle member map + provenance + version labels + output name + adjudicator input contract + QA tool/fixtures.

A stale reference anywhere is a package failure.

M2-P3 is specification-only. No scientific runner, model load, GPU dispatch, build, timing, or fresh TTFT observation has occurred.

Next: `M2_P4_ZERO_SCIENCE_PACKAGE_IMPLEMENTATION`.


## 2026-09-22 — M2-P4 ZERO-SCIENCE PACKAGE IMPLEMENTATION

P4 was explicitly authorized in a separate predecessor commit.

Authorization blob:
`6ad37531ca81227e65420b1261780ebfafbca97a`.

The atomic infrastructure package was materialized without executing it.

Canonical P4 bindings:
- execution lock `9af4c7b4c96354223e1f671a43af64b215072330`;
- runner `4722e86a01453d973ee2122b49229b80bf7d84f6`;
- evidence manifest template `bf496e00e4cf0bff86582e0649c6c26bc28b6f60`;
- package manifest `4629910255580322706b01f318ad9a80044208d7`;
- atomic QA tool `73b74f2dbc65f0e1bff7cd2c226b4112c34ecfdb`;
- fixture index `9735e97d5ea264642725466e50ae26a00e8a4951`.

Five frozen schemas now cover success result, fail-closed result, evidence manifest, package manifest and adjudicator input.

The canonical positive fixture and all ten P3-required negative fixture classes are materialized.

Self-reference handling is explicit:
- static members -> exact Git blob;
- package manifest self -> runtime exact Git binding + external P5/P6 binding;
- future P7 authorization -> runtime exact Git binding;
- adjudicator input -> sidecar generated only after bundle SHA256 exists.

This avoids both stale-label provenance and impossible self-hash cycles.

P4 does **not** bind a diagnostic-science payload and does not silently canonicalize the unexecuted M1 2x2 design.

Scientific/execution accounting remains zero:
- BuildOnly runner executions: 0;
- target model loads: 0;
- GPU dispatches: 0;
- performance measurements: 0;
- fresh TTFT observations: 0.

Execution-stage repair budget remains inactive, consumed 0/1.

P5 has not run and no P5 PASS is claimed.

Next: `M2_P5_ATOMIC_PACKAGE_QA_POSITIVE_AND_NEGATIVE_FIXTURES`.


## 2026-09-22 — M2-P5 ATOMIC PACKAGE QA PASS

P5 evaluated the exact P4 execution package bound to source HEAD:

`7e807caf7dd357f9c820f89f5a719a1e4a10139a`

and source tree:

`09960203707653d22626ec2b39b7be20cac8e65a`.

The branch was verified unchanged before QA and again before P5 evidence publication.

Canonical atomic-QA tool:
- path `tools/ttft_m2_atomic_package_qa.py`;
- blob `73b74f2dbc65f0e1bff7cd2c226b4112c34ecfdb`;
- mode `p5`;
- exit code `0`;
- result `PASS`;
- errors `[]`.

The isolated execution runtime could not perform a direct network clone. No alternate source package was used. Every path consumed by the QA tool was materialized from the authoritative GitHub package and verified against its exact committed Git blob, while the connector independently verified the branch remained at the exact P4 HEAD.

Canonical positive fixture: PASS with no observed errors.

All ten preregistered negative fixtures were executed and their required fail-closed rejection was observed:
- STALE_LOCK_VERSION;
- WRONG_BUNDLE_DESTINATION_FILENAME;
- WRONG_GIT_BLOB_FIELD;
- MISSING_EVIDENCE_MEMBER;
- DUPLICATE_EVIDENCE_MEMBER;
- RESULT_SCHEMA_VERSION_MISMATCH;
- RUNNER_LOCK_MISMATCH;
- MANIFEST_RUNNER_MISMATCH;
- PACKAGE_RESULT_MISMATCH;
- SAME_SOURCE_AUTHORITY_TAUTOLOGY.

Supplementary JSON Schema Draft 2020-12 meta-validation passed for all five frozen schemas.

P5 evidence:
- formal artifact blob `3012c817e2983d0928cd7599cfa70197c4debd3c`;
- document blob `cdfa1c85ca25dc6eddeb5b03710d5d4230532210`;
- raw QA blob `aa832665cf25842887f34d6ad5c2f58bb0b75c23`;
- fixture detail blob `be2ffb8053e20cf61a612f180727180b70e57d9e`;
- raw QA SHA256 `C1C032DF3DED4F63FD9B34F5DD00B70F50DE7757AA78F65852628E9405D4B84D`.

The execution package is now frozen at the exact P4 package bindings. P5 evidence/governance files are outside the frozen execution package. Any future package mutation requires requalification.

Zero-science accounting remains:
- BuildOnly runner executions: 0;
- diagnostic executable launches: 0;
- target model loads: 0;
- GPU dispatches: 0;
- performance measurements: 0;
- fresh TTFT observations: 0;
- mechanism result: NONE.

Execution-stage repair budget remains inactive at 0/1.

P7 remains unopened and BuildOnly remains unauthorized.

P5 terminal result:

`PASS_M2_P5_ATOMIC_PACKAGE_QA`

Next: `M2_P6_ZERO_SCIENCE_GOVERNANCE_ADJUDICATION`.


## 2026-09-22 — M2-P6 ZERO-SCIENCE GOVERNANCE ADJUDICATION

P6 independently re-verified the frozen P4 execution package, P5 evidence, repair-budget semantics, zero-science boundary and future P7 authorization gate.

Adjudication input HEAD:
`0d1d99e5284496a091973dc3f64f0b3da398d7ef`

Frozen execution package remains:
- source HEAD `7e807caf7dd357f9c820f89f5a719a1e4a10139a`;
- source tree `09960203707653d22626ec2b39b7be20cac8e65a`.

All 11 package-critical Git blobs independently re-fetched at P6 matched the P5 freeze exactly.

The P4→P5 diff changed only P5 evidence, governance, documentation and TTFT_M2 lineage. No frozen package member changed.

P5 evidence independently re-verified:
- formal P5 blob `3012c817e2983d0928cd7599cfa70197c4debd3c`;
- raw QA blob `aa832665cf25842887f34d6ad5c2f58bb0b75c23`;
- fixture-detail blob `be2ffb8053e20cf61a612f180727180b70e57d9e`;
- raw QA SHA256 recomputed as `C1C032DF3DED4F63FD9B34F5DD00B70F50DE7757AA78F65852628E9405D4B84D`;
- canonical positive PASS;
- 10/10 negative expected rejections observed;
- five Draft 2020-12 schema meta-validations PASS.

Repair-budget adjudication:
- maximum execution-stage repairs: 1;
- consumed: 0;
- active: false;
- P6 PASS alone does not activate the budget;
- explicit P7 authorization remains required.

Zero-science adjudication:
- BuildOnly runner executions: 0;
- diagnostic executable launches: 0;
- target model loads: 0;
- GPU dispatches: 0;
- performance measurements: 0;
- fresh TTFT observations: 0;
- scientific result: NONE.

The future P7 authorization path was queried and does not exist at P6. The frozen runner fails closed when P7 is absent or malformed and additionally requires target-model execution, GPU dispatch and performance measurement to remain false.

P6 formal evidence:
- artifact blob `4ffae2c16d60d02a63c9cdbe96be1f5d1e7e0ad9`;
- document blob `ff6fd6b3fbbf33e23662a77fdacac29806659de6`.

Formal result:

`PASS_M2_P6_ZERO_SCIENCE_GOVERNANCE_ADJUDICATION`

P6 does not create P7 authorization, does not authorize BuildOnly, and does not activate the execution-stage repair budget.

Next and only admissible step:

`M2_P7_EXPLICIT_BUILDONLY_AUTHORIZATION_GATE`.
