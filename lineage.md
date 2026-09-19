# ArcLLM — lineage.md

> **Rule:** the roadmap block below is pinned. After it, this file is **append-only**.
> Each entry must be short: date / phase / evidence / decision / next. Never rewrite history.

## Pinned execution steps

```text
P0  Bootstrap native runtime
    Windows build + GGUF inspect + target lock + Vulkan loader probe + memory plan
    Gate: native EXE builds and emits valid evidence for frozen Qwen2.5-Coder-1.5B.

P1  GGUF tensor store
    Parse tensor directory/layout completely; map packed Q4_K_M without expansion.
    Gate: exact tensor inventory + direct packed weight access.

P2  Vulkan memory/runtime core
    Device/queue + sharded weight arenas + scratch + command/pipeline infrastructure.
    Gate: persistent Vulkan allocations; no >1 GiB storage-buffer assumption.

P3  Kernel bring-up
    RMSNorm → Q4_K matvec/GEMM → RoPE → attention/softmax → SwiGLU/residual.
    Gate: per-op numerical agreement against CPU reference.

P4  One decoder layer
    Entire Qwen2 layer on Vulkan with resident intermediates.
    Gate: layer output agreement; no host round-trip inside layer.

P5  Full decoder residency
    embeddings → all layers → final norm → lm_head; weights/scratch resident.
    Gate: full forward agreement on 1.5B.

P6  GPU-resident KV + autoregressive generation
    CPU tokenizer/sampler only at token boundary.
    Gate: continuous generation on Arc 140V with resident KV.

P7  Q4_K_M production path
    Direct packed Q4_K_M kernels + prefill/decode specialization + reuse/pipeline cache.
    Gate: stable 1.5B runtime and benchmark against frozen llama.cpp R8-VK reference.

P8  7B memory-planned runtime
    Context-aware residency; constrained-resident mode only if needed.
    Gate: Qwen2.5-Coder-7B runs without violating memory planner invariants.

P9  Local API
    CLI + OpenAI-compatible local server.
    Gate: /v1/chat/completions end-to-end.

P10 Quantization research
    Activation/output-aware Q4 only after Q4_K_M production baseline is stable.
```

---

## 2026-09-18 — R8-VK close
Evidence: strict Vulkan offload 29/29 layers on Qwen2.5-Coder-1.5B; pp512 5.11x CPU, tg128 1.84x CPU; R8-VK = STRONG_SUPPORT.
Decision: custom runtime primary architecture = Vulkan Compute whole-decoder residency. R8/R8-VK remain closed; no tuning there.
Next: P0 native bootstrap.

## 2026-09-18 — P0 implementation
Evidence target: frozen local Ollama blob Qwen2.5-Coder-1.5B, SHA256 `6A77366395772462C84F0C4D226AC404674327CBE78C01E4391CC7E0C698851E`.
Implementation: native C++17 CLI, GGUF v2/v3 metadata+tensors inspector, SDK-less Vulkan loader/heap probe, context-aware resident-memory planner, reproducible PowerShell build/run.
Decision: P0 code prepared; execution evidence pending on target Windows machine.
Next: run `run_p0.ps1`; if gate passes, append P0 PASS and open P1.


## 2026-09-18 — P0 first execution
Evidence: native build/SHA/GGUF/Vulkan PASS; qwen2, 338 tensors, 18.005 GiB heap; ctx4096 envelope 2.120 GiB. Gate false only because current free RAM was already below the 8 GiB launch reserve.
Decision: capability gate uses frozen A1 validated resident floor 3.75 GiB; current-state reserve remains advisory.
Next: run P0-R1 once; PASS opens P1.


## 2026-09-18 — P0 PASS
Evidence: exact target SHA; native build; GGUF v3/qwen2/338 tensors; Vulkan loader+device; required 2.120 GiB <= frozen A1 resident floor 3.75 GiB. Current-state launch safety remained advisory.
Decision: P0 CLOSED.
Next: P1 direct packed GGUF tensor store.

