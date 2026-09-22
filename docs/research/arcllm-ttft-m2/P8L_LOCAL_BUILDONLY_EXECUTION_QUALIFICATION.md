# TTFT_M2 — P8L Local BuildOnly Execution Qualification

**Date:** 2026-09-22  
**Status:** `AUTHORIZED_READY_FOR_LOCAL_EXECUTION`

## Purpose

Execute the exact frozen TTFT_M2 BuildOnly package on the preregistered local Windows workstation after M2-A1 corrected the infrastructure-governance boundary.

This stage is infrastructure qualification only.

No target model, GPU dispatch, performance timing, fresh TTFT observation, or mechanism adjudication is authorized.

## Authorization

Authorization:

`config/arcllm_ttft_m2_p8l_local_buildonly_authorization_v0.1.json`

Authorization blob:

`fa15f82ee75a3356c21e213d935eb93002d885f7`

Decision:

`M2_P8L_LOCAL_BUILDONLY_AUTHORIZED`

## Local wrapper

Path:

`scripts/ttft_m2/p8l_local_oneclick.ps1`

Blob:

`9b7ebffa37c5c2657a2db95682134e8307ffc163`

The wrapper is orchestration-only and is outside the frozen research package.

It must not alter any frozen package member.

## Frozen package bindings

```text
runner
4722e86a01453d973ee2122b49229b80bf7d84f6

execution lock
9af4c7b4c96354223e1f671a43af64b215072330

evidence manifest
bf496e00e4cf0bff86582e0649c6c26bc28b6f60

package manifest
4629910255580322706b01f318ad9a80044208d7

P7 authorization
8faf0cd91da381333fdf7971e431fc091701a62b

atomic QA tool
73b74f2dbc65f0e1bff7cd2c226b4112c34ecfdb

fixture index
9735e97d5ea264642725466e50ae26a00e8a4951
```

## Local preflight

The wrapper must verify:

1. current branch is `research/arcllm-ttft-m2`;
2. tracked worktree is clean;
3. P8L authorization is exact and active;
4. every frozen binding above resolves to its exact Git blob;
5. wrapper blob matches its authorization binding;
6. governance points to P8L as the current next stage.

Only after these checks may the wrapper invoke:

`run_ttft_m2_buildonly.ps1`

## Exact runner behavior

The exact frozen runner performs its own runtime preflight, including atomic package QA.

It remains zero-science.

## Outputs

Primary report:

`results/ttft_m2_p8l_local/TTFT_M2_P8L_LOCAL_REPORT.json`

Frozen runner bundle:

`results/ttft_m2_buildonly_return_to_chatgpt.zip`

Combined return bundle:

`results/ttft_m2_p8l_local_return_to_chatgpt.zip`

The combined return bundle is the preferred file to return for adjudication.

## Failure handling

If the wrapper fails before invoking the exact frozen runner, this is not automatically a research-package defect.

If the exact frozen runner starts and fail-closes, preserve all outputs and stop.

Do not repair or rerun before independent failure-domain adjudication.

The package-repair budget remains 0/1 until a defect is actually attributed to the frozen research package.

## Scientific boundary

Still forbidden:

- target-model execution;
- diagnostic scientific executable launch;
- GPU dispatch;
- performance or TTFT timing;
- fresh TTFT observations;
- mechanism adjudication;
- hypothesis, threshold, endpoint or workload changes.

P9 remains blocked until P8L obtains a valid BuildOnly result and a separate P9 authorization is issued.
