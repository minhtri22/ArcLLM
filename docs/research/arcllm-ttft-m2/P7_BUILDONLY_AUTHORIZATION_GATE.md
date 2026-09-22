# TTFT_M2 — P7 Explicit BuildOnly Authorization Gate

**Date:** 2026-09-22  
**Decision:** `AUTHORIZE_EXACT_FROZEN_M2_BUILDONLY_EXECUTION`

## Gate basis

P7 was opened only after:

- P5: `PASS_M2_P5_ATOMIC_PACKAGE_QA`
- P6: `PASS_M2_P6_ZERO_SCIENCE_GOVERNANCE_ADJUDICATION`
- frozen P4 package re-verification: PASS
- P7 authorization artifact absent before this decision: confirmed

The frozen execution package remains bound to:

- P4 source HEAD `7e807caf7dd357f9c820f89f5a719a1e4a10139a`
- P4 source tree `09960203707653d22626ec2b39b7be20cac8e65a`
- runner blob `4722e86a01453d973ee2122b49229b80bf7d84f6`
- lock blob `9af4c7b4c96354223e1f671a43af64b215072330`
- package manifest blob `4629910255580322706b01f318ad9a80044208d7`
- evidence manifest template blob `bf496e00e4cf0bff86582e0649c6c26bc28b6f60`

## Decision

The explicit P7 decision is:

`M2_P7_BUILDONLY_AUTHORIZED`

Canonical authorization artifact:

`config/arcllm_ttft_m2_p7_buildonly_authorization_v0.1.json`

blob:

`8faf0cd91da381333fdf7971e431fc091701a62b`

The decision authorizes **only** the exact frozen zero-science BuildOnly runner/package.

## What P7 authorizes

```text
BuildOnly runner execution       true
exact frozen package only         true
runtime preflight required        true
fail-closed on binding drift      true
```

## What P7 does not authorize

```text
target model execution            false
diagnostic executable launch      false
model load                        false
GPU dispatch                      false
performance measurement           false
fresh TTFT observation            false
mechanism adjudication            false
scientific mutation               false
```

Thus this is an infrastructure-execution authorization, not a scientific-experiment authorization.

## Repair-budget transition

Before P7:

```text
execution-stage repair budget active     false
repairs consumed                         0 / 1
```

After this explicit authorization:

```text
execution-stage repair budget active     true
repairs consumed                         0 / 1
scope                                    orchestration/infrastructure only
```

Activation does not consume the repair. A repair is consumed only if a valid execution-stage infrastructure defect is actually repaired under the frozen governance.

Scientific mutation remains forbidden.

## No execution in P7

P7 performs no BuildOnly execution.

Accounting at gate closure remains:

```text
BuildOnly runner executions          0
diagnostic executable launches       0
target model loads                    0
GPU dispatches                        0
performance measurements              0
fresh TTFT observations               0
scientific result                   NONE
```

## P7 conclusion

The authorization gate result is:

`AUTHORIZE_EXACT_FROZEN_M2_BUILDONLY_EXECUTION`

Only the next bounded stage is opened:

`M2_P8_BOUNDED_BUILDONLY_EXECUTION`

P8 must execute exactly the frozen runner under this exact authorization artifact. Any package drift invalidates this authorization and requires requalification.
