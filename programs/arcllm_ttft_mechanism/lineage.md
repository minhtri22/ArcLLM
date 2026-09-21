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


## 2026-09-22 — P5 ZERO-SCIENCE PROGRAM QA PASS

The complete P0-P4 package was audited before implementation.

QA result:
`PASS_ZERO_SCIENCE_TTFT_M1_PROGRAM_QA`.

Open findings:
`0`.

Frozen QA artifacts:
- QA blob `234ea107f5d6e049a879acdea1a0c679bf0e566c`;
- QA document blob `362340c2ef6932a2d078329f5f329e576dbc1d60`.

The branch delta from parent terminal HEAD contains only research config/docs, manifest bookkeeping and this fresh program lineage. There are no `src/`, shader, runner, test or binary changes.

No model load, GPU dispatch or performance measurement occurred.

P5 confirms:
- independent-program provenance;
- prior-art/source review before implementation;
- exact local TTFT boundary;
- finite falsifiable hypothesis set;
- H-ART static-first gate;
- single-harness 2×2 causal design;
- 80 fresh future observations with zero parent timing reuse;
- frozen 1.10 materiality convention;
- semantic validity before performance;
- one infrastructure repair maximum;
- no pooling/rescue;
- explicit null, stable-unresolved and mixed-reproduction terminal outcomes;
- at most one later mechanism-specific intervention;
- parent ANL64 cannot be reclassified.

Current authorization remains zero-execution:
- implementation: false;
- build: false;
- model load: false;
- GPU dispatch: false;
- timing: false.

P6 is eligible only for an explicit bounded diagnostic implementation authorization gate.

Next:
`P6_EXPLICIT_BOUNDED_DIAGNOSTIC_IMPLEMENTATION_AUTHORIZATION_GATE`.


## 2026-09-22 — DUPLICATE-SPEC RECONCILIATION

Commit chronology established that the canonical P1-P5 package was frozen and zero-science-QA'd before a later parallel P1-P4 specification set appeared.

Canonical authority remains:
- P1 register `92682f51f97c9af87ce408629ff7c6f45d3468ac`;
- P1 review `69a5cf641ca33d36a40f1694111d97a8298000d3`;
- P2 path mapping `7c57e7d789cd2672cb9fe9342df5a18065dc46bf`;
- P2 hypotheses `559cf3a60789074045ac0c3f30c4982b99800707`;
- P3 contract `66b6ca2bf1da1e05ddc416bf8f9d73074e8f3fdb`;
- P3 document `ae33e94c8af15361e6ed5dcf804958ec9b3bab41`;
- P4 contract `fb856b27385278878114316ebd667b97fb38ba59`;
- P4 document `010c9ff3af0561998fd14949a84a8e6e8775ec04`;
- P5 QA `234ea107f5d6e049a879acdea1a0c679bf0e566c`.

Later parallel specs are retained as non-authoritative history and consumed no science.

Reconciliation artifact:
`artifacts/TTFT_M1/TTFT_M1_DUPLICATE_SPEC_RECONCILIATION_v0.1.json`,
blob `392f02cac42a2b3f52f2cacf92580a918529e180`.

## 2026-09-22 — P6 BOUNDED DIAGNOSTIC IMPLEMENTATION AUTHORIZED

P6 implementation authorization was opened only after canonical P5 zero-science QA PASS.

Authorization:
- path `config/arcllm_ttft_m1_p6_implementation_authorization_v0.1.json`;
- blob `03bf0f7ab3d2067c6aa1a9a04c8d6fc9072baaa7`;
- decision `P6_BOUNDED_DIAGNOSTIC_IMPLEMENTATION_AND_BUILDONLY_AUTHORIZED`.

Scope is limited to a new diagnostic harness and BuildOnly/static H-ART evaluation.

Production ANL64/Q2 sources and existing shaders remain immutable.

Current authorization:
- implementation: true;
- BuildOnly: true;
- model load: false;
- GPU dispatch: false;
- timing science: false;
- P7: false.

Next: implement exact P6 allowlist, then static QA + BuildOnly/H-ART gate.


## 2026-09-22 — P6 IMPLEMENTED / STATIC QA PASS / BUILDONLY LOCKED

P6 diagnostic implementation stayed within the exact authorization allowlist.

Frozen implementation:
- diagnostic source blob `88d97ddb497bfddcec191358f1e21d982c5efccf`;
- H-ART shader compiler blob `c7b7777c2023da7f0ff7fe489f844bc66069ef43`;
- native BuildOnly tool blob `1c65d419fb664889bcefc452c509628eff6858a6`;
- static package test blob `ac1e0fdea2f16598bb1665b4430340d8295083bf`;
- static QA blob `5174b0d0bc4cf80bca31ea005a3ee55d9ac0ab09`;
- BuildOnly runner blob `ed32620591aeeb3942efcc220ec6a0bfec7d124b`;
- implementation lock blob `b9f3e9b40bd99268ae7d0d4035b53055a007bec8`.

Connector-equivalent static QA established:
- diagnostic `build_prefill` is byte-for-byte equal to frozen safe Q2 source;
- Q4FAST appears zero times in measured prefill and exactly five times in decode;
- factors are exactly SAFE/Q4FAST × PREFILL_ONLY/FULL_INFERENCE;
- conditioning is completed before reset and measured t0;
- five measured attempts per future cell; no separate warmup;
- parent P6 timing artifacts are not read;
- production ANL64/Q2/Vulkan sources and existing shaders remain immutable.

P6 BuildOnly must next evaluate the mandatory H-ART static gate by independently compiling SAFE and Q4FAST artifact sets. The target model must not be loaded and the diagnostic executable must not be launched.

Current state:
`P6_IMPLEMENTATION_LOCKED_BUILDONLY_PENDING`.

Next:
`RUN_COMMITTED_RUN_TTFT_M1_P6_BUILDONLY_PS1_AND_RETURN_BUNDLE`.
