# ARCLLM_LMAX_ARCH_P1 — Readiness Scientific Contract Audit 02

Status: **P REVISION REQUIRED BEFORE R MAY IMPLEMENT OUTCOME-BEARING P1 ARM**

During internal R review, the P0 inference runner revealed a second outcome-bearing ambiguity not resolved by preregistration v0.2.

## Finding

P0 `run_ring` performs one request publication and then the caller waits for completion using:

```cpp
while (!done.load(std::memory_order_acquire)) {
    std::this_thread::yield();
}
```

This completion wait remains active for essentially the full inference duration. The P0 runner also uses a short `consumer_ready` yield loop before timing begins.

P1 v0.2 freezes the adaptive blocking policy for ring slot-sequence waits, but does **not** freeze the request-completion wait policy. Therefore R currently has two scientifically different implementation choices:

1. preserve the P0 busy-yield completion wait, in which case the P1 slot wait change may not address the observed CPU overhead; or
2. replace completion busy-yield with a blocking wait, in which case P1 changes more than the currently frozen scientific arm and may materially alter CPU utilization.

Selecting either choice during R would be an outcome-bearing scientific design decision.

## Required amendment

Before implementation, P1 must freeze completion synchronization for the inference Arm B.

Recommended revision:

- retain the same `std::atomic<bool> done` completion flag;
- caller completion wait uses the same low-duty policy class:
  - at most 16 `YieldProcessor()` spins;
  - one `SwitchToThread()`;
  - then `WaitOnAddress(&done, expected=false)`;
- consumer sets `done=true` with release semantics and immediately calls `WakeByAddressSingle(&done)`;
- no `Sleep`, timed polling, or unbounded `std::this_thread::yield()` loop;
- the pre-timing `consumer_ready` handshake must use the same blocking policy or be completed before the measured request window; its exact treatment must be frozen.
- completion wait counters must be serialized separately from ring slot-wait counters so M can later distinguish request-completion waiting from queue-slot waiting.

No P1 fresh outcome, model inference, Vulkan execution, or 1M-event control-path performance measurement occurred during this audit.
