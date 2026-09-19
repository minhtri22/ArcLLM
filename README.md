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

P0-P7 are CLOSED. **P8-F is frozen PASS. P8-G — bounded four-layer prefix correctness — is READY TO RUN.**

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
P8  7B memory-planned runtime                       ACTIVE (P8-G)
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

P8-F is frozen as a genuine PASS.

Authoritative P8-F SHA256:
- shader provenance: `9686F747B6A4D7380F4621B1A3EEC09B82DE7832461B1A9C80548D21D7D70A41`
- two-layer results: `F172E1B0B00558BDE53EB6994BDC1E5BFFC417F96A33FA1C53B0983BB494AE98`
- summary: `1F3EF56BC723FE2D8D8E567ADCB89BC23AE212CB0BBBB51CBC5B83E81DF9FC70`

P8-F executed exactly layers 0 -> 1 at seq=4 with direct GPU hidden-state handoff. All 34 CPU-vs-GPU checkpoints passed. Worst max_abs was L1.ffn_gate ~= 0.003254; worst RMSE remained L0.k_rope ~= 0.000176, both inside the frozen 0.02 / 0.005 gates. Execution was exactly 30 dispatches, one submit and two decoder layers. Full inference remained forbidden.

P8-G implementation is committed and **READY TO RUN**. Its frozen contract is in `docs/P8G_CONTRACT.md`.

Run:

```powershell
py -3 .\tests\test_p8g_package.py
powershell.exe -ExecutionPolicy Bypass -File .\run_p8g.ps1
```

Expected evidence:

```text
results\p8g_shader_provenance.json
results\p8g_four_layer_results.json
results\p8g_summary.json
```

P8-G changes only prefix depth: exactly layers [0,1,2,3], same seq=4/input/kernels/gates, direct GPU handoff across all three boundaries, independent four-layer CPU reference, 68 checkpoints, and exact 60 dispatches in one submit.

Full 28-layer inference, decode and generation remain forbidden.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
