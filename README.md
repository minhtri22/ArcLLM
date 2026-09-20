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

ArcLLM is now on the governance main path: **Q1 real-model end-to-end feasibility**.

P8-G6 is frozen **H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED** on a fresh 4-seed cohort. All C0/C1/C2/C3 gates pass 4/4. This closes the bounded L3 FFN-down production-semantic blocker; it does not itself establish whole-model inference.

Authoritative P8-G6 SHA256:
- shader provenance: `1490475D0D7EAA0498FEEA5CD0A37460C4881FFFF676A7C912E0E113E2CAAC84`
- fresh production-semantic result: `0526D3F1400080AF6B68D1D44CBD2AF897671B727EA924EC0D0C953C16FDD259`
- summary: `13C36E5EB14D08F60C3DC9277F7A21EE4033DB50D84806839DE62B3E9F7EE303`

Frozen causal history remains unchanged:
- P8-G: **FAIL**
- P8-G1: **H-AMPLIFICATION**
- P8-G2: **H-NONLINEAR/UNEXPLAINED**
- P8-G3: **H-FP32-ACCUMULATION**
- P8-G4: **H-LOCAL-ERROR-NONNEGLIGIBLE**
- P8-G5: **H-R1-ORACLE-SEMANTIC-MISMATCH**
- P8-G6: **H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED**

The project-level convergence governance supersedes the pre-governance suggestion to open another subsystem correctness-metric study. The next main-path question is Q1.

Q1 contract: `docs/P8_Q1_END_TO_END_CONTRACT.md`.

Q1 freezes a real full-model compute run:
- exact frozen 7B target;
- pretokenized input IDs `[1,133151,133152,152062]`;
- segmented embedding;
- all 28 decoder layers;
- GPU-resident KV;
- final norm;
- full 152064-logit segmented LM head;
- greedy argmax;
- one token from prefill logits plus four cached-decode steps = exactly five generated token IDs;
- two independent reset executions A/B;
- inherited production graph census 441 prefill dispatches / 469 dispatches per decode step.

Tokenizer/API and performance comparison are intentionally outside Q1.

Status: **Q1 IMPLEMENTATION_LOCKED / EXACT PARENT-EVIDENCE BLOB RESTORED / BUILDONLY PENDING**. The first local Q1 invocation stopped fail-closed before target execution because the committed P8-G6 result raw was not byte-identical to the original uploaded artifact. The exact original bytes are being restored and static QA now verifies all three parent SHA256 values. Q2/Q3 remain closed.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
