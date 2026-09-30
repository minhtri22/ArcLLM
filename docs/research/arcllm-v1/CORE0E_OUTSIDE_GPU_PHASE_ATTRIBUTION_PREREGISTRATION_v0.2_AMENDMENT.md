# CORE-0E — Outside-GPU Phase Attribution Preregistration v0.2 Amendment

Date: 2026-09-30

Status: **PREREGISTERED_AMENDED_NOT_AUTHORIZED_FOR_MEASURED_EXECUTION**

This amendment supersedes only the phase-arithmetic boundary in v0.1. No CORE-0E measured request had been authorized or executed when zero-science static QA found the defect.

## Defect found in v0.1

v0.1 required six strictly ordered markers:

```text
child_start
< prefill_start
< prefill_end
< decode_start
< decode_end
< child_end
```

but partitioned the child science window as:

```text
P1 + prefill_wall + decode_wall + P4
```

That leaves the positive interval:

```text
decode_transition =
decode_wall_start - prefill_wall_end
```

unassigned. Therefore the exact child-window and H-closure identities were not structurally guaranteed.

This was detected by a synthetic zero-science fixture before inference, not from measured outcomes.

## Prospective repair

Keep the same six strict markers and the same five attribution phases.

Define:

```text
decode_transition =
decode_wall_start - prefill_wall_end
```

for diagnostics only.

P3 becomes:

```text
P3_DECODE_HOST
=
(decode_wall_end - prefill_wall_end)
- G_decode
```

Thus P3 contains:

```text
prefill→first-decode transition
+
non-G residual inside the 31-step decode wall
```

It remains an arithmetic non-G residual. It must not be renamed a CPU bottleneck or any other mechanism.

## Repaired exact identities

```text
child_science_window
=
P1
+ prefill_wall_duration
+ (decode_wall_end - prefill_wall_end)
+ P4

H_system
=
P0 + P1 + P2 + P3 + P4

DeltaH
=
sum(DeltaP0..DeltaP4)
```

The separate `decode_wall_start` marker remains mandatory and must preserve strict order. Its transition interval must be non-negative and is reported diagnostically, but it is not a sixth attribution phase.

## Everything else remains frozen

Unchanged from v0.1:

- exact model and llama/Token-XRay pins;
- W-S/W-C prompts and teacher-forced continuations;
- 24-request collection design;
- CONTROL and COMBINED mode ordering;
- E0/E1/E2 transfer rules;
- G measurement definition;
- CORE-0B authority;
- 1-microsecond closure tolerance;
- E5 H-materiality rule;
- phase Amdahl formulas and thresholds;
- winner margin;
- successor routing;
- no optimization, no NPU, no mechanism selection.

Current measured authorization remains:

```text
measured_execution = false
measured_requests_authorized = 0
```

Next:

**CORE0E_IMPLEMENTATION_STATIC_PREFLIGHT_AND_EXECUTION_LOCK**
