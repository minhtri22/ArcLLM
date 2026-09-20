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

P0-P7 are CLOSED. **P8-G is frozen FAIL. P8-G1=H-AMPLIFICATION. P8-G2=H-NONLINEAR/UNEXPLAINED. P8-G3=H-FP32-ACCUMULATION. P8-G4=H-LOCAL-ERROR-NONNEGLIGIBLE. P8-G5 production-semantic local-error attribution is READY TO RUN.**

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
P8  7B memory-planned runtime                       ACTIVE (P8-G5 local-oracle attribution)
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

Frozen causal chain:
- P8-G1: **H-AMPLIFICATION**
- P8-G2: **H-NONLINEAR/UNEXPLAINED**
- P8-G3: **H-FP32-ACCUMULATION**
- P8-G4: **H-LOCAL-ERROR-NONNEGLIGIBLE**

Authoritative P8-G4 SHA256:
- shader provenance: `1C146FCDD60D14A782512687865AC403D6DEE5C72FC125F8A74C5D4A920D5C7C`
- fresh compositional result: `9255B70BA50DF316B7D8BA04CB6696DA9FB2CB97D82F7A559DCF166F7E76E757`
- summary: `6347545710333A3CF9856399DF040E57A6C140E3463C1E50E5A8A432AF82FFD0`

P8-G4 is structurally valid: all four D0 closures pass and all four same-input D1 historical local gates pass. However all four preregistered 5% D2 tests fail, so the frozen cohort classification is H-LOCAL-ERROR-NONNEGLIGIBLE.

The production P7-G shader source uses float products and sequential float accumulation, matching R0 arithmetic rather than the R1 double-accumulator diagnostic oracle used by P8-G4. Therefore P8-G4's E_local_R1 mixes actual production-kernel residual with the R0->R1 arithmetic-oracle shift.

P8-G5 implementation + static QA are locked and **READY TO RUN**. Contract: `docs/P8G5_CONTRACT.md`.

Run only after pulling the implementation-lock commit:

```powershell
py -3 .\tests\test_p8g5_package.py
powershell.exe -ExecutionPolicy Bypass -File .\run_p8g5.ps1
```

Expected evidence:

```text
results\p8g5_shader_provenance.json
results\p8g5_production_semantic_attribution_results.json
results\p8g5_summary.json
```

P8-G5 replays exactly the frozen P8-G4 cohort and computes R0/R1 CPU oracles together from the same decoded Q4_K weights. The GPU graph is unchanged: per seed 58-dispatch prefix + one production local-down dispatch.

P0 must reproduce the P8-G4 R1 state/local metrics and all four D2 failures before any attribution is accepted. P1/P3 retain exact algebraic closure gates; P2 uses the preregistered 1e-4/1e-6 production-matched equivalence gate; P3 keeps the original 5% threshold unchanged.

No P8-G5 target attribution experiment was run in the implementation-lock commit. No replacement gate is defined. P8-H and full inference remain blocked.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
