# ArcLLM v1 — I002 T1-Only Science Authorization

**Date:** 2026-09-23  
**Program:** `I002-Q4-GU-SG32`  
**Authorization basis:** `PASS_I002_ZERO_SCIENCE_PACKAGE_BUILD`  
**Implementation payload HEAD:** `cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1`  
**T1:** AUTHORIZED  
**T3:** NOT AUTHORIZED

## 1. Why authorization HEAD may differ from implementation HEAD

The preflight-qualified native executables were built from the implementation payload at `cf3580c4...`.

The authorization itself necessarily lives in a later Git commit. Requiring current HEAD to equal the implementation HEAD would make a committed authorization impossible.

Therefore the T1 runner now requires:

1. current branch = `research/arcllm-v1`;
2. implementation HEAD is an ancestor of current authorization HEAD;
3. every T1-critical Git blob matches the authorization contract exactly;
4. local preflight-built T1 executable hash matches exactly;
5. candidate SPIR-V hash matches exactly;
6. target model and environment match exactly.

The executable receives `implementation_head=cf3580c4...` as its provenance identity.

## 2. Bound preflight evidence

Returned preflight bundle:

`9AC9C320206A6AF1B99A56DED3474F309A4FFC3A5E3ADD7C212C84A0107531E5`

T1 executable:

```text
SHA256 3D3F6A5141BB505637E0429948D83FB3A73570C583CD1EB818320CD6A9A00C5E
bytes  452608
```

Candidate SPIR-V:

`B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569`

## 3. Exact T1 scope

Authorized fresh science:

```text
cells:
A/W-S
A/W-C
B/W-C
B/W-S

18 component comparisons/cell
72 total

layers:
0 / 13 / 27

decode positions:
0 / 15 / 30

operators:
gate / up
```

Correctness:

```text
max_abs <= 0.02
RMSE    <= 0.005
```

G1:

`median component speedup >= 1.50× in every cell`

## 4. One-shot guard

After all pre-science identity/model/environment checks pass, the T1 runner writes:

`results/I002_T1_SCIENCE_STARTED.json`

If that marker already exists, T1 execution stops.

Therefore:
- infrastructure/identity failures before science start may be repaired;
- after the first fresh T1 science begins, rerun is forbidden under this authorization.

## 5. Environment guard

Before science start the runner requires:

- Windows build 26200;
- Intel Core Ultra 7 258V;
- Intel Arc 140V;
- driver 32.0.101.8860;
- Balanced power scheme GUID `381b4222-f694-41f0-9685-ff5bb260df2e`;
- AC online;
- exact target model SHA/size.

## 6. T3 remains closed

`t3_execution_authorized=false`

No T3 invocation is authorized from this commit.

Only after independent adjudication of the returned T1 bundle may T3 be considered.
