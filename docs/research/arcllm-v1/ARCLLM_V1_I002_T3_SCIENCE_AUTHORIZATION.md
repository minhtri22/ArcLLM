# ArcLLM v1 — I002 T3-Only Science Authorization

**Date:** 2026-09-23  
**Program:** `I002-Q4-GU-SG32`  
**Authorization basis:** `PASS_I002_T1_REAL_MODEL_COMPONENT_TRANSFER`  
**Implementation payload HEAD:** `cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1`  
**T1:** CLOSED / NOT AUTHORIZED  
**T3:** AUTHORIZED

## 1. Basis

T1 returned bundle:

`9E731E1438F38B90A50E2C5AC8B91A2F69EF160265932BDB8B6CB118D189293E`

Formal T1 result:

`PASS_I002_T1_REAL_MODEL_COMPONENT_TRANSFER`

Independent T1 recomputation:

- 72/72 real-model comparisons valid;
- 72/72 correctness PASS;
- all 72 individual component speedups >1.50×;
- all four cell medians pass G1.

This closes the component-transfer question.

## 2. Authorized question

T3 asks whether the exact 56-node gate/up substitution carries through into the full 7B system.

Frozen order of evidence:

```text
T2 full-model token guard
        ↓
5 paired measurements/cell
        ↓
decode
        ↓
TTFT
        ↓
E2E
```

## 3. T2 semantic guard

Within each frozen cell, before any measured pair:

1. run one baseline full-model warmup;
2. run one candidate full-model warmup;
3. require identical 32 generated token IDs.

Failure stops that one-shot authorization before measured T3 pairs.

Final logits/hidden states need not be bit-identical because subgroup reduction changes floating-point accumulation order.

## 4. T3 frozen cells

```text
A/W-S
A/W-C
B/W-C
B/W-S
```

Per cell:

```text
1 warmup baseline
1 warmup candidate
5 measured baseline/candidate pairs
alternating pair order
```

Total measured pairs:

`20`

No outlier deletion, automatic rerun, or selective rerun.

## 5. Performance gates

### G2 — decode carry-through

Every cell:

`candidate/reference decode latency <= 0.90`

and global geometric-mean decode speedup:

`>= 1.25×`

### G3 — TTFT non-regression

Every cell:

`candidate/reference TTFT <= 1.10`

### G4 — E2E direction

Every cell:

`candidate/reference E2E latency < 1.00`

## 6. Exact payload

T3 executable from zero-science preflight:

```text
SHA256 5B067B047FD069C16C2669EDF52248552FA698B391413FDE91748422CA7FE9BD
bytes  443392
```

Candidate SPIR-V:

`B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569`

The runner binds exact implementation critical blobs and requires the authorization HEAD to descend from the implementation HEAD.

## 7. One-shot guard

After all identity/model/environment checks pass, the T3 runner writes:

`results/I002_T3_SCIENCE_STARTED.json`

If the marker already exists, T3 refuses to run.

After that marker is created, no rerun is permitted under this authorization, including T2 semantic-guard failure.

## 8. Environment guard

Before science start:

- Windows build 26200;
- Intel Core Ultra 7 258V;
- Intel Arc 140V;
- driver 32.0.101.8860;
- Balanced power scheme;
- AC online;
- exact target model SHA/size.

## 9. Stage isolation

The new authorization explicitly sets:

```text
T1 = false
T3 = true
```

The T1 runner will therefore no longer accept the current authorization.

## 10. Next

Run T3 exactly once and return the resulting bundle for independent final I002 adjudication.

No post-outcome tuning or rescue is authorized.
