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

P0-P7 are CLOSED. **P8-G is frozen FAIL. P8-G1=H-AMPLIFICATION. P8-G2=H-NONLINEAR/UNEXPLAINED. P8-G3=H-FP32-ACCUMULATION. P8-G4 fresh-cohort compositional decomposition is DESIGN_FROZEN / READY_FOR_IMPLEMENTATION.**

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
P8  7B memory-planned runtime                       ACTIVE (P8-G4 fresh-cohort decomposition)
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
- P8-G2: **H-NONLINEAR/UNEXPLAINED** under its preregistered FP32 closure/linearity criteria
- P8-G3: **H-FP32-ACCUMULATION**

Authoritative P8-G3 SHA256:
- shader provenance: `C10D0DB9444E584CDC76D939F5D134EC1229BD600CF3EED181C9D08505E955E8`
- arithmetic-precision result: `B0ADAAF9790018467C721599C2147AF7A6C0F69F0967B5C25F77631095571835`
- summary: `EF494284E7BFBED541380267EDB197CF53F9EBB7E86040A83546735F84209C4D`

P8-G3 reproduced R0 exactly. Both closure and alpha-linearity first pass at R1: float product + double accumulator + float output. R1 closure is max_abs ~= 1.326e-5 / RMSE ~= 2.769e-7; R2/R3 reduce closure to about 3.84e-12 / 4.9e-14. Therefore sequential FP32 accumulation is sufficient to explain the P8-G2 A2/A4 misses.

This result does not erase P8-G's historical FAIL and does not by itself justify a replacement correctness gate.

P8-G4 is **DESIGN_FROZEN / READY_FOR_IMPLEMENTATION**. Contract: `docs/P8G4_CONTRACT.md`.

P8-G4 prospectively tests four fresh deterministic inputs, still only through L3, and decomposes total down-projection error into:
- propagated upstream state drift;
- same-input local GPU operator error.

The P8-G3-qualified R1 arithmetic is used only as a diagnostic CPU oracle. The mechanistic replication gate requires local operator error to be <=5% of state-drift error on both max and RMS for all four fresh inputs, with exact decomposition closure.

No replacement gate is defined. P8-H and full inference remain blocked.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
