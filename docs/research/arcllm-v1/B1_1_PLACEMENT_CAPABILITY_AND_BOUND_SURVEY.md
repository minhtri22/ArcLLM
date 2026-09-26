# B1.1 — Placement Capability and Bound Survey

Status: PASS — P1/P3 SHORTLISTED; P2 CAPABILITY-GATED; P4 KILLED.
No new placement performance execution occurred.

## Core result

A / Split-K32 is the B1 baseline. B / EXEC148 is the alternative.
On the current Lunar Lake host, CPU, Arc 140V GPU and Intel AI Boost NPU are separate compute domains attached to system UMA. Creator choice therefore does not create a separate active capacity pool for the 549,527,552-byte EXEC148 image.

Moving creation CPU -> GPU/NPU can change creation latency and contention, but does not remove active system-memory residency. P3/offline sidecar moves creation out of inference runtime, but when B executes the image still has to be available to the GPU.

## Bound

Source Q4_K bytes: 534,675,456.
EXEC148 bytes: 549,527,552.
Minimum transform traffic: 1,084,203,008 bytes.
At the frozen 136 GB/s IP roof, the bandwidth-only floor is 7.972 ms.
Current P0 CPU-direct materialization is 231.6382 ms, about 29.1x above that floor.

B gains versus A are 14.184244 ms/token for W-S and 6.422135 ms/token for W-C.
Current P0 therefore already crosses A at about 16.33 W-S tokens or 36.07 W-C tokens.

## Feasibility matrix

P0 CPU direct UMA: PROVEN. Keep as reference and already-usable architecture. No new placement benchmark required.
P1 GPU create in-place: FEASIBLE BY EXISTING VULKAN DATA PATH. Shortlist for bounded experiment after exact materializer/correctness/lifecycle lock.
P2 NPU create/maintain: HARDWARE PRESENT, INTEROP CAPABILITY EXISTS IN PLATFORM APIs, exact EXEC148 transform and current ArcLLM buffer interoperability unproven. Capability probe only; no performance benchmark yet.
P3 offline prematerialized sidecar: ARCHITECTURALLY FEASIBLE. Shortlist. Runtime transform cost becomes zero, but cold/warm load and active residency remain explicit costs.
P4 audio DSP / other idle engine: KILL for current B1. Hardware exists, but no proven application-level exact-transform + Vulkan-memory path is available to ArcLLM.

## Architecture policy implication

If EXEC148 is valid and resident, B can be selected.
If it is absent, invalid, not amortized, or evicted under memory pressure, fall back to A.
P1 and P3 are methods for obtaining a valid B image more cheaply or earlier; they do not change B semantics.

## Next

B1.2_P1_P3_PLACEMENT_EXPERIMENT_DESIGN_AND_PRELOCK

Design P1 and P3 as independent bounded studies. Keep A baseline and EXEC148 semantics frozen. P2 may receive a separate zero-science compile/memory-interop capability probe and does not block P1/P3.
