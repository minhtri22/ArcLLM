# TTFT_M2 — A1 Governance Amendment: Execution Substrate and Repair-Budget Correction

**Date:** 2026-09-22  
**Amendment:** `M2_A1_EXECUTION_SUBSTRATE_AND_REPAIR_BUDGET_CORRECTION`

## Why this amendment exists

The previous TTFT_M2 governance was too coarse.

It used one execution-stage repair budget for failures belonging to different layers:

1. frozen research package;
2. orchestration / transport wrapper;
3. execution substrate;
4. scientific execution.

That is not the right control boundary.

The P8 GitHub Actions failures happened **outside the frozen research package** and before the exact frozen `run_ttft_m2_buildonly.ps1` executed even once.

Therefore the prior P10 closure remains part of the audit history, but its normative rule that no further M2 infrastructure continuation is permitted is superseded by this amendment.

The scientific conclusion from P10 remains unchanged:

`NO MECHANISM RESULT`

There is still no TTFT mechanism evidence.

## Audit preservation

Nothing is deleted or rewritten.

The following remain historical facts:

- P8 artifact `ae7b17678b9cbb29018c7e0207a12a5fb6b99ac9`;
- P10 artifact `0cbca9efa9754bf45e48ada5c7425ae7a8754d4a`;
- terminal commit `58d1091ad0a8619c97004f3afb70dfbbbf1160f5`;
- GitHub Actions run `35678683835`;
- GitHub Actions run `35678861153`.

What changes is the governance interpretation of those failures.

They are no longer allowed to exhaust the repair budget that protects the frozen research package.

## Corrected failure domains

### A. Research package

Includes:

- frozen runner;
- execution lock;
- schemas;
- manifests;
- provenance;
- package QA tooling / fixtures;
- scientific-design bindings.

Only a defect attributable to this frozen object may consume the **frozen research-package repair budget**.

### B. Orchestration / transport

Includes:

- GitHub Actions YAML;
- CI wrappers;
- artifact upload / download glue;
- shell-invocation wrappers;
- transport-only code.

A defect here does **not** consume research-package repair budget as long as:

- frozen package blobs are unchanged;
- scientific design is unchanged;
- no scientific outcome has been exposed.

### C. Execution substrate

Includes:

- GitHub-hosted Windows runners;
- local Windows workstation;
- shell/toolchain availability;
- runner provisioning / startup.

Failure of one substrate does not imply failure of the frozen package.

A different viable substrate may be used if it is preregistered and executes the exact same frozen object.

### D. Scientific execution

Still forbidden until a valid BuildOnly execution has passed.

No change is made to scientific anti-rescue rules.

## Corrected repair budget

Historical v0.1 budget:

```text
max        1
consumed   1
status     exhausted under prior governance
```

That historical accounting is retained.

But it no longer controls package/substrate continuation because its repair was consumed by an orchestration wrapper defect outside the frozen research object.

New package-integrity budget:

```text
max        1
consumed   0
remaining  1
```

It activates only after the exact frozen runner begins execution and a defect is attributable to the frozen research package.

No scientific mutation is permitted under this repair.

## Corrected close rule

An infrastructure program may be terminally closed only if at least one of the following is true:

1. actual frozen-package execution exposes package defects until its package-repair budget is exhausted;
2. all preregistered viable substrates have been attempted and shown unavailable/unusable;
3. a substrate-qualification gate concludes there is no viable substrate for the exact frozen package;
4. the operator explicitly terminates the program.

The following are **not sufficient** to close the research program:

- a newly-added CI wrapper fails before the frozen runner starts;
- one hosted substrate fails while a viable local substrate remains;
- an executor returns no logs before another viable substrate is attempted.

## M2 status after A1

The GitHub-hosted Windows path remains historical evidence:

```text
run 35678683835  orchestration failure before job creation
run 35678861153  hosted job startup failure before observable step
exact frozen runner executions = 0
```

The next viable substrate is the user's local Windows workstation.

Exact frozen continuation bindings remain:

```text
package source head  7e807caf7dd357f9c820f89f5a719a1e4a10139a
runner blob          4722e86a01453d973ee2122b49229b80bf7d84f6
lock blob            9af4c7b4c96354223e1f671a43af64b215072330
evidence manifest    bf496e00e4cf0bff86582e0649c6c26bc28b6f60
package manifest     4629910255580322706b01f318ad9a80044208d7
P7 authorization     8faf0cd91da381333fdf7971e431fc091701a62b
```

No mutation of these bindings is authorized.

## Next stage

`M2_P8L_LOCAL_BUILDONLY_EXECUTION_QUALIFICATION`

P8L may:

- pull the exact amended branch to the local Windows workstation;
- verify the exact frozen bindings;
- verify the P7 authorization;
- invoke the exact frozen `run_ttft_m2_buildonly.ps1`;
- collect and return its generated result/evidence bundle.

P8L may **not**:

- load the target model;
- launch scientific diagnostics;
- dispatch GPU work;
- measure TTFT/performance;
- collect fresh mechanism observations;
- alter hypothesis, threshold, endpoint, workload or frozen package.

P9 remains blocked until:

1. P8L obtains a valid BuildOnly PASS; and
2. a separate explicit P9 scientific authorization is issued.

## General governance principle

> Governance exists to control scientific degrees of freedom, preserve provenance and prevent post-outcome rescue. It must not convert unrelated transport/orchestration/substrate faults into scientific-package failures or close a research path while an unchanged frozen research object still has an untried viable substrate.
