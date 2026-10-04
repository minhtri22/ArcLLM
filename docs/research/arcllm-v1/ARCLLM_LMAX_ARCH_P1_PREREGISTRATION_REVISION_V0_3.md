# ARCLLM_LMAX_ARCH_P1 — Preregistration Revision v0.3

Status: **FROZEN P SPECIFICATION — NO FRESH P1 OUTCOME AUTHORIZED**

This revision supersedes v0.2 only to freeze the previously underspecified inference Arm-B request-completion wait and consumer-ready handshake. All v0.2 comparator, evidence, workload, quantile, adjudication, anti-rescue and E→M→C rules remain in force.

## Inference Arm-B completion synchronization

Arm B retains the same request-level sequenced-ring architecture and the same `std::atomic<bool> done` completion flag.

The caller must wait for request completion with exactly this low-duty policy:

1. read `done` with acquire semantics;
2. if false, execute at most **16 `YieldProcessor()` active spins**;
3. if still false, execute exactly **one `SwitchToThread()`**;
4. if still false, call **`WaitOnAddress(&done, expected=false)`**;
5. after wake, loop and re-read `done` with acquire semantics;
6. no `Sleep`, no timed polling and no unbounded `std::this_thread::yield()` loop.

The consumer completes the request by:

1. finishing canonical `generate(request)`;
2. storing `done=true` with release semantics;
3. immediately calling **`WakeByAddressSingle(&done)`**.

This completion wait is part of the P1 scientific Arm B and may not be altered in R.

## Consumer-ready handshake

The consumer-ready handshake must complete **before** both `cpu_start_100ns` and `request_start_ns` are captured.

The waiter uses the same frozen low-duty policy:

- at most 16 `YieldProcessor()` spins;
- one `SwitchToThread()`;
- then `WaitOnAddress(&consumer_ready, false)`;
- consumer publishes `consumer_ready=true` with release semantics and calls `WakeByAddressSingle`.

The handshake is outside the measured request window and therefore does not contribute to TTFT, E2E or request CPU utilization. Its wait counters are nevertheless serialized for auditability.

## Counter separation

Arm-B evidence must serialize three distinct wait-counter groups:

### Ring-slot wait counters
- active spin count;
- `SwitchToThread` count;
- `WaitOnAddress` count;
- wake count.

These counters include only slot-sequence availability waits.

### Completion wait counters
- active spin count;
- `SwitchToThread` count;
- `WaitOnAddress` count;
- wake count.

These counters include only caller waits for `done` after request publication.

### Consumer-ready wait counters
- active spin count;
- `SwitchToThread` count;
- `WaitOnAddress` count;
- wake count.

These counters include only the pre-measurement handshake.

The three groups may not be merged. Arm A serializes `null/not_applicable` for all three P1-specific wait groups.

## Scientific rationale

P0 used an unbounded caller loop:

```cpp
while (!done.load(std::memory_order_acquire)) {
    std::this_thread::yield();
}
```

which remained active for nearly the whole inference duration and may plausibly explain much of P0's CPU-utilization penalty. P1 therefore freezes completion waiting as part of the successor architecture instead of allowing R to choose it post hoc.

This remains an exploratory successor. P1 does not claim that completion waiting is the causal mechanism; if E opens M, mechanism mode must separate wait-policy effects from ring/single-writer structure.

## Lifecycle

`ARCLLM_LMAX_ARCH_P1_PREREGISTRATION_REVISION_V0_3 = FROZEN`

No P1 fresh outcome has been opened.

R may now proceed autonomously with implementation, zero-science evidence-schema QA, BuildOnly, dependency/hash/resource work and transport recovery. Any further change to the scientific arm, evidence schema, workload, thresholds or adjudication requires a new P revision before E.

The next user-facing stop is immediately before:

`ARCLLM_LMAX_ARCH_P1_E_FRESH_EXPLORATORY_ONE_SHOT`
