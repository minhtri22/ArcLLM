# CORE-0C — Token-XRay Current-Runtime Localization Preregistration

Date: 2026-09-29  
Parent: **PASS_CORE0B_MATCHED_REQUEST_CHARACTERIZATION_COMPLETE**

## Scientific question

Can the frozen Token-XRay compatibility surface localize where the **current canonical ArcLLM runtime** spends diagnostic execution time across prefill, decode, semantic nodes and Q4V4 lifecycle while preserving exact outputs/topology and explicitly measuring observer effect?

CORE-0C does **not** remeasure the competitive performance result. CORE-0B remains the performance authority.

## Frozen systems

ArcLLM remains the canonical runtime characterized by CORE-0B. No canonical runtime/kernel behavior may be changed to make tracing easier.

Token-XRay is pinned exactly to:

```text
repo  = minhtri22/token-xray
ref   = freeze/token-xray-core-v0.1-arcllm-current-compatible
HEAD  = 35f86ac68f98ffe60fc441a790274cd1f1269dfe
code  = 984908d8ab457328fd82b74090d4ab29383acc2d
```

Required compatibility surface:

```text
decode geometry  = 469/469
decode semantics = 451/451
prefill geometry = 441/441
```

## Workloads

The exact CORE-0B W-S and W-C workloads are reused.

Canonical output references are frozen directly from the first complete CORE-0B dataset:

```text
W-S → results/core0b_primary_20260929T033358387271Z/A_WS_p0_arc.json
W-C → results/core0b_primary_20260929T033358387271Z/A_WC_p0_arc.json
```

Every instrumented run must generate exactly the same 32 token IDs and retain the same canonical ArcLLM topology/lifecycle counters.

## Measurement authority

For Token-XRay traces:

```text
measurement_mode = TOKEN_XRAY_TRACE
timing_authority = DIAGNOSTIC_ONLY
```

CORE-0B's `UNINSTRUMENTED_CHILD_WALL_MS` dataset remains the sole canonical product-performance authority.

No CORE-0C timing value may replace, average with, or retroactively modify the CORE-0B result.

## Observer-effect design

For each workload, run three adjacent paired blocks:

```text
block 0: CONTROL → TRACE
block 1: TRACE   → CONTROL
block 2: CONTROL → TRACE
```

Total:

```text
2 workloads × 3 blocks × 2 modes = 12 requests
```

The control is the exact canonical uninstrumented runtime, but in CORE-0C it is used only for observer-effect calibration. It does not become a replacement benchmark dataset.

For every block report:

```text
observer_ratio = trace external wall / adjacent control external wall
observer_overhead = observer_ratio - 1
```

Report all three pairs per workload plus median, range and MAD. No post-hoc overhead threshold may promote Token-XRay timing to benchmark authority.

## Trace semantics

TOKEN_TRACE must use schema 0.2 and carry its measurement context.

Prefill and decode remain phase-separated.

For fused `ffn_gate_up_fused`:

```text
one physical dispatch
→ prefill.layer.xx.ffn.gate
→ prefill.layer.xx.ffn.up
attribution = SHARED_DISPATCH
```

Physical latency may not be split or duplicated into invented independent semantic timing.

Q4V4 lifecycle evidence remains outside the model-semantic trace:

```text
RUNTIME_LIFECYCLE_TRACE
  acquire
  materialize
  validate
  resident/reuse
  evict/release
```

Names such as `Q4V4.P1.L03` must not be coerced into model-semantic nodes.

Unknown timing remains `null`; it must never be rewritten as zero.

## Validity gates

Every one of the six TRACE requests must satisfy:

- exact 32-token identity versus the frozen ArcLLM reference for its workload;
- canonical prefill dispatches/submits = 441/1;
- canonical decode dispatches/submits per step = 469/1;
- decode steps = 31;
- route A/B = 0/31;
- acquire/evict = 1/1;
- B allocation/materialization/validation/release = 1/1/1/1;
- P1/P3/P0 = 1/0/0;
- finite = true;
- Token-XRay decode geometry = 469/469;
- Token-XRay decode semantics = 451/451;
- Token-XRay prefill geometry = 441/441;
- phase integrity PASS;
- runtime lifecycle schema/sequence PASS.

All six paired controls are also required. No selective pair/run rerun is allowed.

## Localization analysis

Only observed non-null physical dispatch timings may be aggregated.

Report separately:

1. prefill physical runtime-family timing and coverage;
2. decode physical runtime-family timing and coverage;
3. layer-wise diagnostic timing distribution;
4. semantic attribution through the frozen phase-aware mapping;
5. Q4V4 lifecycle evidence outside semantic nodes;
6. three-trace replication/dispersion per workload;
7. observer-effect measurements.

CORE-0C may identify diagnostic hotspots. It may **not** select an optimization mechanism.

That cross-system attribution decision remains downstream CORE-0D work, where CORE-0B's authoritative request gap can be joined with CORE-0C localization and reference-runtime evidence.

## Valid final classes

Exactly one:

- `CORE0C_DIAGNOSTIC_LOCALIZATION_COMPLETE`
- `CORE0C_TRACE_SEMANTICS_INVALID`
- `CORE0C_OBSERVER_CONTRACT_INVALID`
- `CORE0C_RUNTIME_OUTPUT_OR_TOPOLOGY_DRIFT`
- `CORE0C_COLLECTION_INCOMPLETE`

## Current authorization

Authorized now:

- benchmark-only Token-XRay diagnostic adapter implementation;
- static schema/geometry/lifecycle preflight;
- zero-science fixture tests;
- execution-lock construction.

Not authorized yet:

- any of the 12 measured CORE-0C requests;
- hardware counters;
- profilers;
- mechanism selection;
- canonical runtime/kernel changes.

Next step: **CORE0C_IMPLEMENTATION_STATIC_PREFLIGHT_AND_EXECUTION_LOCK**.
