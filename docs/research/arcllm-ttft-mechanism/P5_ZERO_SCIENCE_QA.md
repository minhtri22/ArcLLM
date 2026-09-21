# ARCLLM_TTFT_M1 — P5 Zero-Science Program QA

**Date:** 2026-09-22  
**Result:** `PASS_ZERO_SCIENCE_TTFT_M1_PROGRAM_QA`

## Scope

P5 audits the complete P0-P4 package before any implementation or fresh measurement.

## Zero-science boundary

The branch was compared against the exact ANL64 terminal parent HEAD `0e40b3affe4f6add9ce23921687b2659017c95d3`.

Observed branch changes are limited to:
- new research configuration;
- research documentation;
- the fresh program lineage;
- manifest bookkeeping.

There are no changes to:
- `src/`;
- `shaders/`;
- execution runners;
- tests;
- binaries.

No model was loaded and no GPU/timing science was executed.

## Independence QA

PASS:
- new branch;
- fresh lineage;
- exact parent P7 binding;
- parent ANL64 remains terminal;
- P6 timings are historical hypothesis-generation material only;
- no parent threshold or outcome is reinterpreted.

## Prior-art QA

PASS:
- paper review precedes implementation;
- source implementations are pinned where inspected;
- external performance claims are not admitted as local evidence;
- serving-level scheduling mechanisms are explicitly separated from locally transferable Vulkan/runtime invariants.

## Path / hypothesis QA

PASS:
- TTFT `t0→t1` boundary is explicit;
- exact prefill-source equality is recorded;
- planner overhead is rejected as a direct measured-TTFT cause;
- Q4_FAST-in-prefill is rejected;
- remaining hypothesis set is finite and falsifiable;
- H-ART is static-first;
- H-NULL and unresolved/mixed terminal states are predeclared.

## Causal-design QA

PASS.

Frozen 2×2 design:

```text
SP = SAFE   + PREFILL_ONLY
SF = SAFE   + FULL_INFERENCE
QP = Q4FAST + PREFILL_ONLY
QF = Q4FAST + FULL_INFERENCE
```

Future execution, if separately authorized:
- one diagnostic binary;
- one exact common prefill path;
- W-S + W-C;
- two independent process sessions;
- five measured observations per arm/workload/session;
- 80 fresh TTFT observations;
- zero parent timing reuse.

Primary endpoint: TTFT.

Decomposition telemetry is secondary and cannot override the primary gate.

## Falsification / STOP QA

PASS:
- 1.10 materiality threshold frozen before new data;
- workloads/sessions adjudicated independently;
- no pooling to rescue;
- one infrastructure-only repair maximum;
- full replay after a valid repair;
- second F0 stops;
- valid negative is terminal;
- stable-unresolved and mixed-reproduction outcomes are explicit;
- no post-hoc mechanism creation.

## Future intervention governance

At most one mechanism-specific intervention can be opened after fresh identification.

Any future intervention must demonstrate with fresh evidence:
- semantic preservation;
- decode ratio >=1.10;
- E2E ratio <=0.90;
- TTFT ratio <1.10.

This requirement cannot retroactively change parent ANL64.

## QA result

```text
open findings = 0
P0-P4 package = PASS
P5 = COMPLETE_PASS
```

P6 is **eligible only for a separate bounded diagnostic implementation authorization gate**.

Current authorization remains:

```text
implementation       FALSE
build                FALSE
model load           FALSE
GPU dispatch         FALSE
performance timing   FALSE
```

No local `.ps1` is required yet.

Next: explicit P6 bounded diagnostic implementation authorization gate.
