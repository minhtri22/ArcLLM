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

P8-G remains frozen FAIL under its original 0.02 / 0.005 gate.

Frozen causal chain:
- P8-G1: **H-AMPLIFICATION**
- P8-G2: **H-NONLINEAR/UNEXPLAINED**
- P8-G3: **H-FP32-ACCUMULATION**
- P8-G4: **H-LOCAL-ERROR-NONNEGLIGIBLE**
- P8-G5: **H-R1-ORACLE-SEMANTIC-MISMATCH**

Authoritative P8-G5 SHA256:
- shader provenance: `6483D82540EC31F3CE058B51FC48C9BABFED9983F7F59A090D9AE14917176F9A`
- production-semantic attribution: `64565C94AD2D9EF85D1DFF8CE972FD263C2C1B4A28A28B5F6830F6EB9AD6E096`
- summary: `1732663E0A9CF90848D54487E2C2D031EB1A5C2A90808A34ABB1C06D57995751`

P8-G5 is COMPLETE and diagnostic-valid. All P0/P1/P2/P3 closure/dominance checks pass, and classification is H-R1-ORACLE-SEMANTIC-MISMATCH. Production-matched R0 local residuals remain inside 1e-4 / 1e-6 on all four retrospective seeds, while the R0->R1 oracle shift accounts for most of the P8-G4 local term. Under R0 semantics the local/state ratios fall well below 5% for all four seeds.

P8-G5 is retrospective attribution only and does not establish generalization.

P8-G6 implementation + static QA are locked and **READY TO RUN**. Contract: `docs/P8G6_CONTRACT.md`.

Run only after pulling the implementation-lock commit:

```powershell
py -3 .\tests\test_p8g6_package.py
powershell.exe -ExecutionPolicy Bypass -File .\run_p8g6.ps1
```

Expected evidence:

```text
results\p8g6_shader_provenance.json
results\p8g6_fresh_production_semantic_results.json
results\p8g6_summary.json
```

The executable runs only fresh IDs {73,89,107,131}. R0 is the primary comparator for C1/C2/C3. R1 is emitted as descriptive-only diagnostics and is statically excluded from those decision metrics and from classification, except for its preregistered finite-output check in C0.

Frozen gates remain unchanged:
- local GPU vs R0: max_abs <=1e-4 / RMSE <=1e-6;
- R0 compositional closure: max_abs <=1e-5 / RMSE <=1e-7;
- local/state dominance: <=5% on both max and RMS.

No fresh target experiment was run in the implementation-lock commit. No replacement correctness gate is defined. P8-H and full inference remain blocked.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
