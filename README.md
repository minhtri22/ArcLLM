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

P0-P7 are CLOSED. **P8-E is frozen PASS. P8-F — bounded two-layer prefix correctness — is READY TO RUN.**

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
P8  7B memory-planned runtime                       ACTIVE (P8-F)
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

P8-E is frozen as a genuine PASS.

Authoritative P8-E SHA256:
- shader provenance: `73916DE149A413B835541B95F01FEAF2B87DDDE03C039C3D1ABC0E2A3A115861`
- single-layer results: `992A986081FAFC81AC2E6E1A38063384434DE3138CAE469A04A57FED57BDA52B`
- summary: `EBC4C8088B0992A30D72973DC7485CCF7AA0618B354509A506CC9FDE294B5B9D`

P8-E executed exactly layer 0 at seq=4 through the frozen 19-arena / 341-piece graph-binding plan. All 17 CPU-vs-GPU checkpoints passed. The largest observed error was at K RoPE/K-cache with max_abs ~= 0.00277 and RMSE ~= 0.000176, still well inside the frozen 0.02 / 0.005 gates. Final layer output max_abs ~= 0.000297 and RMSE ~= 1.75e-5. Execution was exactly 15 dispatches, one submit and one decoder layer. Full inference remained forbidden.

P8-F implementation is committed and **READY TO RUN**. Its frozen contract is in `docs/P8F_CONTRACT.md`.

Run:

```powershell
py -3 .\tests\test_p8f_package.py
powershell.exe -ExecutionPolicy Bypass -File .\run_p8f.ps1
```

Expected evidence:

```text
results\p8f_shader_provenance.json
results\p8f_two_layer_results.json
results\p8f_summary.json
```

P8-F changes only one scientific variable: layer composition. It executes exactly prefix layers 0 -> 1 at seq=4 with direct GPU hidden-state handoff, independent two-layer CPU reference, 34 checkpoint records and exact 30 dispatches in one submit.

Full 28-layer inference, decode and generation remain forbidden.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
