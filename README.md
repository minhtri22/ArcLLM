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

P0-P7 are CLOSED. **P8-D is frozen PASS. P8-E — bounded single-layer 7B graph correctness — is DESIGN_FROZEN / READY_FOR_IMPLEMENTATION.**

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
P8  7B memory-planned runtime                       ACTIVE (P8-E design)
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

P8-D is frozen as a genuine PASS.

Authoritative P8-D SHA256:
- shader provenance: `FABD3DAE027DB5AA69E037FE179BCD0C4F5A16C5D3AAFF0E08A938101AC8F454`
- graph binding results: `53C373BD3BA1A9BD31B45702CED08E2EB39C7F2B7B099E052EC8C5153A824ABD`
- summary: `74624BFAC44F4E5B9A6F08AC972508A353DAB96E0B6D4B92E8C658ABB1651D27`

P8-D resolved 339/339 graph tensors with zero missing or ambiguous bindings, retained exactly 19 arenas / 341 pieces / two segmented logical tensors, passed exhaustive span and global-coverage equivalence, and executed exactly two endpoint dispatches in one submit with decoder_layer_dispatches=0. Embedding remained exact; selected LM-head logits remained within ~2.38e-7 max_abs.

P8-E is now **DESIGN_FROZEN / READY_FOR_IMPLEMENTATION**, not READY_TO_RUN. Its contract is in `docs/P8E_CONTRACT.md`.

P8-E permits exactly one decoder layer (layer 0) at sequence length 4, with deterministic synthetic hidden input and full CPU-vs-GPU checkpoint comparison. Full 28-layer inference, decode and generation remain forbidden.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
