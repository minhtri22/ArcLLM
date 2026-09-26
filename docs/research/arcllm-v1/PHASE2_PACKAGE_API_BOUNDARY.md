# ArcLLM v1 Phase 2 Package/API Boundary

## Purpose

Phase 2 packages the validated Phase 1 Q4_K `ffn_down` primitives behind a stable boundary that upper layers can consume without depending on Vulkan, shaders, SPIR-V identities, sidecar filenames, or experiment harnesses.

This boundary does not generalize beyond the frozen validated domain and does not invent a workload classifier.

## Public surface

The public package surface contains:

- API version `1.0`;
- a stable capability identifier for the validated Q4_K decode `ffn_down` domain;
- stable primitive identifiers for A / Split-K32 and B / EXEC148;
- opaque evidence profile identifiers `PROFILE_0` and `PROFILE_1`;
- request/runtime state for reuse, residency lease, representation validity, and acquisition-path availability;
- a deterministic `plan()` result containing route, lifecycle action, acquisition class, reason, threshold, residency bytes, and validation requirement;
- an abstract backend adapter for acquire / validate / release / resolve-primitive;
- `apply_plan()`, which applies exactly one lifecycle decision and never hides fallback acquisition retries.

## Evidence-profile rule

`PROFILE_0` and `PROFILE_1` preserve the frozen W-S and W-C evidence profiles internally. The API deliberately does not rename them into semantic runtime categories because Phase 1 did not validate a classifier or prove that W-S/W-C correspond to universal workload classes.

A future semantic classifier may sit above this boundary only after its own evidence exists.

## Hardware opacity

Upper layers do not receive:

- Vulkan objects;
- shader or SPIR-V paths;
- EXEC148 byte layout;
- sidecar filesystem paths;
- workgroup geometry;
- timer/counter surfaces;
- placement benchmark controls.

Those remain backend implementation details.

## Capability descriptor

The single Phase 2 v1 capability advertises:

- fallback primitive: A / Split-K32;
- represented primitive: B / EXEC148;
- represented residency: 549,527,552 bytes;
- PROFILE_0 creation thresholds: primary 2, secondary 15, tertiary 17 tokens;
- PROFILE_1 creation thresholds: primary 4, secondary 33, tertiary 37 tokens.

The descriptor is evidence metadata, not a claim outside the frozen domain.

## Lifecycle contract

One plan may request at most one action:

- no lifecycle action;
- acquire B through the primary GPU-in-place path;
- acquire B through the validated cold-unbuffered secondary path;
- acquire B through the proven CPU-direct tertiary path;
- evict B.

An acquisition is not usable until exact representation validation succeeds. If acquisition or validation fails, `apply_plan()` resolves A and returns the backend failure. It does not silently try the next acquisition path. A later call may reevaluate with updated availability.

## Residency contract

The package asks for exactly 549,527,552 bytes but does not decide whether that lease should be granted. Memory-pressure policy belongs to the resource manager.

Routing A for an out-of-domain request does not imply eviction of a still-valid B representation.

## Versioning

`ApiVersion.major` changes for incompatible public contract changes. Minor changes may add backward-compatible descriptors or status/reason values.

Stable numeric IDs are package ABI identifiers, not experiment labels.

## Scope boundary

Phase 2 v1 exposes the validated primitive/policy boundary and a backend seam. It does not claim a universal operator API, a complete model-runtime API, or end-to-end production inference integration.

Lower-level placement research remains closed unless new evidence can change a package-visible routing, acquisition, retention, eviction, or accessibility decision.
