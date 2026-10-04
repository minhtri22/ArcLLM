# ARCLLM_LMAX_ARCH_P1 — Preregistration Revision v0.4

Status: **FROZEN P SPECIFICATION — NO FRESH P1 OUTCOME AUTHORIZED**

This revision supersedes v0.3 only to freeze the exact service-delay mechanism used by the mandatory 1,000,000-event zero-model control-path subtest. All v0.3 inference-arm, comparator, evidence-schema, workload, quantile, adjudication, anti-rescue and E→M→C rules remain unchanged.

## Frozen service-delay mechanism

Mechanism ID: **`MONOTONIC_BUSY_DELAY`**

For every consumed control-path event:

1. capture `consume_timestamp_ns` at the already-frozen position;
2. if `service_delay_ns == 0`, return immediately and do not enter a delay loop;
3. otherwise compute:
   `deadline_ns = consume_timestamp_ns + service_delay_ns`;
4. repeatedly read the **same monotonic high-resolution clock** used for publish/consume timestamps;
5. after every unsuccessful deadline check, execute exactly one **`YieldProcessor()`**;
6. stop when `now_ns >= deadline_ns`.

Inside the service-delay loop, the only permitted operations are:

- monotonic clock read;
- `YieldProcessor()`.

The following are forbidden:

- `SwitchToThread`;
- `Sleep`;
- `std::this_thread::sleep_for`;
- `WaitOnAddress`;
- condition-variable waiting;
- waitable timers;
- any timed OS blocking primitive.

## Counter and endpoint semantics

- Service-delay loop iterations are **not** queue/ring wait counters.
- Both `DIRECT_LOCKED_SPSC64` and `LMAX_RING_P1` must call the **identical helper implementation**.
- The helper may expose an audit-only loop-iteration counter, but that counter is not part of `CONTROL_PATH_ONLY` adjudication.
- CPU utilization is **not** introduced as a zero-model control-subtest endpoint.
- Formal control-path endpoints remain:
  - exact consumed order;
  - event throughput;
  - publish-to-consume p50/p95 latency;
  - steady-state allocation count;
  - backpressure observation rate.

## Rationale

A busy delay fixes the consumer-unavailability interval using a common mechanism for both arms and avoids injecting a second OS blocking primitive whose scheduling behavior could interact differently with the queue mechanisms. Its synthetic CPU cost is common-mode and is not itself an endpoint.

## Lifecycle

`ARCLLM_LMAX_ARCH_P1_PREREGISTRATION_REVISION_V0_4 = FROZEN`

No P1 fresh outcome has been opened.

R may now proceed autonomously with implementation, zero-science evidence-schema QA, BuildOnly/hash/resource work and transport recovery. Any further outcome-bearing ambiguity requires a new P revision before E.

The next user-facing stop is immediately before:

`ARCLLM_LMAX_ARCH_P1_E_FRESH_EXPLORATORY_ONE_SHOT`
