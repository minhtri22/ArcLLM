# ARCLLM_LMAX_ARCH_P1 — Preregistration Revision v0.2

Status: **FROZEN P SPECIFICATION — NO FRESH P1 OUTCOME AUTHORIZED**

This revision supersedes v0.1 only to remove the zero-model Arm-A comparator ambiguity identified during internal readiness. All other P1 scientific question, inference arms, workloads, thresholds, quantile estimator, anti-rescue rules, E→M→C unlock rules and evidence-completeness requirements remain unchanged.

## Frozen zero-model Arm A

Comparator ID: `DIRECT_LOCKED_SPSC64`.

It is used **only** for the mandatory 1,000,000-event zero-model control-path subtest. It does not replace `CURRENT_DIRECT` in the inference A/B campaign.

Exact mechanics:

- 1 producer and 1 consumer.
- Capacity exactly **64**.
- Fixed preallocated array of 64 event slots.
- FIFO state is `head / tail / count`.
- Exactly one `std::mutex` protects queue state.
- Producer blocks on `std::condition_variable not_full` with predicate `count < 64`.
- Consumer blocks on `std::condition_variable not_empty` with predicate `count > 0`.
- Producer calls `notify_one(not_empty)` after publication.
- Consumer calls `notify_one(not_full)` after removal.
- No dynamic allocation is allowed after queue initialization.
- No polling, `Sleep`, spin loop, `WaitOnAddress`, or alternative synchronization is permitted for Arm A.

Arm-A counter semantics:

- `spin_count = 0`.
- `switch_to_thread_count = 0`.
- `wait_on_address_count = 0`.
- Producer and consumer condition-variable wait counts are serialized separately.
- `wake_count` is the number of `notify_one` calls.

## Common event and timestamp semantics

Both control-path arms use the same fixed-size event payload:

- `uint64 sequence_id`;
- `uint64 publish_timestamp_ns`;
- no owning heap pointer, string, vector, or container.

For both arms:

- `publish_timestamp_ns` is captured immediately before the event becomes visible to the consumer.
  - Arm A: immediately before queue `count` is incremented.
  - Arm B: immediately before the producer performs the release sequence publication.
- `consume_timestamp_ns` is captured immediately after successful dequeue/slot consume and payload copy, before producer-unblocking notification/wake.
- `publish_to_consume_latency_ns = consume_timestamp_ns - publish_timestamp_ns`.
- The same monotonic high-resolution clock implementation is used by both arms.
- Consumer service delay is applied **after** `consume_timestamp_ns` is captured and before attempting the next event.
- Expected consumed sequence is exactly `0..999999`.

## Symmetric backpressure semantics

A `full_backpressure_observation` is counted **at most once per event** when the producer first observes no free capacity before it blocks.

- Arm A full condition: `count == 64`.
- Arm B full condition: the target sequenced slot is unavailable for the current producer sequence.
- Repeated wakeups/rechecks for the same event do not add additional observations.
- Formal rate: `full_backpressure_observation_rate = full_backpressure_observations / 1,000,000`.
- For each delay cell qualifying a `CONTROL_PATH_ONLY` claim, Arm B must satisfy:
  `B backpressure rate <= A backpressure rate`.

## Comparator fairness invariants

The two zero-model arms must share:

- producer count 1, consumer count 1;
- capacity 64;
- identical event payload;
- exactly 1,000,000 events per arm per delay;
- identical service-delay placement;
- identical publish/consume timestamp positions;
- no dynamic allocation after initialization;
- identical monotonic clock;
- identical raw binary formats and lengths.

Forbidden after this freeze:

- inline/synchronous Arm A;
- unbounded Arm A;
- capacity other than 64;
- different timestamp positions;
- polling/sleep Arm A;
- alternative mutex/condition-variable queue chosen during R;
- any comparator substitution after fresh E begins.

## Required control-path manifest additions

Each arm/delay manifest must include:

- `steady_state_allocation_count`;
- `full_backpressure_observation_rate`;
- producer/consumer condition-variable wait counts;
- SHA-256 and byte lengths for both 1,000,000-entry raw binary streams.

The v0.1 evidence-completeness repair remains mandatory: inference evidence must serialize request start/end, first/full token-ready timestamps, CPU start/end, logical processor count and allocation counter boundaries so A can recompute every formal metric from primitives.

## Lifecycle state

`ARCLLM_LMAX_ARCH_P1_PREREGISTRATION_REVISION_V0_2 = FROZEN`

No P1 fresh outcome has been opened.

R is now internal machinery and may proceed autonomously. R may implement only this exact comparator and P1 ring contract, prove evidence serialization with zero-science fixtures, build/hash/freeze the executable and recover transport as needed.

The next user-facing stop is immediately before:

`ARCLLM_LMAX_ARCH_P1_E_FRESH_EXPLORATORY_ONE_SHOT`

unless R discovers another required scientific-contract change.