## 2026-09-18 — P1 implementation
Evidence target: read-only memory-mapped GGUF payload; exact tensor spans/types; direct Q4_K access without expansion; payload bounds/overlap validation.
Decision: P1 code prepared; no model math and no benchmark.
Next: run `run_p1.ps1`; PASS opens P2.


## 2026-09-18 — P1 wrapper repair
Evidence: first P1 invocation stopped in PowerShell parsing before build/execution; no P1 measurement occurred.
Decision: rewrite P1 entry script as ASCII-only CRLF for Windows PowerShell 5.1 compatibility; P1 contract unchanged.
Next: run P1-R1 once.


## 2026-09-18 — P1 build repair
Evidence: P1 wrapper passed and native build started; compile stopped in `tensor_store.cpp` because Windows `max` macro collided with `std::numeric_limits<uint64_t>::max()`. No P1 execution occurred.
Decision: define `NOMINMAX` before `windows.h` and use macro-safe `(std::numeric_limits<uint64_t>::max)()`. P1 contract unchanged.
Next: run P1-R2 once.


## 2026-09-18 — P1 PASS
Evidence: read-only zero-copy GGUF mapping; 338 tensors; F32=141, Q4_K=168, Q6_K=29; all bounds valid; no overlap; direct packed Q4_K access PASS.
Decision: P1 CLOSED.
Next: P2 Vulkan memory/runtime core.

## 2026-09-18 — P2 implementation
Evidence target: one logical Vulkan device + compute queue; full packed GGUF payload split into <=256 MiB persistent weight arenas; persistent scratch arena; command pool/buffer/fence; GPU fill command with host-visible verification.
Decision: P2 code prepared; no shader/model math and no benchmark.
Next: run `run_p2.ps1`; PASS opens P3.


## 2026-09-18 — P2 PASS
Evidence: 1 Vulkan device; compute queue acquired; full 980,097,536-byte packed GGUF payload resident in 4 persistent arenas; all arena fingerprints verified; 64 MiB scratch persistent; command submit/fence PASS; GPU scratch fill verified; 5 buffers alive during command.
Decision: P2 CLOSED.
Next: P3 kernel bring-up.


## 2026-09-18 — P3 implementation
Evidence target: real Vulkan compute shaders with CPU numerical references for RMSNorm, direct packed Q4_K matvec/GEMM, normal RoPE, softmax, bounded GQA attention, and SwiGLU+residual.
Decision: use pinned glslang 16.5.0 compiler acquisition; no Vulkan SDK install; Q4_K kernel consumes actual `blk.0.attn_q.weight` packed bytes from the frozen GGUF.
Next: run `run_p3.ps1`; PASS closes P3 and opens P4 one decoder layer.


## 2026-09-18 — P3 packaging repairs
Evidence: first P3 invocation stopped before build because `config/p0_target.json` was missing. P3-R2 then acquired pinned glslang and compiled all six shaders PASS, but native build stopped because `gguf.*` and `tensor_store.*` were absent. No kernel execution occurred in either attempt.
Decision: P3-R3 is self-contained: frozen target config plus P2-validated GGUF/tensor-store sources are bundled; compiler discovery accepts official `glslang.exe` or `glslangValidator.exe`. Numerical contract unchanged.
Next: run P3-R3 once.


## 2026-09-18 — P3 PASS
Evidence: pinned glslang 16.5.0 provenance verified; all seven Vulkan kernel gates PASS against independent CPU references. Real `blk.0.attn_norm.weight` used for RMSNorm; real packed `blk.0.attn_q.weight` consumed directly by the Q4_K shader with no pre-expansion.
Decision: P3 CLOSED.
Next: P4 one complete Qwen2 decoder layer with resident intermediates and no host round-trip inside the layer.

## 2026-09-18 — P4 implementation
Evidence target: complete real `blk.0` graph on one Vulkan command buffer/one submit; packed Q4_K and Q6_K real weights; resident intermediates; zero intermediate host read/write; final output compared to an independent CPU reference at frozen max_abs <= 2e-2 and RMSE <= 5e-3.
Decision: P4 code prepared; correctness only, no performance tuning. Q6_K support is added because the frozen layer's real V and FFN-down tensors are Q6_K.
Next: run `run_p4.ps1`; only a genuine execution PASS closes P4 and opens P5 full-decoder residency.

