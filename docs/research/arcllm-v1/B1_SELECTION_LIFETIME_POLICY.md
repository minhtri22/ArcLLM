# B1 Selection / Lifetime Policy Integration

## Scope

This closes the architecture integration step that follows the canonical B1.2 placement result. It does not reopen placement benchmarking.

The policy applies only to the frozen validated Q4_K decode `ffn_down` domain and exact EXEC148 identity already established by B1.2.

## Architecture roles

- **A / Split-K32** remains the no-extra-representation fallback.
- **B / EXEC148** remains the lower steady-state latency path when its acquisition and 549,527,552-byte residency are justified.
- **P1 GPU in-place creation** is the primary B acquisition path.
- **P3 cold-unbuffered sidecar** is secondary only when P1 is unavailable or vetoed.
- **P0 CPU-direct creation** is a tertiary proven fallback.
- **P3 warm buffered load** is disabled for the current P0-improvement policy.

## Creation policy

Creation thresholds apply only while B is absent.

| Acquisition path | W-S strict threshold | W-C strict threshold | Role |
|---|---:|---:|---|
| P1 GPU in-place | 2 tokens | 4 tokens | primary |
| P3 cold-unbuffered | 15 tokens | 33 tokens | secondary |
| P0 CPU direct | 17 tokens | 37 tokens | tertiary reference |
| P3 warm | disabled | disabled | killed for current question |

Before any B acquisition:

1. The request must remain inside the validated domain.
2. Expected future reuse of the same valid EXEC148 image must be known.
3. A 549,527,552-byte system-UMA residency lease must be granted.
4. The selected acquisition path must be available and exact-representation validation must pass.
5. An external runtime SLA may veto acquisition, but B1 does not invent or calibrate such an SLA threshold.

If no valid acquisition path clears its threshold, route to A.

## Retention policy: sunk-acquisition hysteresis

Creation and retention deliberately use different gates.

Once a valid B image is resident, its acquisition cost is sunk. B has lower measured steady-state latency than A in both frozen workloads. Therefore:

- keep and use B for any positive future reuse while the residency lease remains granted;
- do **not** fall back to A merely because remaining reuse drops below the 2/4-token P1 creation thresholds;
- do not repeatedly create and evict B around the crossover.

This hysteresis is the central B1 lifetime-policy integration result.

## Eviction policy

Evict B and route to A when any of the following becomes true:

- the 549,527,552-byte residency lease is revoked under memory pressure;
- model unload or exact model identity changes;
- quant, shape, layer set, EXEC148 schema, or canonical representation identity changes;
- the B execution path becomes unavailable;
- correctness identity validation fails;
- expected future reuse is known to be zero.

No fixed idle-time TTL is introduced. No universal free-memory percentage or byte headroom threshold is introduced because B1 measured the representation size, not the system-wide opportunity-cost curve of memory pressure.

## Memory-pressure contract

The policy delegates admission and eviction to a runtime resource manager through a lease abstraction:

`request_lease(549527552 bytes) -> GRANTED | DENIED`

and, while resident:

`lease_state -> GRANTED | REVOKED`

- DENIED before acquisition: stay on A.
- REVOKED while resident: evict B and return to A.
- GRANTED: memory pressure alone does not force a route change.

This is intentional: the exact B bytes are evidence-backed; a global memory-pressure threshold is not.

## Placement research reopen gate

Lower-level placement research is closed by default.

A new placement study may reopen only if it names a current policy state and could plausibly change one of:

- an A-vs-B creation decision for a horizon currently routed to A;
- whether B can remain resident under a resource state that currently requires eviction;
- whether B can still be acquired when P1 is unavailable at lower valid total cost;
- a representation-validity or execution-accessibility constraint.

The following are not sufficient reopen reasons:

- a faster isolated microbenchmark that does not flip policy;
- idle NPU/DSP capacity without an exact supported data path;
- additional mechanism attribution that would not alter routing, residency, acquisition, or eviction.

## State machine

```
A_ONLY
  |
  | P1 available + lease granted + H >= threshold
  v
CREATE_B_P1
  |
  | exact validation PASS
  v
B_RESIDENT
  |   |   lease revoked / invalidation / H=0
  |   v
  | EVICT_B
  |   |
  |   v
  +-> A_ONLY
```

If P1 cannot be used, P3-COLD then P0 CPU may be considered in that order under their own frozen thresholds.

## Boundary

This document is an architecture-policy lock. It does not itself authorize production runtime hook implementation or new performance execution.

Next: bounded deterministic policy evaluator + runtime-hook integration with zero-science QA only.
