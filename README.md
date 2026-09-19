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

P0-P7 are CLOSED. **P8-G is frozen FAIL. P8-G1 supports H-AMPLIFICATION. P8-G2 is frozen H-NONLINEAR/UNEXPLAINED. P8-G3 — arithmetic-precision attribution — is DESIGN_FROZEN / READY_FOR_IMPLEMENTATION.**

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
P8  7B memory-planned runtime                       ACTIVE (P8-G3 precision attribution)
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

P8-G remains frozen FAIL under its original 0.02 / 0.005 gate.

P8-G1 remains frozen **H-AMPLIFICATION**.

P8-G2 is COMPLETE and diagnostic-valid, but its preregistered A2 linear-closure and A4 alpha-linearity criteria failed; the frozen P8-G2 classification is therefore **H-NONLINEAR/UNEXPLAINED**.

Authoritative P8-G2 SHA256:
- shader provenance: `43A8DEA2AD14FF516B7FDF3EB69EC3D807C455F84D53E67C7BBF00A20C4993F4`
- amplification geometry: `2D183D80DD4A63CC75E10D2DC42BE08D7606B147A39FC15021E1D52E569CA168`
- summary: `B48FC0B48CC94363C24C0FD772058AC46C8BD3ED32203039F7FDEE230B0CB8A6`

P8-G2 reproduced its parent and confirmed that the production GPU kernel residual is negligible relative to propagated error: about 0.108% by max_abs and 0.167% by RMS. A2 closure residual was about 1.35% / 1.53% of propagated max/RMS error. A4 failed narrowly: only alpha=0.25 max_abs/alpha exceeded the frozen 1% tolerance.

Source audit shows the CPU Q4_K reference uses a sequential float accumulator over 18,944 terms. Therefore finite-precision non-distributivity is a prospective explanation for the P8-G2 A2/A4 failures.

P8-G3 is **DESIGN_FROZEN / READY_FOR_IMPLEMENTATION**. Contract: `docs/P8G3_CONTRACT.md`.

P8-G3 uses nested CPU arithmetic controls on the exact same observed vectors: frozen FP32 replay -> double accumulator with float product/output -> full double dot with float input algebra -> full double input algebra. It records the earliest regime that restores the unchanged P8-G2 closure and alpha-linearity criteria.

No replacement gate is defined. P8-H and full inference remain blocked.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
