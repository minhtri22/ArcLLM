# ArcLLM v1 — I002 T1 Real-Model Component Transfer Adjudication

**Date:** 2026-09-23  
**Program:** `I002-Q4-GU-SG32`  
**Authorization HEAD:** `045326009244db56082b281f329d87178125b7f0`  
**Implementation HEAD:** `cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1`  
**Returned ZIP SHA256:** `9E731E1438F38B90A50E2C5AC8B91A2F69EF160265932BDB8B6CB118D189293E`  
**Result:** `PASS_I002_T1_REAL_MODEL_COMPONENT_TRANSFER`

## 1. Bundle validity

Returned files:

- exact science authorization;
- four frozen cells A/W-S, A/W-C, B/W-C, B/W-S;
- T1 run metadata;
- T1 summary.

Run metadata records:

```text
authorization_head = 045326009244db56082b281f329d87178125b7f0
implementation_head = cf3580c4f6ff15491e6bcfeb2d3c42fa3e3bd9a1
executed_cells = A_WS, A_WC, B_WC, B_WS
automatic_rerun = false
selective_rerun = false
T3 authorization = false
```

The returned authorization binds the exact preflight bundle and exact T1 executable/candidate/model/environment contract.

## 2. Frozen sample census

Each cell contains the exact frozen matrix:

```text
layers       0 / 13 / 27
decode index 0 / 15 / 30
operators    gate / up

3 × 3 × 2 = 18 comparisons/cell
4 cells     = 72 comparisons
```

Independent recomputation finds the exact expected 18 unique tuples in every cell.

## 3. Correctness transfer

All 72 comparisons are finite and PASS the frozen gates.

Frozen limits:

```text
max_abs <= 0.02
RMSE    <= 0.005
```

Observed worst values:

```text
max_abs max = 3.814697265625e-05
RMSE max    = 3.627874058914e-06
```

Margins to limits are approximately:

```text
max_abs limit / observed worst ≈ 524×
RMSE limit / observed worst    ≈ 1,378×
```

Thus the exact SA1 subgroup32 mechanism transfers to real target-model weights and activations with substantial numerical margin.

## 4. Independent component-speedup recomputation

Speedup is recomputed from every raw pair as:

`baseline_ticks / candidate_ticks`

Per-cell results:

| Cell | Median | Minimum | Maximum |
|---|---:|---:|---:|
| A/W-S | 3.6897× | 3.3717× | 14.5872× |
| A/W-C | 7.7901× | 2.9232× | 19.8881× |
| B/W-C | 6.3047× | 3.6223× | 12.9169× |
| B/W-S | 9.0023× | 3.5921× | 23.6198× |

Frozen G1 threshold:

`median component speedup >= 1.50× in every cell`

All four cells pass.

Moreover:

```text
72 / 72 individual comparisons > 1.50×
global minimum = 2.9232×
global median  = 6.6359×
global geomean = 6.8655×
```

## 5. Arm-order check

The T1 harness counterbalances candidate-first/baseline-first execution.

Independent grouping:

```text
candidate-first n=36
median speedup = 6.7958×

baseline-first n=36
median speedup = 6.4716×
```

Both groups independently remain far above G1.

Therefore the PASS is not explained by one arm order.

## 6. Operator / layer / position robustness

Operator medians:

```text
gate  6.6606×
up    5.5898×
```

Layer medians:

```text
L0   3.6885×
L13 11.5356×
L27  6.6359×
```

Decode-index medians:

```text
0   5.9900×
15  6.6606×
30  6.9377×
```

Even the weakest individual observation remains 2.9232×.

The variation is real and should not be interpreted as a single fixed model-level multiplier, but it does not threaten the frozen transfer gate.

## 7. Baseline-state continuation check

T1 restores the baseline gate/up result before downstream execution.

Generated continuation replicates across sessions:

```text
W-S A/B generated hash = f31d4bb9fe5eb9c3
W-C A/B generated hash = 471519ddc45b232e
```

Both workloads produce 32 generated token IDs in both sessions.

This supports that candidate probes did not contaminate the downstream baseline continuation used to reach later frozen sample points.

## 8. Formal decision

Frozen T1 requirements:

- 72 exact comparisons;
- all finite;
- max_abs <= 0.02;
- RMSE <= 0.005;
- median component speedup >= 1.50× in every cell.

Observed:

```text
comparison census     72/72
correctness PASS      72/72
individual >1.50×     72/72
cell G1 PASS          4/4
```

Therefore:

`PASS_I002_T1_REAL_MODEL_COMPONENT_TRANSFER`

T1 is formally closed.

## 9. Scientific consequence

The SA1 exact-shape component effect has now transferred from deterministic fixtures to real target-model weights and activations.

The remaining uncertainty is system carry-through:

```text
56 gate/up substitutions
        ↓
full-model token semantics
        ↓
decode latency
        ↓
E2E
        ↓
TTFT non-regression
```

This is exactly T2/T3.

## 10. Next stage

T3 is now eligible for a separate one-shot authorization.

T3 must preserve the already frozen gates:

```text
T2 token semantic guard before measured pairs

each cell:
candidate/reference decode latency <= 0.90

global:
decode geomean speedup >= 1.25×

each cell:
TTFT ratio <= 1.10
E2E ratio < 1.00
```

No T3 science is included in this adjudication.
