# ARCLLM_LMAX_ARCH_P1 — Readiness Scientific Contract Audit 03

Status: **P REVISION REQUIRED BEFORE CONTROL-PATH E HARNESS MAY BE IMPLEMENTED**

Internal R implementation reached the mandatory 1,000,000-event control-path subtest and found one remaining outcome-bearing ambiguity.

## Finding

P1 preregistration freezes consumer service delays at:

- 0 ns;
- 10,000 ns;
- 100,000 ns;

and freezes that the delay is applied after `consume_timestamp_ns` and before the next consume attempt.

However, it does not freeze **how the delay is implemented**.

Scientifically different implementations include:

- active busy-wait against the monotonic clock;
- `Sleep` / `sleep_for`;
- waitable timer;
- hybrid spin/yield/sleep.

These alternatives materially alter CPU scheduling, producer run opportunity, queue occupancy, full/backpressure observations and publish-to-consume latency. Therefore the choice can change the `CONTROL_PATH_ONLY` outcome.

R must not choose this after preregistration.

## Required amendment

Before the 1M harness is implemented, freeze one exact service-delay mechanism for both control-path arms.

Recommended comparator-neutral rule:

### MONOTONIC_BUSY_DELAY

For each consumed event:

1. capture `consume_timestamp_ns` as already frozen;
2. if configured delay is zero, immediately continue;
3. otherwise calculate `deadline_ns = consume_timestamp_ns + service_delay_ns`;
4. repeatedly read the same monotonic high-resolution clock until `now_ns >= deadline_ns`;
5. the delay loop performs only `YieldProcessor()` between clock reads;
6. no `SwitchToThread`, `Sleep`, `sleep_for`, `WaitOnAddress`, condition variable or waitable timer is permitted inside the service-delay mechanism;
7. service-delay loop iterations are not counted as queue/ring wait counters;
8. both arms use the identical delay helper implementation.

Rationale: this provides the same declared consumer unavailability window in both arms without introducing an OS blocking primitive that interacts differently with the two queue mechanisms. CPU cost of the synthetic service delay is common-mode and is not itself an endpoint.

The control-path formal endpoints remain event throughput, publish-to-consume latency, steady-state allocation count, exact order and backpressure observation rate. No CPU-utilization endpoint is introduced for the zero-model subtest.

No P1 fresh outcome, model inference, Vulkan execution or 1M-event performance measurement occurred during this audit.

## R progress before stop

R machinery already created, without executing outcomes:

- `experiments/arcllm_lmax_arch_p1/p1_ring.h`
- `experiments/arcllm_lmax_arch_p1/p1_runner.cpp`
- `experiments/arcllm_lmax_arch_p1/generate_instrumented_runtime.py`

These implementation files remain provisional until the scientific contract is complete and R static/build QA passes.