## 2026-09-18 — P4-R1 package-audit repair
Evidence: P4 first invocation blocked before execution in `tests/test_p4_package.py`; Python `Path.read_text()` used the Windows locale default while `lineage.md` is UTF-8, so the em-dash heading did not compare byte-for-character as intended. No shader compile, native build, or Vulkan layer execution occurred.
Decision: classify as harness/package audit failure only. P4 scientific contract, graph, kernels, weights, numerical gates, and execution boundary are unchanged. P4-R1 makes text encodings explicit and uses stable ASCII semantic markers for lineage assertions.
Next: run P4-R1 once; only a genuine execution PASS closes P4 and opens P5.


## 2026-09-18 — P4 PASS
Evidence: complete real `blk.0` Qwen2 layer executed as 15 Vulkan dispatches in one command buffer/one submit/one fence wait; real frozen layer weights; direct packed Q4_K and Q6_K; zero intermediate host reads/writes; final output max_abs=0.0005810260773 and RMSE=3.027076833e-05, both within frozen P4 gates.
Decision: P4 CLOSED.
Next: P5 full decoder residency.

## 2026-09-18 — P5 implementation
Evidence target: GPU token embedding from tied packed Q6_K `token_embd.weight`; all 28 real decoder layers with authoritative mixed Q4_K/Q6_K V/down tensors; final RMSNorm; tied packed-Q6_K LM head. All 338 GGUF tensors are kept resident in four tensor-aware <=256 MiB packed weight arenas. One Vulkan command buffer/one submit/one fence wait; no host intervention until post-completion validation. Sequence length is frozen to 1 because P4 already validated causal GQA at seq=4; P5 isolates full-depth residency/correctness. LM-head rows are chunked into bounded dispatches without host round-trip.
Decision: P5 code prepared; correctness only, no throughput tuning and no KV-cache generation claim. Frozen gates: final normalized hidden max_abs<=0.03/RMSE<=0.005; logits max_abs<=0.10/RMSE<=0.01; all finite.
Next: run `run_p5.ps1`; only a genuine full-decoder execution PASS closes P5 and opens P6 GPU-resident KV + generation.


## 2026-09-18 — P5 PASS
Evidence: full frozen Qwen2.5-Coder-1.5B decoder executed on Vulkan with all 338 tensors / 980,097,536 packed bytes resident in four <=256 MiB arenas; 28 layers + output norm + tied LM head, 441 dispatches in one command buffer/one submit/one fence wait, no intermediate host reads/writes. Final norm max_abs=0.0002012252808, RMSE=7.641232208e-06. Logits max_abs=3.051757812e-05, RMSE=5.368667236e-06; CPU/GPU top1 both 117612.
Decision: P5 CLOSED.
Next: P6 GPU-resident KV + generation.

## 2026-09-18 — P6 implementation
Evidence target: batched 4-token prefill followed by one autoregressive decode step producing two greedy output tokens total. All model weights remain resident exactly as P5. Per-layer K/V are written into persistent Vulkan buffers during prefill and consumed directly by the decode attention path; KV is never read or written by the host during generation. CPU interaction is restricted to the frozen orchestration boundary: read logits, greedy argmax, write the selected token id for the next decode. Independent CPU reference maintains a separate KV cache.
Decision: P6 code prepared; correctness only. Frozen prompt token ids=[1,17,42,256], RoPE base position=17, max_ctx=16. Gates: both logit checkpoints max_abs<=0.10/RMSE<=0.01 with exact CPU/GPU greedy token agreement; final used K and V cache max_abs<=0.02/RMSE<=0.005; all finite; exact residency/execution invariants.
Next: run `run_p6.ps1`; only a genuine P6 execution PASS closes P6 and opens P7 Q4_K_M production path.


