# CORE-0B — Final Scientific Adjudication

Date: 2026-09-29

## Verdict

**PASS_CORE0B_MATCHED_REQUEST_CHARACTERIZATION_COMPLETE**

This is a PASS that the preregistered characterization completed validly. It is **not** a claim that ArcLLM performance passed a competitive threshold.

## Frozen comparison

- current canonical ArcLLM product runtime;
- exact model SHA256 `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`;
- exact pinned llama.cpp `v0.4.1@b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`;
- Vulkan baseline with 29/29 layers offloaded;
- W-S and W-C;
- 32 generated tokens;
- uninstrumented child-process wall time as the only performance authority.

Dataset:

`results/core0b_primary_20260929T033358387271Z`

Frozen dataset commit:

`3985a2ac1697225489da01564dbc3e49a298ef69`

## Collection integrity

Exactly 40 primary requests were executed as 20 adjacent matched pairs.

All four cells retained 5/5 valid pairs:

| Cell | Valid pairs | Median ArcLLM / llama | Min | Max | MAD |
|---|---:|---:|---:|---:|---:|
| A / W-S | 5/5 | 6.233259× | 5.665063× | 7.935739× | 0.568196 |
| A / W-C | 5/5 | 9.145345× | 8.846757× | 11.033835× | 0.284208 |
| B / W-S | 5/5 | 7.594498× | 5.123283× | 7.920242× | 0.325745 |
| B / W-C | 5/5 | 6.920212× | 5.311344× | 8.417752× | 1.182228 |

The preregistered global statistic is the geometric mean of the four cell medians:

```text
ArcLLM / llama.cpp = 7.398325462576×
```

Independent recomputation reproduced every pair ratio, every cell statistic, and the global geometric mean exactly within numerical tolerance. Open findings: **0**.

Greedy outputs were repeat-stable within each arm/workload across all repeated requests. This is diagnostic consistency, not an additional outcome gate.

## Scientific interpretation

At the **current product request boundary**, canonical ArcLLM has a large latency gap relative to the exact pinned llama.cpp Vulkan baseline under both frozen workloads and both sessions.

The result includes the product-visible consequences of ArcLLM's current request-scoped runtime lifecycle. It therefore does not isolate kernel compute alone.

Absolute timing changed materially across the collection, especially in later Session B requests, but the blocked/order-reversed design retained valid adjacent comparisons. CORE-0B does not assign a cause to that nonstationarity.

## Boundaries

CORE-0B does **not** establish:

- a persistent-session comparison;
- warm steady-state decode tokens/s;
- TTFT after preloaded model/session state;
- a kernel-level mechanism for the gap;
- that any individual ArcLLM kernel is 7.398× slower;
- an NPU result;
- a mechanism to optimize next.

Token-XRay, hardware counters, profilers and resource samplers were absent from the primary collection.

## Consequence

CORE-0B is formally closed.

The performance authority remains the uninstrumented CORE-0B dataset. A diagnostic successor may now open:

`CORE-0C — Token-XRay current-runtime localization`

CORE-0C must preregister its trace, observer-effect and attribution contracts before any instrumented inference. Its trace timing may localize excess cost but may not replace CORE-0B performance authority.
