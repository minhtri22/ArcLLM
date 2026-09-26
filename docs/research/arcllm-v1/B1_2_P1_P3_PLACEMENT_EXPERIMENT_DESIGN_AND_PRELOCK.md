# B1.2 — P1/P3 Placement Experiment Design and Prelock

Status: **FROZEN DESIGN / PRELOCK — NO IMPLEMENTATION OR PERFORMANCE EXECUTION AUTHORIZED**

## Scientific question

Can either of the two B1.1-shortlisted acquisition paths obtain the exact frozen EXEC148 image at lower runtime acquisition cost than the proven P0 CPU-direct reference, thereby expanding the reuse-horizon region in which B dominates A?

The architecture is not reopened:

- A / Split-K32 remains the default primitive in the validated Q4-down domain.
- B / EXEC148 steady-state value is frozen.
- AB remains non-target.
- B1.2 changes only how a valid EXEC148 image is obtained.

## Frozen reference

P0 CPU-direct acquisition: **231.6382 ms**.

Frozen steady-state component latency:

| Workload | A | B | B gain/token |
|---|---:|---:|---:|
| W-S | 38.373384 ms | 24.189140 ms | 14.184244 ms |
| W-C | 37.835155 ms | 31.413020 ms | 6.422135 ms |

For any acquisition cost C:

```text
H* = C / (L_A - L_B)
minimum integer reuse for strict B win = floor(H*) + 1
```

No A or steady-state B retest is a primary endpoint.

## Shared correctness

Every candidate must create/load the same 549,527,552-byte EXEC148 image.

Primary correctness identity:

```text
family SHA256 =
60565f9f0b12de4884e884d8311263df7238679745c83393a695935cd3eccbb2
```

The frozen component correctness oracle also remains active. Scientific validation time is reported separately and excluded from acquisition cost.

## P1 — GPU create in place

Hypothesis:

> The GPU can construct canonical EXEC148 directly in the final GPU-accessible UMA buffer with lower serialized runtime acquisition wall time than P0.

Primary path:

```text
resident Q4_K
   ↓
GPU exact block transform
   ↓
final EXEC148 buffer
   ↓
visibility/queue completion
   ↓
ready for B
```

Primary endpoint: median acquisition wall over 8 independent attempts.

P1 is kept only if:
1. every image passes exact correctness; and
2. median acquisition wall < 231.6382 ms.

Otherwise the current serialized P1 is killed. It may not be rescued after outcome exposure by overlap, altered EXEC148 semantics, or a new decomposition.

## P3 — offline sidecar

The sidecar contains exactly the canonical EXEC148 payload plus frozen identity metadata. Offline creation time is not a runtime endpoint.

Two runtime states are preregistered independently.

### Warm page-cache state

An untimed full preload establishes the warm state. The measured path then loads the sidecar through the frozen buffered path into the already-allocated final Vulkan buffer.

### Cold unbuffered state

The measured path uses a cache-bypassing Windows unbuffered-I/O contract. Alignment, chunking, staging and copies must be frozen before execution and all placement-specific work is included in acquisition wall time.

Each state uses 8 measured attempts and the median estimator.

Interpretation:
- warm < P0, cold < P0 → sidecar improves both states;
- warm < P0, cold >= P0 → sidecar is warm-cache-only; cold policy uses P0/A;
- both >= P0 → P3 is dominated by P0 for current B1 runtime-performance purpose.

No compression or alternate representation may be introduced after timing.

## Primary timing boundary

Included:
- all placement-specific runtime transform/read/copy/submit/wait work needed to make final EXEC148 ready.

Excluded:
- model load common to both A/B;
- final buffer allocation;
- shader/pipeline creation;
- offline sidecar creation;
- independent scientific tuple validation;
- component correctness validation.

Excluded work must be complete before the primary timer starts or happen after it stops.

## Cross-candidate policy

P1 and P3 do not race each other during their experiments. Each is compared independently to P0.

After both results are frozen, architecture policy may select the lowest valid acquisition path for the current state.

If EXEC148 is already valid and resident, acquisition cost is zero and no creator/load path is needed.

Active B residency remains 549,527,552 bytes regardless of P1/P3 and memory-pressure eviction falls back to A.

## Closed scope

B1.2 does not authorize:
- implementation;
- build qualification;
- P1 or P3 timing;
- A/B/AB retesting;
- counters;
- Token-XRay science;
- NPU performance work;
- audio/DSP work.

Next gate: **independent B1.2 prelock review**.
