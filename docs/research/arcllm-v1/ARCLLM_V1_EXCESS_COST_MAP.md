# ArcLLM v1 — Cross-Runtime Excess-Cost Map v0.1

**Date:** 2026-09-25  
**Status:** M2 complete; M3 deferred

## Evidence join

This map joins four independent evidence layers:

- M0: exact per-layer Q4_K/Q6_K tensor census;
- M1: current post-I002 Arc Vulkan tick-share localization;
- M2: exact pinned llama.cpp Vulkan calibrated timing by semantic matmul shape;
- I003: matched practical decode-token latency scale.

For each workload, practical family attribution is derived as:

`family attributed ms = matched I003 token ms × family timing share`

The result is a cross-evidence attribution, not a directly paired family microbenchmark.

## Coverage

The seven mapped quantized semantic groups cover:

- **4,369,225,728 / 4,370,560,992 bytes = 99.969%** of logical one-pass weight payload;
- Arc: **97.67% W-S / 95.75% W-C** of M1 ticks;
- llama: **92.60% W-S / 90.77% W-C** of M2 calibrated Vulkan time.

## Excess map

| Semantic family | Weight payload | 136 GB/s optimistic floor | W-S Arc/llama | W-C Arc/llama | Geomean excess | Mean practical gap |
|---|---:|---:|---:|---:|---:|---:|
| LM-head Q6_K | 447.07 MB | 3.287 ms | 36.05× | 21.07× | **27.56×** | **204.3 ms** |
| FFN gate+up Q4_K split-K | 2,138.70 MB | 15.726 ms | 4.50× | 6.34× | **5.34×** | 160.3 ms |
| FFN-down Q6_K | 779.74 MB | 5.733 ms | 9.67× | 10.40× | **10.02×** | 132.2 ms |
| FFN-down Q4_K | 534.68 MB | 3.931 ms | 13.98× | 18.05× | **15.89×** | 131.4 ms |
| Q+O Q4_K | 404.62 MB | 2.975 ms | 6.51× | 8.80× | **7.57×** | 59.4 ms |
| K+V(Q4) | 43.35 MB | 0.319 ms | 23.84× | 35.35× | **29.03×** | 41.6 ms |
| V(Q6) | 21.07 MB | 0.155 ms | 17.27× | 24.28× | **20.47×** | 14.2 ms |

## Hardware interpretation

Pinned llama's mapped quantized families sit roughly **2.2–4.6×** above the optimistic weight-only floor.
Arc sits roughly **12.5–135×** above the same family floors.

The important contrast is gate/up. After I002 subgroup split-K it carries almost half the logical
weight payload yet its cross-runtime excess is only ~5.34×. The remaining Arc Q/K/V/O/down/lm-head
families still use row-owned serial-K mappings and show much larger time-per-weight pressure.

This does not prove that every residual gap is caused by the same microarchitectural mechanism.
It does provide enough causal direction that quiet-host counters are not the next best experiment.

## M3 decision

`DEFER_M3_QUIET_HOST_COUNTERS`

Counters could separate physical bandwidth, cache-hit behavior, occupancy and issue utilization.
But those details do not change the next discriminating question: **does replacing one residual
row-serial-K mapping with subgroup/workgroup K-parallel execution causally collapse the excess?**

## Next causal sentinel

First sentinel: **Q4_K FFN-down, K=18944, rows=3584**.

Why this one:

1. FFN-down is the largest current Arc coarse family (~35.1% combined).
2. Q4 down alone has ~15.89× cross-runtime geomean excess.
3. It uses Q4_K, the same quant format as the already-proven gate/up split-K mechanism.
4. Its geometry is deliberately inverted/harder than gate/up: long K=18944, rows=3584.
5. A strong transfer directly supports the residual serial-K mapping hypothesis; failure is informative and would justify M3 before expanding to Q6/LM-head.

No implementation is authorized by this map. Next artifact: `H1_Q4_DOWN_SPLIT_K_SENTINEL_SPEC`.
