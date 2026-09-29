# CORE-0B — Zero-Science Preflight Final Adjudication

Date: 2026-09-29

## Verdict

**PASS_CORE0B_ZERO_SCIENCE_PREFLIGHT**

Open findings: **0**

The preflight is formally closed. The 40-request primary collection is now authorized by a separate exact authorization artifact, but it has **not** been executed.

## Frozen identities

- ArcLLM canonical parent: `7a5672112dc22de15f0e9bb6445508fbb3099b15`
- benchmark implementation HEAD: `7754d898f2e4c09263e583ac2e24e2938e1f9d7a`
- final preflight evidence commit: `f1eff1486018e2907957a83054dcf9151699baff`
- llama.cpp: `v0.4.1@b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`
- exact model SHA256: `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`
- ArcLLM executable SHA256: `6DCDC89154307A486115D8904DF1B434B34A812CDA8A16A577142B3E778765BF`
- llama cold adapter SHA256: `3B289AD547288E59CC0E463E45A01207C94D6F34758DC48D475F7E6D59B5C1C6`

## Functional qualification

ArcLLM W-S and W-C both passed the frozen current-canonical invariants:

- 441 prefill dispatches / 1 submit;
- 469 decode dispatches per step / 1 submit;
- 31 decode steps;
- route A = 0, route B = 31;
- acquire/evict = 1/1;
- B allocation/materialization/validation/release = 1/1/1/1;
- P1/P3/P0 = 1/0/0;
- finite = true;
- exactly 32 generated tokens.

Pinned llama.cpp passed both workloads with:

- Vulkan runtime present;
- full offload = 29/29 layers;
- n_ctx/n_batch/n_ubatch = 4096/256/256;
- threads/threads_batch = 8/8;
- KV K/V = F32/F32;
- exactly 32 generated tokens;
- finite logits.

Cross-system generated-token identity is not required by the frozen contract.

## Measurement authority

The only primary performance authority is:

`UNINSTRUMENTED_CHILD_WALL_MS`

Primary runner rules:

- no Token-XRay;
- no hardware counters;
- no profiler;
- no resource sampler;
- fail closed without exact science authorization;
- enforce exact model, executable, critical Git blobs, and all 19 active SPIR-V hashes before collection;
- execute all 40 requests without early stop.

## Pre-science repairs

Two infrastructure/tooling issues were found and repaired before science:

1. Windows nested CMake/MSBuild path length prevented the exact llama Vulkan build. Build paths were shortened; llama source/commit/build options were unchanged.
2. The independent checker initially mistook its own VTUNE/NSIGHT/NSYS environment-blocking strings for profiler invocation. The checker was repaired and rerun.

Neither repair changed ArcLLM runtime/kernel, llama source, model, workloads, metrics, measurement authority, or decision rule.

## Boundary

No primary performance request has been executed.

```text
science_execution = false
primary_requests_executed = 0
performance_claim_created = false
mechanism_selected = false
CORE0C_authorized = false
```

Next permitted action: run exactly one complete CORE-0B primary collection under `config/core0b_science_authorization.json`.
