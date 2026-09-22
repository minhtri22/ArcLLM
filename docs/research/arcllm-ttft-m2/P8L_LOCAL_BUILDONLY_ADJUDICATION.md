# TTFT_M2 — P8L Local BuildOnly Adjudication

**Date:** 2026-09-22  
**Result:** `PASS_M2_P8L_LOCAL_BUILDONLY_EXECUTION_QUALIFICATION`

## Adjudication

The preregistered local Windows execution substrate produced a valid execution of the **exact frozen TTFT_M2 BuildOnly package**.

This resolves the infrastructure blocker that remained after the historical GitHub-hosted P8 attempts.

It does **not** produce a TTFT mechanism result.

## Exact execution identity

```text
branch     research/arcllm-ttft-m2
HEAD       e623bfb878e6e59cb465271905245e83bf720b9e
substrate  LOCAL_WINDOWS_WORKSTATION
worktree   tracked clean
```

P8L authorization:

```text
fa15f82ee75a3356c21e213d935eb93002d885f7
M2_P8L_LOCAL_BUILDONLY_AUTHORIZED
```

Frozen bindings all matched exactly:

```text
runner             4722e86a01453d973ee2122b49229b80bf7d84f6
execution lock     9af4c7b4c96354223e1f671a43af64b215072330
evidence template  bf496e00e4cf0bff86582e0649c6c26bc28b6f60
package manifest   4629910255580322706b01f318ad9a80044208d7
P7 authorization   8faf0cd91da381333fdf7971e431fc091701a62b
atomic QA tool     73b74f2dbc65f0e1bff7cd2c226b4112c34ecfdb
fixture index      9735e97d5ea264642725466e50ae26a00e8a4951
```

## Runtime preflight

The exact frozen runner executed its own runtime-preflight QA.

Result:

`PASS`

Errors:

`[]`

The exact runner then completed with exit code `0`.

Returned status:

`M2_INFRASTRUCTURE_BUILDONLY_COMPLETE_AWAITING_ADJUDICATION`

No failure result was generated.

## Returned evidence integrity

User-returned outer bundle:

`1BA6CB2393299C6F81CF88110D9B58650D462E602D2EBAC6540708104AA1AAAE`

Frozen BuildOnly inner bundle:

`80E25A75F9783D98F39840594E0BC2888DFF9A4BEB6AB1EAF57846B48150D872`

Generated result:

`2A6031607A66A7F8B8D908300AF34DF79FA12F5B338231DCFA361758C0B7119B`

Runtime evidence manifest:

`A6E4AC7C12E076C72D486685B9C69FB2200C56F470FF594EDD57ACF02BED708C`

Package manifest byte SHA256:

`FFE32A007581B38983C2F30A7CF66348AF104C2CE1035F9453EAD91F76E7FDE3`

Independent adjudication verified:

- adjudicator-input bundle hash == returned inner ZIP hash;
- adjudicator-input result hash == result bytes;
- adjudicator-input evidence hash == runtime evidence-manifest bytes;
- adjudicator-input package-manifest hash == package-manifest bytes;
- all required inner bundle members are present;
- there are no extra inner bundle members;
- BuildOnly result validates against Draft 2020-12 schema;
- runtime evidence manifest validates;
- package manifest validates;
- adjudicator input validates.

The adjudicator input is intentionally a sidecar generated after the inner ZIP hash. Its absence from the inner ZIP is consistent with the frozen `RUNTIME_SIDECAR_AFTER_BUNDLE_HASH` design.

## Zero-science boundary

```text
target model loaded               false
diagnostic executable launched    false
GPU dispatch                      false
performance measurement           false
fresh TTFT observations           0
scientific result                 NONE
mechanism adjudication            false
```

No scientific outcome has been exposed.

## Repair-budget effect

No defect was observed in the frozen research package.

Therefore:

```text
frozen package repair budget
max        1
consumed   0
remaining  1
```

The local execution required no package repair.

## Effect on historical GitHub P8 failures

Historical GitHub Actions runs remain valid audit evidence:

- run `35678683835`: orchestration failure before job creation;
- run `35678861153`: hosted Windows job startup failure before an observable step.

The exact frozen runner executed zero times in those runs.

P8L now demonstrates that the unchanged frozen package **is executable** on the preregistered local Windows substrate.

Therefore those historical failures do not establish a frozen-package defect and do not block continuation of TTFT_M2.

The narrower root cause of the hosted-runner startup failure remains:

`UNRESOLVED`

No unsupported attribution to billing, runner allocation, account policy or another external cause is made.

## Stage effect

```text
M2_P8L  COMPLETE_PASS_VALID_LOCAL_BUILDONLY
M2_P9   NOT_OPENED_ELIGIBLE_FOR_EXPLICIT_AUTHORIZATION_GATE
```

P9 is **not** opened by this adjudication.

The next admissible step is:

`M2_P9_EXPLICIT_FRESH_MECHANISM_IDENTIFICATION_AUTHORIZATION_GATE`

That gate must separately decide whether the already-frozen fresh mechanism-identification design is authorized for scientific execution.