## 2026-09-18 — P6 PASS
Evidence: batched 4-token Vulkan prefill wrote persistent GPU K/V across all 28 layers; one-token autoregressive decode consumed the same cache across a second submission. Weights remained all-resident (338 tensors / 980,097,536 packed bytes), no KV host reads/writes occurred during generation, and CPU/GPU greedy tokens matched exactly [6228,17]. Prefill logits max_abs=0.001761436462, RMSE=0.0003688782; decode logits max_abs=0.001020431519, RMSE=0.0002062647367. Final used K cache max_abs=0.01123046875/RMSE=0.000235098343 and V cache max_abs=0.0022777915/RMSE=0.000155599446; all frozen gates PASS.
Decision: P6 CLOSED.
Next: P7 Q4_K_M production path.

## 2026-09-18 — P7-A production baseline implementation
Evidence target: scale the proven P6 path to pp512 + tg128 without changing model quantization or residency architecture. P7-A introduces scalable online-softmax attention (no fixed 16-token shared array), 2-D packed Q4_K/Q6_K GEMM dispatch for batch 512, and prepared/cached Vulkan pipelines+descriptor sets reused across decode submissions. All 338 frozen tensors remain packed and resident; KV remains GPU-resident through a 640-position benchmark context.
Decision: P7-A is a measurement subphase, not P7 closure. Correctness regressions for every newly changed kernel are frozen gates; pp512/tg128 performance is measured against the frozen R8-VK references (~677.87 pp512 tok/s, ~40.35 tg128 tok/s) but those references are not PASS thresholds. No performance verdict may change correctness gates after measurement.
Next: run P7-A once, inspect timing decomposition and optimize only the evidenced bottleneck before closing P7.


## 2026-09-19 — P7-A measurement PASS
Evidence: changed-kernel correctness regressions PASS. Full resident pp512 median wall=72,123.8303 ms => 7.098901956 tok/s (~1.047% of frozen R8-VK reference). tg128 wall=37,855.7243 ms => 3.381258776 tok/s (~8.380% reference), median step=271.79145 ms, p95=377.2021 ms. pp submit/wait ~= wall (72,122.7512 vs 72,123.8303 ms); tg submit/wait=37,347.4357 of 37,855.7243 ms, so host/orchestration overhead is not the primary bottleneck.
Decision: P7-A measurement gate PASS; P7 remains OPEN. Do not optimize blindly. Device-side execution is the first evidenced bottleneck class, but P7-A does not attribute cost to individual kernels.
Next: P7-B GPU bottleneck attribution using Vulkan timestamp queries on the unchanged production graph.

## 2026-09-19 — P7-B GPU bottleneck attribution implementation
Evidence target: run one pp512 pass and one cached decode step on the same resident graph while bracketing every dispatch with Vulkan timestamp queries. Aggregate device ticks by semantic kernel family and report top individual dispatches plus synchronization/unattributed share. Preserve all P7-A changed-kernel correctness regressions and residency invariants.
Decision: P7-B is measurement only. No kernel algorithm, quantization, residency architecture, correctness tolerance, or performance threshold is changed. Optimization is deferred until the dominant device-time category is measured.
Next: run P7-B once; select exactly one evidenced bottleneck family for P7-C optimization.


## 2026-09-19 — P7-B provenance materialization repair
Evidence: P7-B profiler execution completed successfully on the target machine and produced authoritative `p7b_profile_results.json` / `p7b_summary.json`; however `results\shader_provenance.json` was absent even though the compile log reached `Shader compile: PASS`. The measured profile remains a valid device-timing result; the missing file is an evidence/provenance materialization defect, not a kernel or performance negative.
Decision: P7-B-R1 changes no shader algorithm, graph, quantization, residency invariant, timing gate, or profile result. It strengthens shader provenance generation with write -> reopen -> parse -> hash/count verification and provides a repair-only entry point that recompiles the unchanged pinned shaders and consolidates the already-returned authoritative P7-B evidence without rerunning the 74-second profile.
Next: run `repair_p7b_provenance.ps1`; once verified provenance is present, freeze P7-B attribution and open P7-C on exactly one bottleneck family: packed FFN GEMM.


