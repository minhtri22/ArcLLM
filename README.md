# ArcLLM

ArcLLM is an experimental native Vulkan Compute runtime for GGUF LLM inference on Windows, currently validated on Qwen2.5-Coder-1.5B with Intel Arc 140V UMA.

## Research Governance

**ArcLLM now operates under a mandatory convergence policy:** `docs/ARC_LLM_RESEARCH_GOVERNANCE.md`.

Deferred technical findings that do not block Q1/Q2/Q3 are recorded in the append-only `docs/ARC_LLM_DEFERRED_INVESTIGATIONS.md`; they do not automatically open research work. The registry is reviewed for impact only after the main validation/final adjudication, unless an item becomes a direct blocker or evidence-integrity defect.

This governance is higher priority than P8/P9 roadmaps for all future research decisions. ArcLLM must converge through **Q1 Feasibility → Q2 Performance/Resource Envelope → Q3 Regime Advantage → fresh reproduction → final adjudication**.

The already-frozen P8-G6 experiment remains governed by its existing contract and implementation lock `7da34af2241091459b50905bfc008c7c6ba623ef`; the new governance does not retroactively change its metrics, thresholds or adjudication.

## Architecture

- Native C++17 + Vulkan Compute.
- Whole-decoder packed-weight residency.
- Direct packed Q4_K / Q6_K execution; no full-model weight expansion.
- GPU-resident KV cache across decode submissions.
- Persistent <=256 MiB weight arenas and explicit memory planning.
- Correctness-first optimization with frozen gates and append-only lineage.
- Pinned glslang 16.5.0 shader provenance.

Frozen model SHA256:

```text
6A77366395772462C84F0C4D226AC404674327CBE78C01E4391CC7E0C698851E
```

## Status

P0-P7 are CLOSED. **P8-G is frozen FAIL. P8-G1=H-AMPLIFICATION. P8-G2=H-NONLINEAR/UNEXPLAINED. P8-G3=H-FP32-ACCUMULATION. P8-G4=H-LOCAL-ERROR-NONNEGLIGIBLE. P8-G5=H-R1-ORACLE-SEMANTIC-MISMATCH. P8-G6 fresh production-semantic confirmation is READY TO RUN.**

The frozen P7 production winner is **P7-L**: tiled attention projections + P7-G FFN-down tile16 + fused Q4_K gate+up. P7-I, P7-J, P7-K, P7-N and P7-O are preserved performance negatives. See `docs/P7_CLOSEOUT.md`.

The active target remains the exact local Ollama model layer for `registry.ollama.ai/library/qwen2.5-coder`, size 4,683,074,048 bytes, SHA256 `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`. P8-B PASS proved simultaneous real Vulkan residency for the full segmented plan. P8-C PASS then proved independent mapping equivalence plus GPU numerical correctness across both oversize vocab-tensor boundaries: embedding max_abs/RMSE = 0/0; selected LM-head logits max_abs ~= 2.38e-7. P8-D is now limited to graph-level binding integration. Full inference remains forbidden.

## Roadmap

```text
P0  Bootstrap native runtime                         CLOSED
P1  GGUF tensor store / packed quantized weights    CLOSED
P2  Vulkan memory/runtime core                      CLOSED
P3  Kernel bring-up                                 CLOSED
P4  One decoder layer                               CLOSED
P5  Full decoder residency                          CLOSED
P6  GPU-resident KV + generation                    CLOSED
P7  Q4_K_M production path                          CLOSED
P8  7B memory-planned runtime                       ACTIVE (P8-G6 fresh production-semantic confirmation)
P9  Local OpenAI-compatible API                     PLANNED
P10 Activation/output-aware Q4 research              DEFERRED
```

P10 is not required for a usable runtime.

## Workflow

```text
PLAN -> CODE -> TEST -> QA -> COMMIT -> PULL/RUN ON TARGET -> JSON EVIDENCE -> REVIEW
```

Rules:
1. One bounded optimization hypothesis at a time.
2. Never lower a gate after seeing results.
3. Build/package/environment failures are not scientific negatives.
4. Run static QA before commit.
5. Authoritative experiment evidence is preserved with raw SHA256.
6. `lineage.md` is append-only.
7. The target Windows machine pulls the committed checkpoint and returns JSON evidence.

## Current run

**Q1 is CLOSED: Q1_FEASIBILITY_ESTABLISHED.**

The exact frozen Qwen2.5-Coder 7B model completed full 28-layer prefill + autoregressive cached decode on the target Intel Arc runtime. F0/F1 passed for both independent executions, F2 exact repeat passed, and the returned F3 evidence package is complete.

Authoritative Q1 evidence:
- implementation: `ec83bf42727f799e31d3900a7545e2642b3b90eb`;
- returned evidence archive SHA256: `DFB3E86C4F51D06290A2AE1ED1479C96F35AE0D329CFA5E2B7D50289DC641B43`;
- raw result SHA256: `FAD892B25C3A82F62C0CC8060F413792B8B0B73F78A7C3B4CF969E19753E63FA`;
- final summary SHA256: `36BFB413BCE31A1A6E2B77F96071162809B48FC7F566D61B34C779D90388AC95`.

