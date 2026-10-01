# CORE-0E-R1 — Marker Resolution Recovery Preregistration

## Status

**PREREGISTERED. No implementation or measured execution is authorized by this document.**

Parent CORE-0E is formally closed as `STOP_CORE0E_COLLECTION_INCOMPLETE`. The failed 24-request collection is quarantined from R1 primary evidence.

## Frozen reason for recovery

The only parent information admitted into recovery design is structural: 24/24 requests executed; 4/12 matched rows were invalid; all four invalid arms were llama.cpp `COMBINED_PHASE_TRACE`; each failed only `marker_order`; and each failure had `prefill_wall_end_ns == decode_wall_start_ns`.

No parent wall-time, GPU-time, P0–P4 magnitude, excess ratio, phase share, phase winner, or mechanism inference may be used to tune R1.

## Sole contract change

CORE-0E v0.2 required strict ordering at every adjacent marker boundary. R1 changes exactly one relation:

```
parent:   prefill_wall_end_ns <  decode_wall_start_ns
R1:       prefill_wall_end_ns <= decode_wall_start_ns
```

All other adjacent marker relations remain strict. Therefore:

```
child_start < prefill_start < prefill_end <= decode_start < decode_end < child_end
```

`decode_transition = decode_start - prefill_end` must be non-negative. Equality means a zero transition at the observed clock resolution; it must not be manufactured by clamping or epsilon injection.

Artificial sleep, spin, fence, or any other delay inserted solely to force a positive marker gap is forbidden.

## Science preserved exactly

Model, llama.cpp and Token-XRay pins; teacher-forced trajectories; W-S/W-C workloads; 3 blocks per workload; 24 requests; system/mode order; adjacent pairing; P0–P4 formulas; E1–E5 thresholds; phase-Amdahl thresholds; successor routing; and no-rerun/no-early-stop rules are inherited unchanged from CORE-0E v0.2.

The preferred implementation is validator/checker-only. Parent benchmark binaries and shaders should remain byte-identical if static audit confirms no binary change is required.

## Recovery evidence

R1 must use a fresh namespace `core0e_r1_measured_*`. No row from the failed parent collection may be replayed, substituted, or promoted into R1 evidence.

This is the only marker-resolution recovery. If R1 collection validity fails again, the marker-recovery line closes; no R2 rescue is permitted.

## Next gate

`CORE0E_R1_IMPLEMENTATION_STATIC_PREFLIGHT_AND_EXECUTION_LOCK`

That gate may change only validator/checker/runner plumbing needed for this preregistered boundary rule and must remain zero-science.
