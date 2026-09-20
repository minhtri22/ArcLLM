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

Status: **Q2 IMPLEMENTATION_STATIC_LOCKED / LOCAL PREFLIGHT REQUIRED**. GitHub Q2 CI is unavailable before job steps, so target-local zero-measurement preflight is now the authoritative build/runtime qualification gate. The 20 measured attempts remain code-blocked until `q2_preflight_lock.json` is returned, adjudicated and a final execution lock is committed.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
