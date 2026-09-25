# Q4_DOWN_ARCHITECTURE_CAUSAL_DECOMPOSITION

Status: **FROZEN PARENT STUDY — NO IMPLEMENTATION OR FRESH PERFORMANCE EXECUTION AUTHORIZED**

## Scientific question

For the exact 14-layer Q4_K FFN-down family, decompose the observed latency gap into:

```text
work-decomposition effect
+
execution-representation effect
+
interaction effect
```

M3-B mechanistically supports residual row-serial-K dependency pressure and separately shows
Q4-down device-read amplification. This study deliberately tests those as two orthogonal factors.

## Exact target

```text
quant  = Q4_K
K      = 18944
rows   = 3584
batch  = 1 decode
layers = 3,4,6,7,8,11,12,14,15,17,18,19,21,22
family logical weight bytes = 534,675,456
```

## 2x2 design

```text
                         REPRESENTATION
                  storage-native     materialized RAM
                ┌────────────────┬────────────────┐
SERIAL-K        │ 0 BASELINE     │ B MATERIALIZE  │
                ├────────────────┼────────────────┤
SPLIT-K32       │ A DECOMPOSE    │ AB COMPOSE     │
                └────────────────┴────────────────┘
```

A may change only work decomposition. B may change only representation. AB must be literally
the exact A decomposition consuming the exact B materialized image; no AB-only optimization.

### A — work decomposition

One native subgroup32 cooperates on one output row. The source Q4_K representation stays unchanged.
No width search (64/128) occurs in this parent study.

### B — execution materialization

One-time CPU materialization before measured inference. B remains Serial-K and preserves k=0..18943
accumulation order. The image may reorder bytes and duplicate metadata, but may not fully dequantize
to FP16/F32 or change logical Q4_K values. There is zero per-token CPU model math.

### AB — composition

Exact B image + exact A Split-K32. Nothing else.

## Evaluation vector

Primary: calibrated Q4-down component latency. For each measured inference, sum the 14 Q4-down
dispatch durations at every decode step and take the median of the 31 family sums.

Mechanism counters at decode index 15:

```text
GPU_MEMORY_BYTE_READ                 → device_read / 534,675,456
XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION
XVE_STALL
XVE_STALL_SBID
```

Counter timing is never primary timing.

Correctness is a hard gate:
- exact 32 generated token IDs and known workload hash;
- component max_abs <= 0.02;
- component RMSE <= 0.005;
- B/AB must reconstruct exact logical Q4_K tuples for all 14 tensors.

Final logits/hidden hashes are recorded but need not be bit-identical because Split-K changes FP32
reduction order.

Architecture cost:
- materialization wall time;
- extra resident RAM bytes and ratio;
- token break-even;
- pairwise total-cost crossover curves.

## Timing campaign

Eight paired blocks per workload, after one unmeasured warmup per arm/workload:

```text
1  0   A   B   AB
2  A   B   AB  0
3  B   AB  0   A
4  AB  0   A   B
5  AB  B   A   0
6  0   AB  B   A
7  A   0   AB  B
8  B   A   0   AB
```

Every arm occupies every ordinal position exactly twice.

## Factorial decomposition

For workload-specific cell latencies `L0, LA, LB, LAB`:

```text
G_A   = L0 - LA
G_B   = L0 - LB
G_INT = LA + LB - L0 - LAB

L0 - LAB = G_A + G_B + G_INT
```

Positive `G_INT` means synergistic latency reduction.

Multiplicatively:

```text
g_A   = ln(L0 / LA)
g_B   = ln(L0 / LB)
g_INT = ln((LA * LB) / (L0 * LAB))

ln(L0 / LAB) = g_A + g_B + g_INT
```

Use 10,000 paired bootstrap resamples over the eight blocks, separately for W-S and W-C.

## Architecture choice is not forced

For token horizon N:

```text
C0(N)  = N * L0
CA(N)  = N * LA
CB(N)  = Tmat_B  + N * LB
CAB(N) = Tmat_AB + N * LAB
```

Report non-dominated arms and crossover token counts rather than inventing an arbitrary deployment
horizon. A representation may reduce traffic yet fail architecture selection if latency does not
improve or its amortization is poor.

## Frozen next sequence

```text
parent freeze                     ← THIS COMMIT
      ↓
child A exact Split-K32 design
      ↓
child B exact materialized layout design
      ↓
derive AB mechanically
      ↓
zero-science QA + correctness
      ↓
independent lock verification
      ↓
4-arm timing
      ↓
4-counter mechanism campaign
      ↓
factorial recompute/adjudication
```

No implementation is authorized by this parent freeze.
