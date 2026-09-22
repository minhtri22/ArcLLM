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


## 2026-09-22 — M2-P7 EXPLICIT BUILDONLY AUTHORIZATION GATE

P7 was opened only after P5 atomic-package QA PASS and P6 zero-science governance PASS.

Authorization parent HEAD:
`5058ef3b2f45510fdda7255101c74422a57a950a`

The frozen execution package remains bound to:
- P4 source HEAD `7e807caf7dd357f9c820f89f5a719a1e4a10139a`;
- P4 source tree `09960203707653d22626ec2b39b7be20cac8e65a`;
- runner blob `4722e86a01453d973ee2122b49229b80bf7d84f6`;
- lock blob `9af4c7b4c96354223e1f671a43af64b215072330`;
- package manifest blob `4629910255580322706b01f318ad9a80044208d7`;
- evidence manifest template blob `bf496e00e4cf0bff86582e0649c6c26bc28b6f60`.

P7 decision:

`AUTHORIZE_EXACT_FROZEN_M2_BUILDONLY_EXECUTION`

Canonical authorization artifact:
- path `config/arcllm_ttft_m2_p7_buildonly_authorization_v0.1.json`;
- blob `8faf0cd91da381333fdf7971e431fc091701a62b`;
- schema `arcllm.ttft_m2.p7.buildonly_authorization.v0.1`;
- decision `M2_P7_BUILDONLY_AUTHORIZED`.

Formal gate record:
- blob `ce261a743633b2f7e306d455b4f871e44923acee`.

P7 authorizes only the exact frozen zero-science BuildOnly runner/package.

Still forbidden:
- target-model execution;
- diagnostic executable launch;
- model load;
- GPU dispatch;
- performance or TTFT measurement;
- fresh TTFT observation;
- mechanism adjudication;
- scientific mutation.

Execution-stage repair budget transitions to active because all activation prerequisites are now satisfied:
- active: true;
- consumed: 0/1;
- scope: orchestration/infrastructure only.

Activation itself consumes no repair.

No BuildOnly runner was executed during P7. All execution/science counters remain zero and scientific result remains NONE.

P7 terminal result:

`AUTHORIZE_EXACT_FROZEN_M2_BUILDONLY_EXECUTION`

Next and only admissible stage:

`M2_P8_BOUNDED_BUILDONLY_EXECUTION`.


## 2026-09-22 — M2-P8 FIRST ATTEMPT FAILED BEFORE JOB CREATION; REPAIR 1/1

First P8 Actions run:
`35678683835`

Head:
`1ecb6b7606f81e71fd7c5c6eb0f0d12c5838e2a1`

The run completed with failure and the Actions API returned zero jobs. The exact frozen BuildOnly runner did not execute.

Classification:
`VALID_EXECUTION_STAGE_ORCHESTRATION_DEFECT`

Defect:
`P8_WORKFLOW_YAML_PLAIN_SCALAR_COLON_PARSE_FAILURE`

The P8 workflow job condition was a YAML plain scalar containing a compared commit message with `: `, causing workflow parsing to fail before Windows job creation.

This defect is outside the frozen execution package and outside science.

Bounded repair 1/1:
- frozen P4 package unchanged;
- P7 authorization unchanged;
- only orchestration workflow predicate repaired;
- replay trigger changed to colon-free exact message `P8-REPLAY-ONE-SHOT`;
- full P8 replay required from zero.

Repair evidence:
- record blob `0a21904295708660df7cb06ceda8163768d6b50c`;
- document blob `efec8b9f596450bd0fca9a831f99ba83f2dd80c7`;
- repaired workflow blob `45bf73ca7aad83a361299ce4ea7c4490070b564f`.

Execution-stage repair budget:
- active: true;
- consumed: 1/1;
- remaining: 0;
- second execution-stage defect -> `STOP_INFRASTRUCTURE_UNSTABLE`.

Scientific/execution accounting remains zero because the first workflow failed before job creation.

Next: full fresh M2-P8 replay from zero.


## 2026-09-22 — M2-P8 TERMINAL INFRASTRUCTURE STOP

P8 was authorized by P7 but did not obtain a valid BuildOnly execution.

Attempt 1:
- run `35678683835`;
- head `1ecb6b7606f81e71fd7c5c6eb0f0d12c5838e2a1`;
- failure before job creation;
- exact runner executions: 0;
- classified `P8_WORKFLOW_YAML_PLAIN_SCALAR_COLON_PARSE_FAILURE`;
- valid execution-stage repair consumed: 1/1.

Repair 1/1 changed only the P8 orchestration workflow and required full replay from zero. Frozen package and science were unchanged.

Attempt 2 full replay:
- run `35678861153`;
- head `2fcb9756d032d806e52bc521457c7c49dfe6f868`;
- workflow parsed and job `106591157811` was created;
- conclusion: failure;
- observable steps: 0;
- job log unavailable;
- artifacts: 0;
- exact runner executions: 0.

Bounded classification:
`SECOND_EXECUTION_STAGE_INFRASTRUCTURE_STARTUP_FAILURE_UNRESOLVED`

No narrower cause is claimed because no job log exists.

Because repair budget is exhausted at 1/1, governance requires:

`STOP_INFRASTRUCTURE_UNSTABLE`

No second repair, environment switch or third attempt is admissible inside TTFT_M2.

Frozen package bindings remain unchanged, including runner `4722e86a01453d973ee2122b49229b80bf7d84f6`, lock `9af4c7b4c96354223e1f671a43af64b215072330`, package manifest `4629910255580322706b01f318ad9a80044208d7`, and P7 authorization `8faf0cd91da381333fdf7971e431fc091701a62b`.