Both A/B executions generated `[128275,128301,128275,128301,128275]`, with identical per-step logits and final-hidden hashes.

Q1 performance timings remain descriptive only.

The project is now on **Q2 — matched performance/resource characterization**.

Q2 contract: `docs/Q2_MATCHED_BENCHMARK_CONTRACT.md`.

Frozen baseline:
- `ggml-org/llama.cpp` v0.4.1;
- commit `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`;
- Vulkan;
- exact same GGUF;
- raw token input;
- F32 KV;
- context 4096;
- 8 CPU threads;
- batch/ubatch 256;
- all layers requested for GPU offload.

Frozen workloads:
- W-S: prompt 4, output 32;
- W-C: prompt 256, output 32;
- one warmup + five measured attempts per system/workload cell.

Q2 is characterization only. It cannot declare an advantage. Q3 remains blocked.

Status: **FINAL — FEASIBLE_NO_DEMONSTRATED_ADVANTAGE**. Q1 established real 7B end-to-end feasibility, Q2 completed matched characterization, and Q3 completed two fresh sessions / 40 measured attempts without reproducing any preregistered practical regime advantage. The current ArcLLM architecture line is closed. Any future architecture work requires a separate post-verdict intervention review; NEXUS may only seed a new hypothesis after mechanistic mapping.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.


## Post-verdict successor review

The current ArcLLM architecture remains **CLOSED** at `FEASIBLE_NO_DEMONSTRATED_ADVANTAGE`.

A specification-only post-verdict review of ArcLLM evidence, NEXUS findings and external publications selected one bounded **research reopen candidate**:

`SA-H1 — Decode-Specialized Packed-Quant Executor`.

This is not an implementation authorization. The next allowed step is SA0 causal/capability qualification only; no new kernel or target measurement is permitted yet. See `docs/POST_VERDICT_ARCHITECTURE_INTERVENTION_REVIEW.md`.


## SA0 successor qualification

SA0 is complete at the specification/causal level with status `SA0_SPECIFICATION_QUALIFIED_CAPABILITY_PREFLIGHT_REQUIRED`.

The primary successor mechanism is refined to **SA-H1a: decode-specialized batch-1 packed Q4_K/Q6_K GEMM/dataflow**. QKV and gate+up fusion remain secondary enablers; fusion-only and direct Event-Ledger transfer are explicitly rejected as primary explanations.

The current ArcLLM architecture remains closed and no successor kernel or target-model execution is authorized. The next permitted step is an **SA0-CAP zero-science exact-device Vulkan capability preflight** with no model load.


### SA0-CAP

The zero-science exact-device capability probe is now **implementation-static-locked**. It performs Vulkan physical-device queries only and cannot load the model, create shader modules/pipelines or submit compute dispatches.

Run the target-local preflight and return `results/sa0_capability_return_to_chatgpt.zip` for independent adjudication. SA1-P remains closed until that evidence is accepted.


### SA0-CAP adjudicated

Exact-device SA0-CAP is **PASS**. The Arc 140V exposes the required baseline Vulkan compute/subgroup/timestamp/memory capabilities and optional subgroup-size-control, FP16/INT8 and KHR cooperative-matrix routes.

SA0 is complete. The next permitted work is **SA1-P specification-only component-study preregistration**. No successor kernel or target-model run is authorized yet.


### SA1-P

SA1-P is now a **specification-only preregistration candidate**. The single frozen mechanism is subgroup-32 split-K per output row for batch-1 direct-packed Q4_K/Q6_K GEMM. Cooperative matrix/fusion/tile search are out of scope.

No SA1 kernel, harness or measurement is authorized until independent QA passes and an implementation lock is committed.


### SA1-P locked

SA1-P independent QA passed and the implementation lock is frozen. Only **SA1-K1 Q4** implementation is now permitted under a finite allowlist: one subgroup-32 split-K candidate shader plus the component harness/tooling required to qualify it.

Q6 implementation, component measurement, target-model inference and Q3 remain blocked.


### SA1-K1 implementation

The first successor implementation now exists under the SA1-P lock: one Q4_K subgroup-32 split-K shader plus an isolated synthetic component harness.

Measurement is still **not authorized**. The next gate is independent static audit followed by the exact-device correctness-only zero-measurement preflight.


SA1-K1 has passed an independent static-equivalent audit. Native compile/build and correctness-only preflight are still pending on the exact Arc 140V target; measured Q4 execution remains blocked.


### SA1-K1 preflight adjudicated

The Q4 subgroup-32 candidate passed exact-device compile/build and correctness-only preflight on the Arc 140V. Ten correctness cases passed the frozen numerical gates, with zero timestamp queries and zero measured pairs.

The implementation evidence is now locked. Q4 performance measurement is still pending a separate execution-authorization commit; Q6/model/Q3 remain blocked.
