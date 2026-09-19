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

P0-P7 are CLOSED. **P8-G is frozen FAIL at L3.ffn_down. P8-G1 — L3 FFN-down causal decomposition — is DESIGN_FROZEN / READY_FOR_IMPLEMENTATION.**

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
P8  7B memory-planned runtime                       ACTIVE (P8-G1 diagnosis)
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

P8-G is frozen as a genuine FAIL under the original numerical gate.

Authoritative P8-G SHA256:
- shader provenance: `C258C76816BDE57CC9D56A3C73955019716A1F01637A11D23197129CC53EA1B9`
- four-layer results: `BDB7F4C1480CF960A89EBB29FACE58FC751BA8EF6AF76812F23A7C5ED0E94129`
- summary: `0B1D4CF0355FBB7987217CC1DA8133BD81CA5332B37D5542E69A427A7C196BFE`

P8-G correctly executed layers [0,1,2,3] with 60 dispatches / one submit and direct GPU handoff. L0-L2 fully passed. L3 passed through SwiGLU, then first failed at `L3.ffn_down`: max_abs ~= 0.028328 exceeded the frozen 0.02 gate while RMSE ~= 0.00036484 remained below 0.005. L3 ffn_down is Q4_K whereas L0-L2 ffn_down are Q6_K.

P8-G1 is **DESIGN_FROZEN / READY_FOR_IMPLEMENTATION**. Contract: `docs/P8G1_CONTRACT.md`.

P8-G1 does not rerun or relax P8-G. It causally decomposes the L3 failure using CPU-vs-GPU input interventions and an exact-weight A/B between P7-G Q4_K tiled16 and P7-C Q4_K tiled8.

P8-H and full 28-layer inference remain blocked.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