P8 formal evidence:
- artifact blob `ae7b17678b9cbb29018c7e0207a12a5fb6b99ac9`;
- document blob `2ad183f856ddf329cd39d1f766bd0aea0927ed89`.

Scientific accounting remains zero; scientific result remains NONE.

P9 is blocked and not opened.

Next: `M2_P10_FINAL_PROGRAM_ADJUDICATION`.


## 2026-09-22 — M2-P10 FINAL PROGRAM ADJUDICATION

P10 adjudicated the exact P8 terminal state at HEAD:

`86a951becd08c67ef1b8dffba13b1d1cd141206c`

P8 terminal evidence:
- artifact `ae7b17678b9cbb29018c7e0207a12a5fb6b99ac9`;
- result `STOP_INFRASTRUCTURE_UNSTABLE`;
- valid BuildOnly execution obtained: false;
- exact frozen runner executions: 0.

Execution-stage repair budget:
- maximum: 1;
- consumed: 1;
- remaining: 0;
- exhausted: true.

P9 status:
`BLOCKED_NOT_OPENED`

No target model execution, diagnostic executable launch, GPU dispatch, performance measurement, fresh TTFT observation, or mechanism adjudication occurred.

Final P10 evidence:
- artifact blob `0cbca9efa9754bf45e48ada5c7425ae7a8754d4a`;
- document blob `433b3ee26d2cc26c36024e2cd0b1f85567cff64b`.

Final scientific interpretation boundary:

TTFT_M2 infrastructure failures are not evidence supporting or falsifying any candidate TTFT mechanism. No candidate mechanism was ranked, preferred, distinguished, causally implicated, supported or falsified. Scientific result remains `NONE`.

Formal terminal result:

`INFRASTRUCTURE_STOP_NO_MECHANISM_RESULT`

Formal terminal status:

`TTFT_M2_PROGRAM_CLOSED_INFRASTRUCTURE_STOP_NO_MECHANISM_RESULT`

TTFT_M2 is closed. No further execution, repair or P9 opening is permitted inside M2. Any future work requires a new independent governance program rather than an M2 rescue.


## 2026-09-22 — M2-A1 GOVERNANCE AMENDMENT

The prior governance was found to conflate three different infrastructure domains:

1. frozen research package;
2. orchestration / transport wrapper;
3. execution substrate.

That conflation allowed a GitHub Actions YAML defect and hosted-runner startup failure to exhaust the same budget intended to protect the frozen research package even though the exact frozen BuildOnly runner never executed.

A1 therefore amends governance without deleting or rewriting prior evidence.

Historical P8/P10 remain preserved:
- P8 blob `ae7b17678b9cbb29018c7e0207a12a5fb6b99ac9`;
- P10 blob `0cbca9efa9754bf45e48ada5c7425ae7a8754d4a`;
- historical terminal commit `58d1091ad0a8619c97004f3afb70dfbbbf1160f5`.

Their scientific non-result remains valid: `NONE`.

Their normative rule forbidding all further M2 infrastructure continuation is superseded by A1.

A1 evidence:
- amendment blob `4e56683b23153d4a4055d1bafd4422f11d186783`;
- document blob `0948d9dbc8b213840e208aaa679e9cc32541ea75`.

Corrected budget domains:

```text
historical v0.1 execution-stage budget
  consumed 1/1
  retained as historical accounting
  no longer controls package/substrate continuation

frozen research-package repair budget
  consumed 0/1
  activates only if exact frozen runner begins and package defect is attributable

orchestration / transport failures
  do not consume package-repair budget

execution-substrate failures
  do not consume package-repair budget
  alternate preregistered substrate permitted
```

The exact frozen package remains unchanged:
- runner `4722e86a01453d973ee2122b49229b80bf7d84f6`;
- lock `9af4c7b4c96354223e1f671a43af64b215072330`;
- package manifest `4629910255580322706b01f318ad9a80044208d7`;
- P7 authorization `8faf0cd91da381333fdf7971e431fc091701a62b`.

GitHub-hosted Windows remains historical unsuccessful substrate evidence.

The local Windows workstation is now the primary viable untried substrate.

P9 remains blocked until valid local BuildOnly PASS and a separate scientific authorization.

Next:
`M2_P8L_LOCAL_BUILDONLY_EXECUTION_QUALIFICATION`.


## 2026-09-22 — M2-P8L LOCAL BUILDONLY AUTHORIZATION

P8L local Windows execution qualification is now explicitly authorized.

Authorization:
- path `config/arcllm_ttft_m2_p8l_local_buildonly_authorization_v0.1.json`;
- blob `fa15f82ee75a3356c21e213d935eb93002d885f7`;
- decision `M2_P8L_LOCAL_BUILDONLY_AUTHORIZED`.

Local orchestration wrapper:
- path `scripts/ttft_m2/p8l_local_oneclick.ps1`;
- blob `9b7ebffa37c5c2657a2db95682134e8307ffc163`.

The wrapper is outside the frozen research package and may only:
1. verify local worktree / branch / authorization;
2. verify exact frozen Git blobs;
3. invoke the exact frozen `run_ttft_m2_buildonly.ps1`;
4. package a local return report.

Frozen package remains unchanged.

Package-repair budget remains 0/1 before local runner execution.

Scientific execution remains forbidden.

Preferred return evidence:
`results/ttft_m2_p8l_local_return_to_chatgpt.zip`

Next:
execute P8L once on the preregistered local Windows workstation, then adjudicate the returned evidence before any repair, rerun or P9 opening.
