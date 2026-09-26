# Phase 2 Generic Policy Engine over Registry

## Goal

The policy engine consumes the generic registry and runtime state keyed by opaque IDs. It must not know primitive-family names, quant formats, representation layouts, hardware APIs, benchmark profile names, or frozen threshold literals.

A new family becomes selectable by registering descriptors and providing runtime state for its opaque IDs.

## Runtime inputs

Primitive state is keyed by `PrimitiveId`:

- resident;
- identity valid;
- execution available;
- residency lease granted.

Acquisition state is keyed by `AcquisitionPathId`:

- available;
- vetoed.

This prevents the policy request from growing family-specific fields.

## Selection order

For a valid capability/profile pair:

1. Resolve the fallback primitive and validated evidence context.
2. Resolve the profile-specific resident preference.
3. If the preferred primitive is resident, apply its lifecycle descriptor first.
4. If it remains valid and the request is inside-domain, reuse it without reapplying creation thresholds.
5. If the request is outside-domain, route fallback and preserve residency only when the lifecycle descriptor says so.
6. If the preferred primitive is absent, scan validated acquisition paths targeting it.
7. A path is eligible only when runtime-available, not vetoed, its target lease is granted, and the profile threshold is met.
8. Select the lowest numeric priority among eligible paths; equal priority is deterministically tied by opaque acquisition ID.
9. If no path is eligible, route fallback.

The engine performs no backend action and no hidden fallback acquisition chain. It only returns one policy decision.

## Evidence-state behavior

Only `VALIDATED` descriptors participate in active decisions. `DISABLED_BY_EVIDENCE`, `CAPABILITY_GATED`, and `HISTORICAL_REFERENCE` remain queryable knowledge but are not active candidates.

## Reference fidelity

The reference Q4K-down registration must reproduce the frozen B1 policy for the complete zero-science state matrix used by the policy QA. This is a migration fidelity requirement, not new performance evidence.

## Scalability

A synthetic independent family is used to prove that the same engine handles different opaque IDs, residency size, acquisition priorities, thresholds and lifecycle states without source changes to the engine.

## Boundary

The engine is still decision-only. It does not bind acquisition/primitive IDs to Vulkan, files, CPU code, or other execution mechanisms.

Next: `PHASE2_GENERIC_BACKEND_BINDING_INTERFACE`.
