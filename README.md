# ArcLLM

ArcLLM is an experimental native Vulkan Compute runtime for running GGUF large-language models on Windows, with the current research target focused on **Qwen2.5-Coder-1.5B** on **Intel Arc 140V / UMA** hardware.

The project is built around a simple architectural thesis: useful local inference on this class of hardware requires **whole-decoder GPU residency**, direct execution over **packed quantized weights**, a **GPU-resident KV cache**, and explicit memory planning rather than repeated CPU↔GPU transfers.

ArcLLM is developed as a research-engineering project rather than as a benchmark-only prototype. Every phase has a frozen gate, authoritative JSON evidence, SHA/provenance tracking, and an append-only `lineage.md` so implementation failures, environment failures, scientific negatives, and genuine PASS results remain distinguishable.

## Current status

The active checkpoint is **P7-I**.

Completed phases:

| Phase | Status | What was established |
| --- | --- | --- |
| P0 | CLOSED | Native Windows bootstrap, frozen target model/SHA, GGUF inspection, SDK-less Vulkan loading, memory capability envelope |
| P1 | CLOSED | Complete GGUF tensor store and direct packed Q4_K / Q6_K access |
| P2 | CLOSED | Persistent Vulkan weight arenas, scratch memory, queue/command infrastructure |
| P3 | CLOSED | Vulkan correctness kernels for RMSNorm, packed quantized GEMM, RoPE, attention, softmax, SwiGLU/residual |
| P4 | CLOSED | One complete Qwen2 decoder layer with resident intermediates and no host round-trip inside the layer |
| P5 | CLOSED | Full 28-layer decoder residency with tied LM head and final CPU-reference agreement |
| P6 | CLOSED | Persistent GPU KV cache and autoregressive generation across submissions |
| P7 | OPEN | Production Q4_K_M path, profiling, and targeted performance optimization |

The latest executed optimization checkpoint is **P7-G PASS**. On the frozen pp512 fixture, performance improved from the original P7-A baseline of about **7.10 tok/s** to about **96.28 tok/s** after tiled packed-GEMM work, while preserving the frozen correctness gates. These numbers are development measurements for the current fixture, not product guarantees.

P7-H is **PASS / frozen**. On the P7-G graph, prefill remained FFN-dominant: gate/up 49.57% plus down 27.39% = about **76.96%** of GPU chain time, while barrier/unattributed time was about 0.025%. P7-I therefore tests exactly one next hypothesis: increase only prefill FFN token reuse from tile16 to tile32; attention projections, attention kernel, LM head, and decode remain frozen.

## Architecture

Current locked architecture:

- Native C++17 runtime on Windows.
- Vulkan Compute through dynamic runtime loading; no Vulkan SDK installation is required for normal execution.
- Pinned Khronos `glslang` downloaded and SHA-verified by the build scripts for GLSL → SPIR-V compilation.
- Whole-model / whole-decoder weight residency.
- Direct packed **Q4_K** and **Q6_K** shader access; full-model pre-expansion to FP16/F32 is not used.
- Persistent sharded weight arenas, currently constrained to <=256 MiB per arena.
- GPU-resident KV cache for generation.
- CPU boundary limited to orchestration/token sampling where the current phase explicitly allows it.
- Correctness-first development: performance tuning is accepted only after the corresponding numerical gates pass.

The current frozen model is:

```text
Qwen2.5-Coder-1.5B
SHA256: 6A77366395772462C84F0C4D226AC404674327CBE78C01E4391CC7E0C698851E
```

The model file itself is **not** stored in this repository.

## Repository layout

```text
config/      Frozen target configuration
docs/        Phase contracts and implementation notes
inputs/      Authoritative evidence inherited from completed phases
shaders/     Vulkan compute shaders
src/         Native C++ runtime / profiler sources
tests/       Static package and contract checks
tools/       Windows build and shader-compilation scripts
lineage.md   Append-only research/engineering history
manifest.json Current checkpoint contract
run_p7i.ps1  Current Windows entry point
```

Generated binaries, SPIR-V files, downloaded toolchains, and local `results/` output are intentionally excluded from Git.

## Current run

On the frozen Windows target machine:

```powershell
cd ArcLLM
powershell.exe -ExecutionPolicy Bypass -File .\run_p7i.ps1
```

Expected output for the current checkpoint:

```text
results\shader_provenance.json
results\p7i_ab_results.json
results\p7i_summary.json
```

P7-I uses three interleaved pp512 A/B trials with the performance gate frozen at median wall speedup >=1.10x. A build, package, environment, provenance, or shader-compile problem is not a scientific/performance negative.

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
P10 Activation/output-aware Q4 research              DEFERRED UNTIL PRODUCTION BASELINE
```

A usable local runtime does **not** depend on P10. The production path is P7 → P8 → P9; P10 is a later research track.

## Development workflow

From this checkpoint forward, changes follow one controlled loop:

```text
PLAN → CODE → TEST → QA → COMMIT → PULL/RUN ON TARGET → JSON EVIDENCE → REVIEW
```

Rules:

1. Plan one bounded change at a time; do not optimize multiple hypotheses in parallel.
2. Preserve frozen correctness/performance gates; never lower a gate after seeing a result.
3. Run static/unit/package tests before commit.
4. QA verifies script-referenced paths, SHA/provenance, line endings, generated-file exclusions, and phase-contract invariants.
5. Commit only after QA passes.
6. The target machine pulls the commit, runs the documented command, and returns JSON evidence.
7. `lineage.md` is append-only. Every completed iteration records **Evidence / Decision / Next**.

## Provenance and evidence

The active tree carries the authoritative evidence required by the current checkpoint (currently the frozen P7-H profile, summary, shader provenance, plus the earlier P7-G baseline evidence) together with the complete append-only `lineage.md`. Historical source packages that are not available byte-for-byte are **not reconstructed as fake Git history**. Older run archives remain external backup artifacts and can be imported later as immutable archives with their original SHA values.

## Hardware assumptions

The current development machine uses Intel Arc 140V integrated graphics with unified memory. ArcLLM therefore treats Vulkan device-local host-visible coherent memory as UMA and does **not** describe the machine as having 16 GB of dedicated VRAM.

Performance and memory behavior on other Vulkan devices may differ and should be revalidated rather than assumed.