## 2026-09-19 — P7-B-R1 provenance repair harness diagnosis
Evidence: R1 shader compilation completed and printed the post-write verification banner, but the parent repair wrapper could not find `results\shader_provenance.json`. Source audit identified the exact harness bug: PowerShell variable names are case-insensitive, so `$Prov` (intended output path) and `$prov` (provenance JSON object) referred to the same variable. The JSON-object assignment overwrote the path before `WriteAllText`. This repeats the previously documented `$Summary`/`$summary` class of PowerShell bug. No P7-B profiler rerun occurred and no scientific evidence changed.
Decision: classify as repair-wrapper/provenance harness defect only. P7-B timing result remains authoritative. P7-B-R2 renames variables to `$ProvPath`, `$ProvObj`, and `$ProvCheck`, verifies the exact resolved output path, and adds a static case-collision audit.
Next: run `repair_p7b_provenance.ps1` from P7-B-R2 only; once the three evidence files are present, freeze P7-B and proceed to P7-C packed FFN GEMM optimization.


## 2026-09-19 — P7-C PASS / prefill FFN tiling frozen
Evidence: tiled Q4_K and Q6_K regressions PASS with max_abs ~2.09e-7 / 2.68e-7. Three live pp512 A/B trials preserve exact top-1 and identical logits within measurement precision. Median baseline=61,824.1135 ms (8.281558295 tok/s); optimized=11,928.5296 ms (42.9223062 tok/s), wall speedup=5.182877989x versus pre-registered >=1.10x gate. Decode path was unchanged and tg128=3.523743842 tok/s.
Decision: P7-C optimization option PASS and is FROZEN. P7 remains OPEN; do not stack another optimization before re-attribution.
Next: P7-D timestamp re-profile the P7-C optimized pp512 graph; profile unchanged decode once for comparison.

## 2026-09-19 — P7-D implementation
Evidence target: timestamp every dispatch on the exact P7-C optimized prefill graph and unchanged decode graph. Preserve op names/categories so P7-B vs P7-D shares are directly comparable.
Decision: measurement only; no new optimization, no new speed threshold, P7 cannot close here.
Next: select exactly one next bottleneck family from P7-D evidence.


## 2026-09-19 — P7-D PASS / bottleneck shifted to attention projections
Evidence: optimized pp512 graph remained correct and timestamp-supported. Device chain profile: attn_qkv=34.3505986%, attn_output=25.06925002%, frozen tiled FFN gate/up=23.70773786%, frozen tiled FFN down=12.79151151%, attention kernel=3.357121108%; barrier/unattributed ~0.0102%. Decode remained unchanged with FFN still dominant.
Decision: P7-D measurement PASS and FROZEN. The next prefill bottleneck is packed attention projections, combined ~59.41985% of chain time. Do not alter attention softmax/LM head/decode yet.
Next: P7-E, exactly one option: reuse the frozen P7-C tiled packed GEMM primitive for prefill Q/K/V/O projections only and A/B against the live P7-C graph.

## 2026-09-19 — P7-E implementation
Evidence target: three interleaved pp512 A/B trials, both arms retaining frozen P7-C FFN tiling. Exactly 112 Q/K/V/O projection dispatches change in the optimized arm. Final logits/top1 are compared live; decode is unchanged and run as non-regression.
Decision: pre-register median pp512 wall speedup >=1.10x; do not change the gate after seeing results. P7 remains OPEN regardless of P7-E outcome.
Next: if PASS, freeze P7-E and re-profile before selecting another family; if FAIL, preserve the negative and do not rescue it by lowering the gate.


## 2026-09-19 — P7-E PASS / prefill attention-projection tiling frozen
Evidence: inherited tiled Q4_K/Q6_K regression PASS. Three live same-run pp512 A/B trials retained frozen P7-C FFN tiling in both arms and changed only 112 Q/K/V/O projection GEMM dispatches. Median P7-C baseline=12,659.7811 ms (40.44303736 tok/s); optimized=6,847.6944 ms (74.76969183 tok/s), wall speedup=1.848765491x versus pre-registered >=1.10x gate. Full logits and top1 matched every trial. Decode graph was unchanged and tg128=3.699161367 tok/s.
Decision: P7-E optimization option PASS and is FROZEN. P7 remains OPEN; do not stack another optimization before re-attribution.
Next: P7-F timestamp re-profile the exact P7-E optimized prefill graph; profile unchanged decode once for comparison.

