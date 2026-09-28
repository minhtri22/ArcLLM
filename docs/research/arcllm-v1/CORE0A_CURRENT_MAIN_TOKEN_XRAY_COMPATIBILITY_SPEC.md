# CORE-0A — Current-main Token-XRay Compatibility and Observer Preflight

Date: 2026-09-29  
Branch: `research/current-main-baseline-xray`

## Purpose

This is a zero-science compatibility gate before any fresh current-main performance measurement.

It serves two independent downstream goals:

1. prepare a clean current `ArcLLM/main` matched baseline against pinned llama.cpp without allowing instrumentation to become timing authority;
2. use the same current runtime as a real consumer test for Token-XRay and produce an explicit adapter-upgrade handoff for a separate Token-XRay agent.

No performance mechanism is selected here.

## Frozen inputs

- ArcLLM canonical parent: `7a5672112dc22de15f0e9bb6445508fbb3099b15`
- Token-XRay main: `17baf9e9e561bdb5efe9904dd4cd678f9e19368d`
- llama.cpp baseline remains pinned to v0.4.1 commit `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`
- exact model SHA256: `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`
- canonical workloads: W-S and W-C
- current ArcLLM decode topology contract: 469 physical dispatches / 451 semantic nodes
- current ArcLLM prefill topology contract: 441 physical dispatches

## Measurement authority

Fresh performance science remains locked during CORE-0A.

Future CORE-0B must use uninstrumented matched runs as the latency/throughput authority.

Token-XRay timestamps, traces and hardware counters are diagnostic/localization evidence. They may not replace uninstrumented benchmark timing.

A later observer-effect check must explicitly compare:
- uninstrumented runtime;
- timestamp-only runtime;
- Token-XRay trace runtime;
- hardware-counter runtime when counters are used.

## CORE-0A questions

### A. Decode compatibility

Can the pinned Token-XRay adapter represent all current ArcLLM canonical decode semantic nodes and physical shader geometry, including Q4V4 route-B FFN-down?

### B. Prefill compatibility

Can the pinned Token-XRay adapter represent the current 441-dispatch prefill path without collapsing prefill into decode identity or losing fused-node attribution?

### C. Runtime lifecycle compatibility

Can Token-XRay represent request-scoped representation lifecycle work introduced by Q4V4:
- acquire;
- GPU materialize;
- validate;
- resident/reuse;
- evict/release;

without falsely attributing that work to a model semantic FFN-down node?

### D. Observer contract

Does Token-XRay explicitly distinguish benchmark-authority timing from instrumented diagnostic timing and preserve observer overhead as evidence?

### E. Tool self-QA

Does the pinned Token-XRay checkout pass its own Python compilation/test baseline before downstream integration?

## Decision rule

`PASS_CORE0A_TOKEN_XRAY_COMPATIBLE` requires all of:
- pinned Token-XRay baseline QA executable;
- 469/469 decode dispatch geometry support;
- 451/451 decode semantic-node support;
- prefill phase identity representable without semantic mislabeling;
- fused prefill attribution representable;
- Q4V4 lifecycle work representable separately from model semantic nodes;
- no unknown active current-main shader in the declared measurement scope;
- timing authority / diagnostic instrumentation distinction explicitly representable.

Otherwise classify:

`STOP_BEFORE_INSTRUMENTED_RUN_TOKEN_XRAY_ADAPTER_DRIFT`

A STOP here is a tooling compatibility result, not an ArcLLM performance/scientific FAIL.

## Required deliverables

CORE-0A must produce:

1. machine-readable compatibility result;
2. exact open-finding list;
3. `TOKEN_XRAY_ARCLLM_ADAPTER_UPGRADE_HANDOFF.md` for a separate agent;
4. no Token-XRay code modification on this ArcLLM branch;
5. no fresh performance execution.

Only after the Token-XRay adapter is upgraded and independently revalidated may CORE-0B/CORE-0C proceed.
