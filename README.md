# ArcLLM

ArcLLM is an experimental native Vulkan Compute runtime for GGUF LLM inference on Windows, currently validated on Qwen2.5-Coder-1.5B with Intel Arc 140V UMA.

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

P0-P7 are CLOSED. **P8-G is frozen FAIL. P8-G1 causally supports H-AMPLIFICATION. P8-G2 — amplification geometry qualification — is DESIGN_FROZEN / READY_FOR_IMPLEMENTATION.**

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
P8  7B memory-planned runtime                       ACTIVE (P8-G2 diagnosis)
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

P8-G remains frozen as a genuine FAIL under its original numerical gate.

P8-G1 is COMPLETE and diagnostic-valid. It reproduced the parent failure and classified the first obstruction as **H-AMPLIFICATION**.

Authoritative P8-G1 SHA256:
- shader provenance: `A4B095E1F2E7CD4AD78EE74C06A96323DF258C2ADB2CCA7DE827816191019740`
- causal decomposition: `416A97CD13A585BD4CAB397A3B3503BA8A87BD94F13BB1F5664F384603359499`
- summary: `1E04C9BF94DA57258D9DEA36FAE3F71B896D9F0CA773BF360305742397C27B1C`

The exact-weight kernel controls passed on both CPU and GPU SwiGLU inputs. P7-G tiled16 and P7-C tiled8 were bit-identical in both direct comparisons. CPU-only propagation of the observed upstream perturbation produced max_abs ~= 0.028336, reproducing the P8-G down-projection failure without a GPU-kernel defect.

Observed directional amplification from the existing evidence is approximately 3.378x in max_abs and 5.135x in RMSE.

P8-G2 is **DESIGN_FROZEN / READY_FOR_IMPLEMENTATION**. Contract: `docs/P8G2_CONTRACT.md`.

P8-G2 quantifies linear propagation closure, directional gain, kernel-residual contribution, normalized signal/error scales, and a frozen α-scaling series. It does not change the historical P8-G gate or define a replacement production gate.

P8-H and full 28-layer inference remain blocked.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