## 2026-09-19 — P7-F implementation
Evidence target: timestamp every dispatch on the exact P7-E optimized pp512 graph with both FFN and Q/K/V/O projection tiling frozen; unchanged decode graph remains the comparison plane. Preserve op names/categories for direct P7-D -> P7-F attribution.
Decision: measurement only; no new optimization, no new speed threshold, P7 cannot close here.
Next: select exactly one next bottleneck family from P7-F evidence.


## 2026-09-19 — P7-G PASS / FFN token-tile16 frozen
Evidence: tile16 Q4_K/Q6_K regressions PASS; full logits/top1 match live P7-E baseline in all three trials. Median pp512 baseline=6512.1689 ms (78.62203943 tok/s), optimized=5317.6081 ms (96.28389125 tok/s), speedup=1.224642504x versus pre-registered >=1.10x gate. Decode path unchanged at 3.723259824 tok/s.
Decision: P7-G optimization PASS and FROZEN. P7 remains OPEN; do not stack another optimization before re-attribution.
Next: P7-H timestamp re-profile the exact P7-G optimized pp512 graph and unchanged decode graph.

## 2026-09-19 — P7-H implementation
Evidence target: timestamp every dispatch on the frozen P7-G graph: FFN token tile16 plus attention-projection tile8, with unchanged attention kernel/LM head/decode.
Decision: measurement only; no new optimization, no new speed threshold, P7 cannot close here.
Next: select exactly one next bottleneck family from P7-H evidence.

## 2026-09-19 — P7-H-R1 build/package repair
Evidence: first P7-H invocation passed package audit, target SHA, and all shader compilation/provenance, then native build stopped before execution because `tools/build_p7h.ps1` referenced stale `src/p7h_post_attnproj_profile.cpp` while the packaged profiler source is `src/p7h_post_tile16_profile.cpp`. No P7-H profile measurement occurred.
Decision: classify as build/package failure only. P7-H graph, shader set, timestamp measurement contract, inherited regressions, and scientific interpretation remain unchanged. R1 points the build script at the packaged source and adds build-script source-path closure to static audit.
Next: run P7-H-R1 once; only an executed timestamp profile can close P7-H measurement.
## 2026-09-19 — P7-H PASS / post-tile16 attribution frozen
Evidence: P7-H executed from Git commit 7e412731cfb8f8ad19b071ae9b32d8463a14ba11 with inherited regressions PASS, Vulkan timestamp_valid_bits=64, all weights/KV resident, packed Q4_K/Q6_K direct, and finite outputs. Prefill chain wall=4477.6328 ms, submit/wait=4472.9068 ms. FFN gate/up=49.57180228% and FFN down=27.38724097%, combined 76.95904325%; barrier/unattributed=0.02478526%. Decode remains FFN-dominant at 57.47858654% combined, with LM head next at 17.43315699%.
Decision: P7-H PASS and FROZEN. P7 remains OPEN. The next optimization family is prefill FFN packed GEMM; do not change attention, LM head or decode in the same experiment.
Next: P7-I A/B exactly one hypothesis — increase prefill FFN token tile 16 -> 32.

## 2026-09-19 — P7-I implementation
Evidence target: live same-run P7-G/tile16 baseline versus tile32 optimized graph, three interleaved pp512 trials. Tile32 Q4_K/Q6_K regression batch=25 exercises all four per-lane token outputs plus tail; full logits/top1 are compared every trial; tg128 path is unchanged.
Decision: pre-register median pp512 wall speedup >=1.10x with unchanged correctness gates. No gate rescue after result. P7 cannot close in this package.
Next: target-machine pull/run returns shader_provenance.json, p7i_ab_results.json and p7i_summary.json.