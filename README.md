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

P0-P7 are CLOSED. **P8-A — frozen 7B memory-plan-only bring-up** is READY TO RUN.

The frozen P7 production winner is **P7-L**: tiled attention projections + P7-G FFN-down tile16 + fused Q4_K gate+up. P7-I, P7-J, P7-K, P7-N and P7-O are preserved performance negatives. See `docs/P7_CLOSEOUT.md`.

The P8 target is now frozen to official `Qwen/Qwen2.5-Coder-7B-Instruct-GGUF` Q4_K_M at revision `13fb94bfda8c8cf22497dc57b78f391a9acb426a`, size 4,683,073,536 bytes, SHA256 `509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C`. P8-A performs memory planning only; full 7B inference is forbidden until it passes.

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
P8  7B memory-planned runtime                       ACTIVE (P8-A)
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

First download/verify the frozen 7B target:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\tools\fetch_p8_target.ps1
```

Then execute the memory-plan-only gate:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\run_p8a.ps1
```

Expected evidence:

```text
results\p8a_memory_plan.json
results\p8a_summary.json
```

Do not run full 7B inference before P8-A PASS.

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
