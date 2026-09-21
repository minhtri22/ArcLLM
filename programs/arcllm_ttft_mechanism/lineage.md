# ARCLLM_TTFT_M1 — Lineage

Policy: append-only within this program. This lineage is independent from the closed ANL64 lineage and will be frozen when this program terminates.

## 2026-09-22 — P0 PROGRAM OPEN

A new independent program was opened on branch `research/arcllm-ttft-mechanism` from ANL64 terminal HEAD `0e40b3affe4f6add9ce23921687b2659017c95d3`.

Parent ANL64 remains terminal and is not reopened.

Origin bindings:
- P6 formal adjudication blob `56dd01850238e4131d357303d3888fd1826f6eb9`;
- P7 terminal adjudication blob `79df1758f23f724890eb302907c0b2d3045df735`;
- closed parent lineage blob `8ae08b06e901e464169264730936f2fa299af941`.

Research question:

`What mechanism causes the reproducible TTFT regression observed in the frozen ANL64 successor, and can a separately preregistered architecture remove that mechanism without sacrificing the already-demonstrated semantic correctness and decode/E2E gains?`

P6 timings are historical hypothesis-generating evidence only and are not fresh evidence for this program.

Current authorization is specification/research only. No implementation, model execution, GPU dispatch, or performance measurement is authorized.

Next: P1 prior-art + source review.


## 2026-09-22 — P1 PRIOR-ART + SOURCE REVIEW COMPLETE

Prior-art and source review was completed before any implementation.

Frozen artifacts:
- prior-art register blob `92682f51f97c9af87ce408629ff7c6f45d3468ac`;
- review document blob `69a5cf641ca33d36a40f1694111d97a8298000d3`.

Key transfer decisions:
- prefill/TTFT vs decode/TPOT phase separation is transferable;
- multi-request scheduling, chunked-prefill serving policy, disaggregated serving and prefix-cache mechanisms are not direct evidence for the isolated single-request parent P6 cells;
- Vulkan resource-reuse/pipeline-state mechanisms are locally relevant but require local causal identification.

Pinned external source implementations include llama.cpp Vulkan, vLLM, Sarathi-Serve, SGLang and FlashInfer at exact commits/blobs.

No external claim was admitted as local PASS.

## 2026-09-22 — P2 TTFT PATH + HYPOTHESES FROZEN

Static source mapping established the exact measured boundary.

The complete `build_prefill()` source region is exact-equal between the safe reference and ANL64 candidate. Q4_FAST is decode-only.

Planner construction, pipeline preparation and reset occur before TTFT `t0`.

Rejected before science:
- planner CPU construction as direct measured-TTFT cause;
- Q4_FAST executing in prefill;
- multi-request scheduler interference as an explanation of isolated P6 cells.

Frozen P2 artifacts:
- path mapping blob `7c57e7d789cd2672cb9fe9342df5a18065dc46bf`;
- hypotheses blob `559cf3a60789074045ac0c3f30c4982b99800707`.

Admissible mechanisms:
- H-ART prefill binary artifact divergence;
- H-DPIPE prepared decode-pipeline state;
- H-PRECOND full-inference warmup preconditioning;
- preregistered state interaction;
- H-NULL no stable material mechanism.

No execution was performed.

## 2026-09-22 — P3 CAUSAL DECOMPOSITION FROZEN

A single-harness 2×2 factorial design was preregistered:

```text
SP = SAFE   + PREFILL_ONLY
SF = SAFE   + FULL_INFERENCE
QP = Q4FAST + PREFILL_ONLY
QF = Q4FAST + FULL_INFERENCE
```

All measured arms must use one exact common 441-dispatch prefill path.

Future fresh design, if separately authorized:
- 2 independent processes/sessions;
- 2 frozen workloads;
- 4 arms;
- 5 measured observations per arm/workload/session;
- 80 fresh TTFT observations total;
- parent P6 timing reused: 0.

P3 was amended pre-science to make terminal inference classes logically disjoint: H-NULL now additionally requires no registered mechanism support; mixed parent-like reproduction receives its own terminal class.

Current P3 blobs:
- causal design `66b6ca2bf1da1e05ddc416bf8f9d73074e8f3fdb`;
- document `ae33e94c8af15361e6ed5dcf804958ec9b3bab41`.

No implementation or execution was performed.

## 2026-09-22 — P4 FALSIFICATION / STOP CONTRACT FROZEN

The finite adjudication order, materiality rule, semantic invalidation, one-repair infrastructure policy and no-rescue rules were frozen.

H-ART is a mandatory static first gate. Any common prefill SPIR-V mismatch stops timing.

Mechanism gates are strict across both workloads and both sessions; pooling cannot rescue a failed cell.

Terminal non-intervention outcomes are explicitly defined:
- H-NULL;
- UNRESOLVED_STABLE_TTFT_HARM;
- HISTORICAL_HARM_NOT_STABLY_REPRODUCED;
- INVALID/STOP infrastructure outcomes.

At most one mechanism-specific intervention may later be opened, and only after fresh identification supports a mechanism. No intervention tournament is permitted.

Current P4 blobs:
- falsification contract `fb856b27385278878114316ebd667b97fb38ba59`;
- document `010c9ff3af0561998fd14949a84a8e6e8775ec04`.

Parent ANL64 remains terminal and cannot be reclassified.

Next: P5 zero-science QA.
