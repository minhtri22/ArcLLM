# SA1-K1 Q4 Measured Adjudication

**Date:** 2026-09-21  
**Classification:** **Q4_STAGE_PASS**

Both authorized Q4 component processes completed with the exact 5-cell census, 30 baseline and 30 candidate measurements per cell, and no target-model load.

No sample was removed. The returned long-tail timing values remain part of the raw evidence; the preregistered statistic is the median for each arm.

## Frozen gate

```text
cell speedup = median baseline GPU ns / median candidate GPU ns

per-process geometric mean >= 1.50x
every cell in each process   >= 1.10x
A and B must pass independently
```

## Process A

| Cell | Baseline median ns | Candidate median ns | Speedup |
|---|---:|---:|---:|
| Q4 H3584→R3584 bias | 856,588.0 | 516,197.5 | 1.659419x |
| Q4 H3584→R3584 no bias | 863,072.5 | 518,515.0 | 1.664508x |
| Q4 H3584→R512 bias | 765,338.0 | 99,088.0 | 7.723821x |
| Q4 H3584→R18944 no bias | 13,912,915.5 | 2,608,463.0 | 5.333760x |
| Q4 H18944→R3584 no bias | 7,105,988.5 | 2,661,067.0 | 2.670353x |

```text
geometric mean = 3.1371499895x
minimum cell   = 1.6594191177x
PASS
```

## Process B

| Cell | Baseline median ns | Candidate median ns | Speedup |
|---|---:|---:|---:|
| Q4 H3584→R3584 bias | 861,952.5 | 514,921.0 | 1.673951x |
| Q4 H3584→R3584 no bias | 860,155.5 | 514,609.0 | 1.671474x |
| Q4 H3584→R512 bias | 765,442.0 | 101,536.0 | 7.538627x |
| Q4 H3584→R18944 no bias | 14,328,411.0 | 2,603,124.5 | 5.504313x |
| Q4 H18944→R3584 no bias | 7,092,499.5 | 2,665,676.5 | 2.660675x |

```text
geometric mean = 3.1475012006x
minimum cell   = 1.6714738763x
PASS
```

## Decision

Both independent processes clear the frozen 1.50x aggregate gate and 1.10x every-cell floor by substantial margins.

Therefore:

```text
SA1-K1 / Q4              Q4_STAGE_PASS
Q4 rerun                 FORBIDDEN / unnecessary
SA1-K2 Q6 implementation PERMITTED TO BE LOCKED
Q6 measurement           NOT YET PERMITTED
target-model execution   BLOCKED
Q3 reopen                BLOCKED
```

This result supports the bounded Q4 component hypothesis: subgroup-32 split-K per output row materially improves the frozen batch-1 Q4_K component shapes on the tested exact Arc 140V environment. It does not by itself establish end-to-end model speedup.
