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
