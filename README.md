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

P0-P6 are CLOSED. P7 (Q4_K_M production path) is OPEN.

The strongest frozen prefill baseline remains P7-G: FFN row8 x token16 plus tiled attention projections. P7-I (token tile32) and P7-J (block-aware vec4 dequant) are genuine performance negatives: both preserve correctness but miss the pre-registered >=1.10x speedup gate.

The active experiment is **P7-K**: increase only FFN output-row tile 8 -> 16 while keeping token tile16, K32, workgroup 8x8, attention projections, attention kernel, LM head and decode unchanged.

## Roadmap

```text
P0  Bootstrap native runtime                         CLOSED
P1  GGUF tensor store / packed quantized weights    CLOSED
P2  Vulkan memory/runtime core                      CLOSED
P3  Kernel bring-up                                 CLOSED
P4  One decoder layer                               CLOSED
P5  Full decoder residency                          CLOSED
P6  GPU-resident KV + generation                    CLOSED
P7  Q4_K_M production path                          OPEN
P8  7B memory-planned runtime                       PLANNED
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

```powershell
git pull
git rev-parse HEAD
py -3 .\tests\test_p7k_package.py
powershell.exe -ExecutionPolicy Bypass -File .\run_p7k.ps1
```

Expected evidence:

```text
results\shader_provenance.json
results\p7k_ab_results.json
results\p7k_summary.json
```

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
