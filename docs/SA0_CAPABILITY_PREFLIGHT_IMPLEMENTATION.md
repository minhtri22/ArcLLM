# SA0-CAP Zero-Science Capability Preflight Implementation

**Date:** 2026-09-21  
**Parent SA0:** `1ade625ae60acaa9ebeb43890d53a198c200d576`  
**State:** probe implementation statically locked; exact-device execution pending.

## Boundary

SA0-CAP is a hardware/API inventory step, not a performance experiment.

It performs:

```text
vkCreateInstance
→ enumerate physical devices
→ physical-device properties/features/extensions
→ queue-family properties
→ memory heaps/types
→ subgroup properties
→ optional cooperative-matrix property enumeration
→ JSON evidence
```

It performs:

- **NO model load**;
- **NO shader module** creation;
- **NO compute pipeline** creation;
- **NO dispatch**;
- **NO Q2/Q3 workload**;
- **NO successor GEMM shader**.

The probe deliberately does not create a logical Vulkan device.

## Exact target gate

The Windows wrapper requires the Core Ultra 7 258V + Arc 140V machine and the Q3 reference Windows GPU driver `32.0.101.8860`.

The Vulkan raw probe independently requires Intel vendor identity, Arc device selection, Vulkan >=1.2, compute queue, timestamp capability, compute subgroup, compute limits and memory topology.

## Optional capabilities

Subgroup-size-control, extended scalar/subgroup features and cooperative-matrix properties are recorded, not required for the primary SA-H1a hypothesis.

Their absence only removes an optional implementation route.

## Evidence chain

`tools/build_sa0_cap.ps1` builds one standalone query executable against the existing pinned/installed Vulkan SDK. It compiles no shader.

`run_sa0_capability_preflight.ps1` runs static QA, exact-machine gates, BuildOnly, then one property-query process. It packages:

- raw Vulkan capability JSON;
- preflight lock;
- BuildOnly manifest;
- frozen SA0-CAP contract;
- parent SA0 specification QA.

The returned bundle must be independently adjudicated before SA1-P is opened.
