# ArcLLM v1 — I002 Final T3 Carry-Through Adjudication

**Date:** 2026-09-24  
**Program:** `I002-Q4-GU-SG32`  
**Primary T3 authorization HEAD:** `d5f144ce9f4e4a278188038e12c41036dc47a2c0`  
**Implementation HEAD:** `cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1`  
**Returned ZIP SHA256:** `B198CCDEB1996FCCBB0A7F2BD8275CE4C74D6055E731E6278F95FA7545ABA09E`  
**Formal result:** `PASS_I002_REAL_MODEL_CARRY_THROUGH`

## 1. Governance chronology

The returned bundle was generated under science authorization schema `v0.3` and authorization HEAD `d5f144ce...`.

The bundle timestamps show the complete T3 collection existed before the later DEV_HOST governance amendment was created in the chat/repository.

Therefore this primary collection is adjudicated strictly under the original frozen v0.3 contract:
- one-shot T3;
- no automatic rerun;
- no selective rerun;
- exact frozen cells/gates.

The later DEV_HOST amendment is not applied retroactively to this primary dataset. It may govern future replications only.

## 2. Bundle integrity and provenance

Returned file SHA256 values:

```text
authorization
BA935837558589FFBA5A145CA18F7820B885B63A984394C9A7F4B0B22BEC48C3

A/W-S
7566E50A6BC616600E7E7E2355DC1D06772ABDE08065EF1A5DC3A2AF108BF72D

A/W-C
ECB6A97026E5D51BDEEE5B5F065CABE9BA9919CC465611F18C573AFBAEB9879B

B/W-C
8ECC62F01660AEE9FB577FDE09321B1C6932A6F54B1F077E64628D95207E34FD

B/W-S
3597A267358047E845D7E41BCD5BFF5ECBBFDCE8BB7486FF776F74A211B06293

run meta
6BDA0707EC121C019018D6E0896EABD2B178881B8812B84446DF813AC1938E01

returned summary
16D842A07FA0A5AC12850B905D44CD2A45C6EE37092A8342EE8B4A5125B99C3E
```

Repository audit of authorization HEAD `d5f144ce...`:
- authorization schema `v0.3`;
- T1=false;
- T3=true;
- 13/13 critical Git blobs match;
- exact candidate and implementation payload bound;
- automatic rerun=false;
- selective rerun=false.

Run metadata records all four cells exactly once.

## 3. T2 semantic guard

All four warmup baseline/candidate pairs generated identical 32-token sequences.

Workload hashes:
- W-S: `f31d4bb9fe5eb9c3`
- W-C: `471519ddc45b232e`

All 20 measured baseline/candidate pairs also satisfy:
- success=true;
- finite logits;
- dispatch census PASS;
- 32 generated tokens;
- candidate tokens == baseline tokens;
- generated hash equality.

Therefore:

`PASS_I002_T2_MODEL_SEMANTICS`

## 4. G2 — decode carry-through

Independent paired recomputation:

| Cell | Median candidate/reference decode latency | Median decode speedup |
|---|---:|---:|
| A/W-S | 0.42849 | 2.33377× |
| A/W-C | 0.49060 | 2.03830× |
| B/W-C | 0.48527 | 2.06069× |
| B/W-S | 0.41859 | 2.38899× |

Frozen requirements:
- each cell latency ratio <=0.90;
- global geomean decode speedup >=1.25×.

Observed:

`global decode geomean speedup = 2.19983×`

All four cells pass.

Additional robustness:
- all 20/20 individual decode pairs improve;
- weakest individual decode speedup = 1.49450×.

Therefore:

`PASS_I002_G2_DECODE_CARRY_THROUGH`

## 5. G3 — TTFT non-regression

Cell median paired TTFT ratios:

```text
A/W-S  0.88409
A/W-C  1.06799
B/W-C  1.00714
B/W-S  1.03442
```

Frozen requirement: <=1.10 in every cell.

All four medians pass.

Some individual pairs exceed 1.10; the worst individual ratio is ~1.18148. This does not fail the preregistered gate because G3 was defined on the per-cell paired median, not every pair.

Therefore:

`PASS_I002_G3_TTFT_NONREGRESSION`

## 6. G4 — E2E direction

Cell median paired E2E ratios:

```text
A/W-S  0.43352
A/W-C  0.58753
B/W-C  0.57458
B/W-S  0.42482
```

All are <1.00.

Equivalent median E2E speedups:

```text
A/W-S  2.30670×
A/W-C  1.70203×
B/W-C  1.74040×
B/W-S  2.35394×
```

Geometric mean of the four median E2E speedups is approximately `2.00263×`.

All 20/20 individual measured pairs also improve E2E.

Therefore:

`PASS_I002_G4_E2E_DIRECTION`

## 7. Carry-through versus component prediction

Using I001R exact gate/up shares and T1 real-model median component speedups, Amdahl device-chain predictions were approximately:

```text
A/W-S  1.89361×
A/W-C  2.01574×
B/W-C  2.00880×
B/W-S  2.09777×
```

Observed T3 median decode speedups:

```text
A/W-S  2.33377×
A/W-C  2.03830×
B/W-C  2.06069×
B/W-S  2.38899×
```

Observed carry-through is therefore consistent with, and in two cells exceeds, the simple Amdahl prediction based on historical family share plus T1 median component effect.

This supports the causal chain:

```text
dominant exact-7B gate/up cost
        ↓
subgroup32 split-K component effect
        ↓
real-model component transfer
        ↓
full-model decode carry-through
        ↓
E2E improvement
```

## 8. Formal I002 verdict

Frozen chain:

- T0 package/build PASS;
- T1 real-model component transfer PASS;
- T2 full-model token semantics PASS;
- G2 decode carry-through PASS;
- G3 TTFT non-regression PASS;
- G4 E2E direction PASS.

Therefore:

`PASS_I002_REAL_MODEL_CARRY_THROUGH`

I002 is formally closed PASS.

## 9. Claim boundary

This proves a material internal ArcLLM-v1 architectural improvement on the exact tested 7B path.

It does **not** yet prove ArcLLM-v1 beats the mature external llama.cpp baseline.

The historical Q2 external baseline is useful context but is not a fresh matched comparison for this new I002 candidate, especially given the shared DEV_HOST setting.

## 10. Next scientific step

Do not immediately optimize the next kernel family.

The next step is a fresh matched external-baseline study using:
- the closed I002 candidate;
- pinned llama.cpp baseline;
- the same model/workloads/output length;
- paired/counterbalanced execution on the same DEV_HOST;
- host-load metadata recorded but not used as an eligibility blocker.

This answers whether the validated internal ~2.20× decode and ~2.00× E2E movement materially closes the practical external gap.

Only after that matched comparison should a new post-I002 device-work profile be opened to select the next mechanism.
