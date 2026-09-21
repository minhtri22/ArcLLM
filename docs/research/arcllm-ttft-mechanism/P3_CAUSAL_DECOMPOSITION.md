# ARCLLM_TTFT_M1 — P3 Causal Decomposition

The core design choice is **shared resource footprint**.

For the primary test, both safe and fast decode chains must be prepared in every process. The only changed variable is which decode chain is executed during conditioning before an identical prefill.

This blocks a major confound:

```text
different prepared objects
!=
different executed warmup
```

D0 then decomposes TTFT into:
- total CPU-observed prefill;
- command record/submit/wait;
- GPU timestamped prefill;
- CPU top-2 tail.

D1 separately tests prepared-resource footprint with no decode execution.

D2 uses neutral prefill-only conditioning with counterbalanced fresh-process order to test process/order/DVFS bias.

If these arms cannot distinguish the mechanisms, the correct result is **NOT_IDENTIFIED**, not post-hoc intervention selection.
