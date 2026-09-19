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

The strongest frozen prefill baseline is now P7-L: tiled attention projections + P7-G FFN-down tile16 + fused Q4_K gate+up. P7-I (token tile32), P7-J (block-aware vec4 dequant), and P7-K (row tile16) are frozen performance negatives; P7-L is a frozen PASS at 1.24293x.

The active experiment is **P7-N**: starting from the exact P7-L winner, fuse SwiGLU into the Q4_K gate+up kernel so it writes final `s` directly. P7-M is PASS/frozen and shows prefill remains dominated by fused gate/up (44.40%) followed by FFN down (30.42%), while barrier/unattributed time is only ~0.024%.

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
py -3 .\tests\test_p7n_package.py
powershell.exe -ExecutionPolicy Bypass -File .\run_p7n.ps1
```

Expected evidence:

```text
results\shader_provenance.json
results\p7n_ab_results.json
results\p7n_summary.json
```

Historical run archives are kept outside the active Git history unless their original byte-exact artifacts are available.
