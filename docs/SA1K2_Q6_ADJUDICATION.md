# SA1-K2 Q6 Adjudication

**Date:** 2026-09-21  
**Result:** `Q6_STAGE_FAIL_CORRECTNESS`  
**Canonical classification:** `SA1_K2_Q6_CORRECTNESS_FAIL`

## Evidence basis

Independent adjudication used `sa1_k2_q6_failure_return_to_chatgpt.zip` with SHA-256:

`B4D6A6F7CF4ADE32EFFD9408754216CF3ED8FCABC2065F1A94D253CBA0933E95`

The bundle binds the consumed scientific execution to commit
`b87f3bccee3809cedb2d88ab77c9885406348a87` and the later packaging-only state
to commit `8e783119ff6268d26d9c2e8405e0cb5f2faec182`.

All ten ZIP entries were independently rehashed. The frozen P7 Q6 baseline SPIR-V is exactly
`F2267838D099128F233EF30817464658AAD71AAFA3933461FB315FAD10ED3F67`.
The candidate source has Git blob
`0fdc0c8f195872396a653b38ee2283156fbaeaa0`, matching the scientific execution state.
The implementation lock and contract have Git blobs
`66b720b55e0b3873c2c6b07a300cd3e5adc31c74` and
`a7efc35a2bc70e4bbbd15751dfc6a6a29b4486a2`.

The scientific-to-packaging delta changes only governance/packaging/static-test state; it does not
change the Q6 candidate shader or component harness. The `-PackageFailedExisting` path reuses the
already-produced raw/build/SPIR-V/executable artifacts and exits before the normal compile/build/GPU path.

## Correctness adjudication

Target-local shader compile: **PASS**.  
Target-local native BuildOnly: **PASS**.

The retained raw result is exactly:

```text
status = ERROR
error  = correctness gate failed: candidate_cpu
```

The frozen harness verifies in this order:

```text
baseline_cpu
candidate_cpu
candidate_baseline
```

Therefore reaching the `candidate_cpu` failure proves that `baseline_cpu` had already cleared its
frozen gate for the active case. The candidate then violated at least one frozen condition:

```text
finite = true
max_abs <= 0.02
RMSE    <= 0.005
```

Because the harness is fail-fast, the exact numerical magnitude and the exact violated dimension were
not retained. They are intentionally recorded as unknown rather than reconstructed or rerun.

No F0 infrastructure, provenance, source-drift, or packaging defect was found that invalidates the
first scientific attempt.

## Execution boundary

Q6 performance measurement: **NOT RUN**.  
Q6 measured pairs: **0**.  
Q6 performance authorization: **absent / false**.  
Target model: **NOT RUN**.  
Q3 reopen: **false**.

## Decision

The preregistered correctness stop rule applies. SA1-K2 is closed as:

`Q6_STAGE_FAIL_CORRECTNESS`

No Q6 rerun, threshold mutation, alternate reduction/geometry, performance rescue, target-model
integration, or Q3 reopen is permitted inside SA1.

This result is a numerical correctness failure before performance. It does **not** establish that Q6
is slow or that subgroup split-K is generally ineffective.
