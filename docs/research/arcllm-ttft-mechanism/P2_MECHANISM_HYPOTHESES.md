# ARCLLM_TTFT_M1 — P2 TTFT Mechanism Hypotheses

The local timer/source audit sharply constrains the hypothesis space.

The measured TTFT begins **after** model setup, chain preparation and buffer reset. ANL64 and the exact safe reference execute the same prefill graph through the same `execute_prepared` implementation.

The primary difference that can reach measured TTFT is therefore **state inherited from earlier work**, especially the full warmup attempt that includes 31 decode steps.

## Frozen hypotheses

### H1 — executed decode carryover

Fast decode warmup leaves a different GPU/cache/clock/driver state that slows the next identical prefill.

This is the primary hypothesis.

### H2 — prepared resource footprint

Even without executing decode, merely preparing a different decode pipeline/resource set may perturb driver state or caches enough to change later prefill latency.

### H3 — host submission path state

The observed regression may live mostly in command recording, submit, fence wait or other CPU/driver overhead rather than GPU compute.

### H4 — GPU prefill state

The same prefill commands may actually take longer on-device after fast-decode conditioning.

### H5 — process/order/DVFS bias

The P6 TTFT pattern may be primarily a process/order/clock-ramping artifact. External benchmarking practice explicitly treats warmup/clock ramping and counterbalanced order as systematic-bias controls.

No hypothesis is accepted because it is easy to optimize. Each has a distinct falsifier.
