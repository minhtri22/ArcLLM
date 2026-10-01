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

## 2026-09-19 — P7-I-R1 Git byte-transport repair
Evidence: first execution from commit a199f78 stopped in static package audit before shader compilation because `run_p7i.ps1` SHA did not match `P7I_SHA256SUMS.txt`. Root cause: text-mode GitHub blob creation dropped the source file's final newline while the checksum manifest was generated from the byte-authoritative working tree. No P7-I shader compile, native build, A/B execution, or performance measurement occurred.
Decision: classify as package/commit-transport failure only. Re-materialize every P7-I checksum-tracked file byte-for-byte, preserve Git checkout EOL policy, regenerate the checksum manifest, and keep the P7-I hypothesis, correctness gates, >=1.10x performance gate, graph, and regression fixture unchanged.
Next: pull P7-I-R1 and run `run_p7i.ps1`; only an executed A/B result can PASS/FAIL P7-I.

## 2026-09-19 — P7-I FAIL / FFN token-tile32 negative frozen
Evidence: tile32 Q4_K/Q6_K regressions PASS with max_abs 2.384185791e-07 / 3.87430191e-07; full pp512 logits and top1 match the live tile16 baseline in all three trials. Median live baseline=7005.143 ms (73.08915749 tok/s), optimized=6981.3753 ms (73.33798543 tok/s), wall speedup=1.003404444x versus the pre-registered >=1.10x gate. The unchanged decode path completed finite but absolute throughput was unusually low (0.9574514708 tok/s versus historical 3.723259824), so absolute cross-run throughput is not used to reinterpret the same-run A/B.
Decision: P7-I is a genuine performance negative and FROZEN. Keep FFN token tile16 as the production candidate; do not rescue tile32 by lowering the gate and do not proceed to tile64. The failed hypothesis is that doubling token reuse from 16 to 32 by itself yields meaningful end-to-end pp512 speedup.
Next: P7-J, exactly one different FFN hypothesis: keep tile16 geometry and replace only the packed Q4_K/Q6_K dequant loader with a block-aware vec4 loader that amortizes repeated metadata/packed-byte extraction across four contiguous weights per invocation.

## 2026-09-19 — P7-J implementation
Evidence target: three interleaved pp512 A/B trials comparing the frozen P7-G tile16 FFN baseline against tile16 FFN with a vec4 block-aware packed dequant loader. Dispatch geometry, token reuse, attention-projection tile8, attention kernel, LM head, residency, KV architecture, and decode remain unchanged. Q4_K/Q6_K specialized regressions use real frozen tensors with batch=9 to exercise both token outputs of tile16; K spans exercise all packed metadata subgroups/quarters.
Decision: pre-register median pp512 wall speedup >=1.10x with unchanged logits max_abs<=0.02, RMSE<=0.005, and exact top1 gates. No absolute historical throughput threshold and no gate rescue after result. P7 remains OPEN regardless of P7-J outcome.
Next: target-machine pull/run returns shader_provenance.json, p7j_ab_results.json, and p7j_summary.json.

## 2026-09-19 — P7-J FAIL / vec4 dequant rejected
Evidence: inherited regressions PASS; optimized Q4_K/Q6_K vec4 regressions PASS with max_abs ~2.38e-7 / 3.87e-7. Three live same-run pp512 A/B trials preserved exact top1 and logits, but median baseline=10,371.9564 ms (49.36387893 tok/s) and optimized=10,560.165 ms (48.48409092 tok/s), wall speedup=0.9821774944x versus pre-registered >=1.10x gate.
Decision: P7-J is a genuine performance negative and is FROZEN. Block-aware vec4 dequant is rejected. Together with P7-I, this closes the current token-reuse/dequant micro-tuning branch; P7-G row8 x token16 remains the frozen FFN baseline. Absolute throughput drift in this run is not used as a gate because the A/B comparison is same-run/interleaved.
Next: P7-K, exactly one independent hypothesis: increase FFN output-row tile 8 -> 16 while keeping token tile16, K32 and workgroup 8x8 unchanged.

## 2026-09-19 — P7-K implementation
Evidence target: three interleaved pp512 A/B trials against the live P7-G row8/token16 baseline. Optimized arm changes only FFN row tile to 16; each 8x8 lane computes two rows x two token halves. Q/K/V/O projection tile8, attention, LM head and decode remain unchanged. Real Q4_K/Q6_K regressions use batch=9.
Decision: pre-register median pp512 wall speedup >=1.10x with unchanged correctness gates. Canonical SHA QA normalizes text EOL/final newline while authoritative P7-J evidence is verified by raw SHA256. P7 remains OPEN regardless of outcome.
Next: target-machine pull/run returns shader_provenance.json, p7k_ab_results.json and p7k_summary.json.

## 2026-09-19 — P7-K-R1 authoritative evidence repair
Evidence: the first P7-K static audit stopped before shader compile/build because inputs/p7j_summary.authoritative.json on GitHub was not byte-identical to the user-returned P7-J summary. The committed blob had a transport transcription error in key pp512_wall_speedup (stored as pp512_wal_speedup). p7j_ab_results and p7j_shader_provenance were independently checked and remain byte-exact.
Decision: classify as repository/provenance packaging failure only. Replace only p7j_summary.authoritative.json with the exact user-uploaded bytes; P7-K hypothesis, shaders, graph, correctness gates and >=1.10x performance gate are unchanged.
Next: rerun P7-K static audit; only an executed A/B run can produce a scientific P7-K verdict.

## 2026-09-19 — P7-K FAIL / row-tile16 rejected
Evidence: inherited regressions PASS; row16 Q4_K/Q6_K regressions PASS with max_abs ~2.38e-7 / 3.87e-7. Three live same-run pp512 A/B trials preserved exact top1 and logits. Median baseline=5,807.862 ms (88.1563646 tok/s), optimized=5,435.8025 ms (94.19032424 tok/s), wall speedup=1.068446103x versus pre-registered >=1.10x gate. Decode path remained unchanged and finite at 3.225888286 tok/s.
Decision: P7-K is a genuine performance negative and is FROZEN. Although row16 improves same-run pp512 by ~6.84%, it does not meet the frozen gate and is not promoted. Do not try row32 mechanically. P7-G row8 x token16 remains the frozen FFN baseline.
Next: P7-L, exactly one architectural hypothesis: fuse Q4_K FFN gate+up so both weight matrices share one activation-tile load and one dispatch/barrier schedule; keep FFN down and all non-FFN paths unchanged.

## 2026-09-19 — P7-L implementation
Evidence target: three interleaved pp512 A/B trials against the live P7-G baseline. Baseline uses 56 gate/up GEMM dispatches across 28 layers; optimized uses 28 fused gate+up dispatches. Fused specialized regression compares both real layer-0 gate and up outputs to CPU references. Attention projections, attention kernel, LM head, FFN down and decode remain unchanged.
Decision: pre-register median pp512 wall speedup >=1.10x with unchanged logits max_abs<=0.02, RMSE<=0.005, and exact top1 gates. P7 remains OPEN regardless of outcome.
Next: target-machine pull/run returns shader_provenance.json, p7l_ab_results.json and p7l_summary.json.

## 2026-09-19 — P7-L PASS / fused gate+up frozen
Evidence: inherited regressions PASS; fused real layer-0 Q4_K gate/up regression PASS with max_abs 2.384185791e-07 / 1.490116119e-07. Three live same-run pp512 A/B trials preserved exact top1 and logits. Median baseline=10,280.4444 ms (49.8032945 tok/s), optimized=8,271.1628 ms (61.90181627 tok/s), wall speedup=1.242926134x versus pre-registered >=1.10x gate. Optimized prefill replaces 56 separate gate/up GEMMs with 28 fused dispatches; FFN down and all non-FFN paths remain frozen.
Decision: P7-L is a genuine PASS and is FROZEN. Absolute cross-run throughput drift, including low unchanged decode throughput, is context only and does not alter the same-run A/B verdict. Do not stack another optimization before re-attribution.
Next: P7-M timestamp re-profile the exact P7-L winner and unchanged decode graph.

## 2026-09-19 — P7-M implementation
Evidence target: timestamp every dispatch on the exact P7-L prefill graph (441 dispatches) and one unchanged cached decode step (469 dispatches). Preserve semantic categories so P7-H -> P7-M absolute ticks and chain shares are comparable. Include inherited regressions plus fused gate/up regression.
Decision: measurement only; no throughput threshold, no optimization, and P7 cannot close here. The next optimization family must be chosen only from P7-M attribution.
Next: target-machine pull/run returns shader_provenance.json, p7m_profile_results.json and p7m_summary.json.

## 2026-09-19 — P7-M-R1 static-QA/source repair
Evidence: first P7-M invocation stopped in static audit before shader compile/build because the Python test searched for an unescaped JSON-writer literal. Review also found a latent C++ syntax defect in the pre-profile dispatch-count log (missing << before "\\n") and a stale ERROR schema name from P7-H. No P7-M executable was built and no timestamp measurement occurred.
Decision: classify as QA/source packaging failure only. R1 changes only the test literal, the console-log syntax, and the error-path schema. Exact P7-L graph, 441/469 dispatch contract, timestamp measurement, regressions, and no-performance-threshold gate are unchanged.
Next: rerun P7-M static audit; only an executed timestamp profile can produce the P7-M scientific result.

## 2026-09-19 — P7-M PASS / post-fusion attribution frozen
Evidence: inherited Q4/Q6, prefill-attention, cached-attention, and fused gate/up regressions all PASS. Vulkan timestamp_valid_bits=64. Exact P7-L prefill graph has 441 dispatches and unchanged decode has 469. Prefill chain_ticks=79,570,264, wall=4,155.44 ms, submit/wait=4,144.7918 ms. Category shares: fused gate/up=44.40335651% (35,331,868 ticks), FFN down=30.42145996% (24,206,436), attention=9.338470964%, attn_qkv=7.593908448%, attn_output=5.862341992%; barrier/unattributed=19,355 ticks = 0.0243244%. Decode remains unchanged and is led by separate FFN gate/up=34.320407%, FFN down=21.64503245%, LM head=17.99803932%, attention=11.187622%, attn_qkv=10.44633931%.
Decision: P7-M measurement PASS and FROZEN. No performance threshold applied and P7 remains OPEN. Device attribution shows orchestration/barriers are negligible; the largest prefill family remains the gate/up path even after P7-L. Do not optimize decode or FFN-down in the same experiment.
Next: P7-N, exactly one hypothesis — fuse SwiGLU into the frozen P7-L gate+up kernel and write final s directly, eliminating gate/up intermediate writes/reads plus one SwiGLU dispatch per layer.

## 2026-09-19 — P7-N implementation
Evidence target: three interleaved pp512 A/B trials. Baseline is the exact frozen P7-L winner (441 prefill dispatches). Optimized changes only prefill gate/up+SwiGLU into one Q4_K fused kernel per layer (413 dispatches). FFN down, attention projections, attention kernel, LM head, residency, KV path, and decode remain unchanged; decode stays at 469 dispatches. Specialized regression compares final s=SiLU(gate)*up against CPU reference on real layer-0 gate/up weights.
Decision: pre-register median pp512 wall speedup >=1.10x with unchanged logits max_abs<=0.02, RMSE<=0.005, exact top1, and finite decode gates. P7 remains OPEN regardless of outcome; no gate rescue after result.
Next: target-machine pull/run returns shader_provenance.json, p7n_ab_results.json and p7n_summary.json.

## 2026-09-19 — P7-N FAIL / gate+up+SwiGLU fusion rejected
Evidence: inherited regressions PASS; fused final SwiGLU regression PASS with max_abs=2.235174179e-08 and RMSE=1.708517748e-09. Three live same-run pp512 A/B trials preserved exact top1 and logits. Median exact P7-L baseline=5,254.987 ms (97.43125911 tok/s), optimized=4,997.0773 ms (102.4598919 tok/s), wall speedup=1.051612109x versus pre-registered >=1.10x gate. Decode remained unchanged and finite at 3.5930945 tok/s.
Decision: P7-N is a genuine performance negative and is FROZEN. Fusing SwiGLU into the gate+up kernel is rejected despite ~5.16% same-run improvement because it misses the frozen gate. Keep exact P7-L as baseline; do not rescue P7-N by lowering the threshold or stack it into later experiments.
Next: move to the next independent P7-M bottleneck family, FFN-down (30.42145996% of prefill chain time).

## 2026-09-19 — P7-O implementation
Evidence target: three interleaved pp512 A/B trials against exact P7-L. Optimized arm changes only prefill FFN-down K tile 32 -> 64 for both Q4_K and Q6_K while row8, token16, workgroup8x8, P7-L gate+up fusion, separate SwiGLU, attention paths, LM head and decode remain unchanged. Both arms retain 441 prefill dispatches; decode remains 469. Specialized regressions use real frozen Q4_K and Q6_K down tensors with batch=9.
Decision: pre-register median pp512 wall speedup >=1.10x with unchanged correctness gates. The hypothesis is that K64 halves internal FFN-down tiled loop/barrier iterations for K=8960 from 280 to 140 without changing graph semantics. P7 remains OPEN regardless of outcome.
Next: target-machine pull/run returns shader_provenance.json, p7o_ab_results.json and p7o_summary.json.

## 2026-09-19 — P7-O-R1 K64 implementation/harness repair
Evidence: first P7-O run passed static audit, shader provenance/compile, native build, and inherited regressions, then specialized FFN-down K64 regressions failed for both Q4_K and Q6_K before any pp512 A/B trial. Inspection found the K64 transform changed only the first of two per-lane accumulation loops from kk<32 to kk<64; the token-half-B loop remained kk<32. Batch=9 intentionally exercised half-B, so both quantized variants failed. The runner then dereferenced scope from the minimal ERROR JSON and produced a secondary PowerShell failure.
Decision: classify as implementation/harness failure, not a scientific P7-O verdict. R1 changes only the stale half-B loop in both K64 shaders, adds regression max_abs/RMSE diagnostics, and guards ERROR JSON before scope dereference. P7-O hypothesis, exact P7-L baseline, K32->K64-only scope, correctness gates, and >=1.10x performance gate are unchanged.
Next: rerun P7-O-R1. Only if specialized K64 regressions PASS should the three interleaved pp512 A/B trials be interpreted scientifically.

## 2026-09-19 — P7-O FAIL / FFN-down K64 rejected
Evidence: after R1 repair, inherited regressions and specialized real-tensor Q4_K/Q6_K K64 regressions all PASS (Q4 max_abs=4.470348358e-07, RMSE=6.383054468e-08; Q6 max_abs=3.576278687e-07, RMSE=6.53922368e-08). Three interleaved pp512 A/B trials preserve exact top1 and logits. Median exact P7-L baseline=6,072.018 ms (84.32122566 tok/s), optimized K64=6,871.3705 ms (74.51206422 tok/s), wall speedup=0.8836691312x versus pre-registered >=1.10x gate. Decode remains unchanged and finite at 3.326577289 tok/s.
Decision: P7-O is a genuine performance negative and is FROZEN. K64 is rejected; do not proceed mechanically to K128. P7-L remains the frozen production winner.
Next: P7 closeout review rather than another micro-optimization.

## 2026-09-19 — P7 CLOSED / P7-L production path frozen
Evidence: direct packed Q4_K/Q6_K, full decoder residency, GPU-resident KV, end-to-end correctness and reproducible shader provenance are established. P7-L is the last optimization option to clear its pre-registered >=1.10x gate (1.242926134x). P7-M re-profile confirms the winner and shows remaining single-family shares: attention 9.3385%, attn_qkv 7.5939%, attn_output 5.8623%; barrier/unattributed ~0.0243%. Subsequent correctness-preserving options P7-N and P7-O fail their frozen performance gates.
Decision: close P7 as the Q4_K_M production path. This is not a claim of global optimality and introduces no new post-hoc throughput gate. Remaining attention/decode/performance ideas move to future optimization work and do not block scaling.
Next: P8 7B memory-planned runtime. Freeze exact 7B GGUF artifact + SHA256 before executable implementation.

## 2026-09-19 — P7 closeout R1 provenance byte-repair
Evidence: first P7 closeout static audit stopped on raw SHA256 mismatch for inputs/p7o_shader_provenance.authoritative.json. The user-returned authoritative file is 8,361 bytes with SHA256 59AD51C2DA5D68D01C387592635D675D674490CCC8BCE4B060DB93984B898EC2. Inspection of the committed blob found one transport/transcription omission: compiled entry p7c_ffn_q6k_tiled.comp was missing its spv_bytes=12412 field. P7-O A/B results and summary were already byte-exact, and no scientific value changed.
Decision: classify as closeout provenance packaging failure only. Replace the provenance file byte-for-byte from the original user upload, preserve inputs/*.json as -text, and strengthen closeout QA to require 18 compiled entries with spv_bytes present on every entry. P7-O FAIL, P7-L frozen winner, and P7 CLOSED remain unchanged.
Next: rerun tests/test_p7_closeout.py; then proceed to P8 target freeze.

## 2026-09-19 — P8 target freeze
Evidence: P7 closeout PASS on target. Official Hugging Face repository Qwen/Qwen2.5-Coder-7B-Instruct-GGUF exposes qwen2.5-coder-7b-instruct-q4_k_m.gguf at revision 13fb94bfda8c8cf22497dc57b78f391a9acb426a. Frozen remote file size=4,683,073,536 bytes and SHA256=509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C.
Decision: freeze this exact artifact for P8. Initial context=4096, prefill chunk=512, inherited arena cap=256 MiB. Preserve the previously observed 17.25 GiB target Vulkan budget and reserve 2 GiB before feasibility judgment. No artifact substitution is allowed inside P8-A.
Next: P8-A memory-plan-only bring-up.

## 2026-09-19 — P8-A implementation
Question: can the exact frozen 7B Q4_K_M artifact inherit P7-L packed residency without any Vulkan allocation or inference?
Implementation: native planner reuses GgufReader only; verifies model metadata, quant mix, tensor spans, deterministic <=256 MiB tensor-aware arena packing, single-tensor fit, FP32 KV@4096, and P7-L-equivalent pp512 working buffers. Runner performs authoritative size+SHA verification before planning. Full inference is explicitly forbidden until PASS.
Gate: exact file identity; qwen2 metadata; F32/Q4_K/Q6_K-only quant mix; non-overlap; no single tensor >256 MiB; every arena <=256 MiB; total planned bytes <=15.25 GiB usable budget after 2 GiB reserve. No performance gate.
Decision: package READY_TO_RUN. Any FAIL must be adjudicated as a concrete memory-contract obstruction; do not relax arena cap or memory formula post hoc.
Next on PASS: P8-B residency-allocation bring-up. Next on FAIL: design the minimum architecture change required by the reported obstruction before any full inference.

## 2026-09-19 — P8-A-R1 pre-execution target amendment: local Ollama artifact
Evidence: before any P8-A scientific execution, the target machine was found to already contain the intended Qwen2.5-Coder 7B Q4_K_M-class model in Ollama. Its manifest model layer is mediaType=application/vnd.ollama.image.model, digest=sha256:60e05f2100071479f596b964f89f510f057ce397ea22f2833a0cfe029bfc2463, size=4,683,074,048 bytes. This differs from the previously frozen Hugging Face candidate (4,683,073,536 bytes; SHA256 509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C) by 512 bytes and therefore cannot be treated as byte-identical or silently substituted.
Decision: because no P8-A result has yet been observed, amend the frozen target before execution to the exact local Ollama content-addressed blob, SHA256 60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463, size 4,683,074,048. The prior HF artifact becomes a superseded, never-executed candidate. Scientific memory gates remain unchanged.
Implementation: add manifest-based local resolver. It scans the qwen2.5-coder manifest directory, accepts only the frozen model-layer mediaType+digest+size, resolves the corresponding blobs/sha256-* path, verifies size, and returns the local blob. run_p8a.ps1 independently re-hashes the actual file before planning. The compatibility fetch script performs no network download.
Next: pull P8-A-R1, run static QA, then run P8-A directly; no download step is required.

## 2026-09-19 — P8-A-R1 FAIL / single-tensor arena obstruction established
Evidence: exact local Ollama target size/SHA verification PASS. GGUF v3 parses as qwen2, 28 layers, hidden=3584, q_heads=28, kv_heads=4, head_dim=128, kv_dim=512, FFN=18944, vocab=152064, model context=32768; 339 tensors with only F32/Q4_K/Q6_K and no overlap. Total weight payload span=4,677,120,000 bytes, FP32 KV@4096=469,762,048 bytes, pp512 working buffers=200,888,324 bytes, total planned=5,347,770,372 bytes against 16,374,562,816 usable bytes, leaving 11,026,792,444 bytes headroom. Capacity PASS. The only failed gates are arena_pack_pass and no_oversize_tensor_pass; exactly token_embd.weight and output.weight exceed the inherited 256 MiB single-tensor arena contract.
Decision: classify as a genuine P8 memory-architecture obstruction, not an environment failure and not a total-capacity failure. Do not increase the 256 MiB arena cap, reduce context, alter KV precision, or change quantization to rescue the result. Full inference remains forbidden.
Authoritative evidence SHA256: p8a_memory_plan=6495545700E4104B05ED7D913ACC728C64329CB92AC894342A690946EDECE31E; p8a_summary=C191DC70AEBAEEF733847AFE8B15ECECC95BE112F1041178DD307FE68D981EA9.
Next: P8-A2, exactly one architecture hypothesis — row-aligned physical segmentation of only the two oversize logical vocab tensors while preserving all frozen memory assumptions.

## 2026-09-19 — P8-A2 implementation
Question: can the exact P8-A obstruction be removed by representing an oversize logical tensor as multiple <=256 MiB row-aligned physical segments, without changing packed tensor bytes, quantization semantics or total memory?
Implementation: metadata-only planner derives exact type/dims/row_bytes for every tensor. Non-oversize tensors remain unsplit. Oversize tensors are split only at complete row boundaries; segment tables record global row range, file range, arena and arena-local byte base. Deterministic physical arenas may cut at segment boundaries but remain <=256 MiB. Addressability qualification requires token_embd.weight global-token-row -> segment/local-row mapping and output.weight global-vocab-row -> segment/local-row mapping. No Vulkan allocation, shader compile or inference.
Gate: parent obstruction must match exactly token_embd.weight + output.weight; exact row coverage; no row/block split; every segment <=256 MiB; every piece contained in exactly one <=256 MiB arena; addressability PASS; unchanged total-capacity PASS. No performance gate.
Next on PASS: P8-B segmented residency allocation/copy bring-up. Next on FAIL: adjudicate the remaining segmented-plan obstruction without relaxing the cap.

## 2026-09-19 — P8-A2 PASS / segmented large-tensor plan frozen
Evidence: exact target identity PASS. Parent obstruction matches exactly token_embd.weight + output.weight. token_embd.weight is Q4_K [3584,152064], row_bytes=2016, packed bytes=306,561,024, split into 133,152 + 18,912 rows. output.weight is Q6_K [3584,152064], row_bytes=2940, packed bytes=447,068,160, split into 91,304 + 60,760 rows. The plan contains 19 arenas and 341 physical pieces; all rows are covered exactly once, every piece is contained once, both global-row address mappings PASS, and all arenas remain <=256 MiB. Total planned residency remains 5,347,770,372 bytes with 11,026,792,444 bytes headroom against the frozen usable budget.
Decision: P8-A2 is a genuine PASS and is FROZEN. The single-tensor arena obstruction from P8-A is resolved by row-aligned physical segmentation without changing quantization, KV, context, working buffers or total memory. Full inference remains forbidden.
Authoritative evidence SHA256: p8a2_segment_plan=7390D579937EDD8748A08D7D72E7DFEC53D7470FA9C86208C2412A33E231D481; p8a2_summary=2534F8AC68134562F8D0AD1F5CB6464440C368E48659FC0DE1EB051C02B74876.
Next: P8-B segmented Vulkan residency allocation/copy bring-up.

## 2026-09-19 — P8-B implementation
Question: can the exact frozen P8-A2 plan be instantiated simultaneously in the validated P7-style DEVICE_LOCAL|HOST_VISIBLE|HOST_COHERENT Vulkan memory on the target Arc 140V?
Implementation: reconstruct and require exact equality to the frozen 19 arena boundaries; map the GGUF read-only through TensorStore; allocate all 19 weight arenas, two FP32 KV buffers and the exact 20 P7-equivalent working buffers concurrently. Weight arenas are initialized from GGUF and full-memcmp verified across all 4,677,120,000 bytes. Exhaustive embedding/output row translation is rechecked against arena-local addresses. Requested and actual VkMemoryRequirements allocation bytes are reported.
Gate: exact parent plan; valid P7 memory flags; 19 simultaneous weight arenas; full weight byte compare; exact K/V and working residency; requested bytes exactly 5,347,770,372; actual allocation bytes <=16,374,562,816 usable budget. No shader module, compute dispatch or inference; no performance gate.
Next on PASS: P8-C segmented embedding/LM-head access correctness bring-up. On build/init/package failure: repair harness only. On reproducible allocation/copy FAIL after plan match: adjudicate residency obstruction without shrinking the frozen plan.

## 2026-09-19 — P8-B-R1 authoritative parent-evidence repair
Evidence: first P8-B static audit stopped before native build/Vulkan execution on raw SHA mismatch for inputs/p8a2_segment_plan.authoritative.json. Comparison against the original user-uploaded P8-A2 plan found one transport/transcription corruption near the final gate object: committed key "rcontained_once_pass" instead of authoritative "contained_once_pass". The committed plan was 4,118 bytes; the authoritative upload is 4,117 bytes with SHA256 7390D579937EDD8748A08D7D72E7DFEC53D7470FA9C86208C2412A33E231D481. p8a2_summary.authoritative.json was already byte-exact at 595 bytes and SHA256 2534F8AC68134562F8D0AD1F5CB6464440C368E48659FC0DE1EB051C02B74876.
Decision: classify as repository/provenance packaging failure only. Restore only the P8-A2 plan from the original uploaded bytes and strengthen static QA with exact byte sizes plus contained_once_pass semantic key checks. P8-A2 PASS, P8-B hypothesis, frozen residency plan, and all P8-B gates remain unchanged.
Next: rerun tests/test_p8b_package.py; only after static PASS may P8-B native build/Vulkan residency execute.

## 2026-09-19 — P8-B PASS / segmented Vulkan residency frozen
Evidence: exact target and parent plan match PASS. Vulkan exposes one device, compute queue family 0, memory type index 3 with flags=15 and required DEVICE_LOCAL|HOST_VISIBLE|HOST_COHERENT PASS. All 19 weight arenas allocate simultaneously and full byte-compare all 4,677,120,000 weight bytes. Two FP32 KV buffers allocate at 469,762,048 bytes; all 20 working buffers allocate at 200,888,324 bytes. Requested residency is exactly 5,347,770,372 bytes; actual VkMemoryRequirements allocation totals 5,347,770,496 bytes, only 124 bytes overhead and far below the frozen 16,374,562,816 usable budget. No failure stage.
Decision: P8-B is a genuine PASS and is FROZEN. Segmented 7B residency is physically realizable on the target Arc 140V under the frozen plan. Full inference remains forbidden.
Authoritative evidence SHA256: p8b_residency_results=127E25B9CA8F8B4C74CDEBAFB6559CAFF078EFFE7F1513BD1A7429FD0E69B3B2; p8b_summary=078FE23DF5B31B3D69162BADC6285D608F0EB1178430E38A56DD0FF827EC2B5C.
Next: P8-C segmented embedding/LM-head GPU access correctness bring-up.

## 2026-09-19 — P8-C implementation
Question: can GPU kernels resolve global vocab rows across the two frozen segment boundaries and reproduce CPU references from original GGUF bytes?
Implementation: exactly two specialized correctness dispatches. The embedding probe binds both Q4_K token_embd segments and dequantizes eight preregistered token rows including boundary-1=133151 and boundary=133152; all 28,672 output floats are compared to CPU dequant. The LM-head probe binds both Q6_K output segments and computes eight preregistered row dot-products including boundary-1=91303 and boundary=91304 against one deterministic FP32 vector; all eight logits are compared to CPU q6 reference. Pinned glslang 16.5.0 provenance is required.
Gate: finite outputs; per-probe max_abs<=0.02 and RMSE<=0.005; exact preregistered boundary probes; exactly two dispatches in one submit. No performance gate and no transformer-layer/prefill/decode execution.
Next on PASS: P8-D segmented access integration into the 7B graph. On package/shader/runtime failure: repair harness only. On correctness FAIL: adjudicate segmented address/dequant path before integration.


## 2026-09-19 — P8-C pre-run mapping-equivalence QA repair
Evidence: independent review of the READY_TO_RUN P8-C package found that boundary-focused GPU numerical comparison was implemented, but the separately frozen pre-dispatch mapping-equivalence check from the handoff was omitted. The required invariant is segment_source_offset + local_row*row_bytes == tensor_source_offset + global_row*row_bytes for every preregistered embedding and LM-head probe row.
Decision: classify as a pre-run harness/contract omission, not a scientific result. Restore only the missing independent address-translation gate; do not change the P8-C hypothesis, probe rows, segmentation geometry, numerical thresholds, dispatch count, quantization, or full-inference prohibition. P8-C remains READY_TO_RUN and has not yet been scientifically adjudicated.
Next: rerun tests/test_p8c_package.py, then run P8-C on the target machine. Only target-machine JSON may adjudicate P8-C PASS/FAIL.


## 2026-09-19 — P8-C PASS / segmented access correctness frozen
Evidence: target-machine run at commit 65e07b06e1e6bf6f9c514c78fa24cf2579e062c0 passed static contract, pinned shader compile, native build and runtime. Exact target SHA256 remained 60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463. Pre-dispatch mapping equivalence passed independently for embedding and LM-head. Boundary-adjacent probe sets were preserved exactly. Embedding compared 28,672 FP32 values with max_abs=0 and RMSE=0. LM-head compared eight selected logits with max_abs=2.38418579102e-07 and RMSE=1.11027394095e-07. Execution was exactly two dispatches, one submit and one fence wait; all outputs finite. process_exit_code=0 and every P8-C gate is true.
Build note: MSVC C4505 warnings reported only unused internal helper functions removed by the optimizer; build completed successfully and these warnings do not affect the executed P8-C path.
Decision: P8-C is a genuine PASS and is FROZEN. The P8-A2 row-segment mapping plus P8-B residency plan is now qualified for graph-level integration at both oversize vocab-tensor endpoints. This does not establish decoder-layer or full-model inference correctness. full_inference_permitted remains false.
Authoritative evidence SHA256: shader_provenance=25D7E7D01F033B85692E83C102D269A37E21988D229191BCA5FF51DBC0E118E3; p8c_access_results=9773C48A7D895EB3D22B993132854E58BC0668288725E5186E80D3462D4D5340; p8c_summary=8620A9089CF9066088F5E30543864D7A17B87F4DC55CEFE1CD10320C01574CD4.
Repository preservation: p8c_access_results and p8c_summary are stored byte-exact as authoritative inputs. The shader_provenance raw file was not supplied in chat, so only its user-reported SHA256 is frozen here; no provenance JSON content is fabricated.
Next: P8-D segmented access integration into the 7B graph.

## 2026-09-19 — P8-D design frozen
Question: can the existing production graph consume the exact 19-arena/341-piece P8-A2 map through a graph-level tensor-binding resolver without changing non-segmented tensor semantics?
Design: retain all 7B architecture, quantization, P7-L kernel choices, KV/context and memory assumptions. Resolve ordinary tensors to one physical slice and only token_embd.weight/output.weight to two row-aligned slices. Require exhaustive graph tensor binding and byte-span equivalence. Re-route only the two P8-C endpoint probes through the graph resolver/executor; decoder-layer, prefill, decode and generation execution remain disabled.
Decision: P8-D contract is DESIGN_FROZEN and implementation is permitted. It is not READY_TO_RUN until code plus static QA are committed.
Next on P8-D PASS: P8-E bounded single-layer 7B graph correctness bring-up. Full 28-layer inference remains forbidden.


## 2026-09-19 — P8-D implementation / READY_TO_RUN
Implementation: added TensorBindingDescriptor/P8DBindingSlice graph-level residency descriptors on the frozen 7B target. The graph census is explicit: 3 top-level tensors + 28 blocks x 12 tensors = 339 logical tensors. The P8-A2 row-segmentation rule is independently recomputed from GGUF geometry and must reproduce the exact 19-arena plan. Resolver QA requires exactly 341 physical pieces, exactly two multi-piece logical tensors (token_embd.weight and output.weight), zero missing/ambiguous bindings, per-tensor byte-span equivalence, and contiguous unique ownership of all 4,677,120,000 weight bytes.
Execution boundary: all 19 weight arenas are allocated, but the only compute operations are the existing two P8-C endpoint shaders. Their Vulkan buffers are obtained from bindings.at("token_embd.weight") and bindings.at("output.weight"), not direct hard-coded payload offsets. Mapping equivalence is rechecked through the graph descriptors. decoder_layer_dispatches is frozen at 0; no prefill, decode, KV-attention or generation path is constructed.
Parent provenance: runner verifies byte-exact P8-A2 plan SHA256 7390D579937EDD8748A08D7D72E7DFEC53D7470FA9C86208C2412A33E231D481, P8-C access SHA256 9773C48A7D895EB3D22B993132854E58BC0668288725E5186E80D3462D4D5340 and P8-C summary SHA256 8620A9089CF9066088F5E30543864D7A17B87F4DC55CEFE1CD10320C01574CD4. The missing raw P8-C shader provenance remains represented only by its frozen hash; P8-D produces fresh provenance for the unchanged shader sources.
Decision: P8-D is READY_TO_RUN. No scientific verdict exists until target-machine JSON is returned. On package/build/compiler failure, repair infrastructure only. On resolver/span FAIL, adjudicate graph-binding architecture. On numerical FAIL after resolver PASS, adjudicate descriptor propagation before any layer execution.
Next on PASS: P8-E bounded single-layer 7B graph correctness bring-up. Full 28-layer inference remains forbidden.


## 2026-09-19 — P8-C raw shader-provenance preservation completed
Evidence: the previously missing raw P8-C shader_provenance.json was supplied later. Its exact raw SHA256 is 25D7E7D01F033B85692E83C102D269A37E21988D229191BCA5FF51DBC0E118E3, matching the hash frozen at P8-C closeout. It records glslang 16.5.0 / Vulkan 1.2 and the exact same source/SPIR-V hashes later reproduced by P8-D for both segmented endpoint shaders.
Decision: provenance gap only is closed. Store the raw file byte-exact as inputs/p8c_shader_provenance.authoritative.json and point P8C_SHA256SUMS.txt at the authoritative input. No P8-C scientific result or gate changes.


## 2026-09-19 — P8-D PASS / graph binding integration frozen
Authoritative evidence: p8d_shader_provenance SHA256 FABD3DAE027DB5AA69E037FE179BCD0C4F5A16C5D3AAFF0E08A938101AC8F454; p8d_graph_binding_results SHA256 53C373BD3BA1A9BD31B45702CED08E2EB39C7F2B7B099E052EC8C5153A824ABD; p8d_summary SHA256 74624BFAC44F4E5B9A6F08AC972508A353DAB96E0B6D4B92E8C658ABB1651D27.
Result: status=PASS, target tensor_count=339 and exact frozen 7B geometry. Resolver required/resolved 339/339 tensors with missing=0 and ambiguous=0; exact 19 arenas, 341 physical pieces, two segmented logical tensors and 337 ordinary one-piece tensors. span_equivalence_pass=true, global_coverage_pass=true, segmented_names_pass=true and segment_geometry_pass=true. Independent embedding/LM-head mapping equivalence remained true. Embedding compared 28,672 values with max_abs=0 and RMSE=0. LM-head compared eight selected logits with max_abs=2.38418579102e-07 and RMSE=1.11027394095e-07. Execution was exactly two dispatches, one submit and one fence wait; decoder_layer_dispatches=0 and all outputs finite. Every P8-D gate is true and process_exit_code=0.
Decision: P8-D is a genuine PASS and is FROZEN. The P8-A2 segmented physical map is now qualified through graph-level logical tensor bindings, including exhaustive payload ownership and the two segmented endpoints. This still does not establish decoder-layer correctness or full-model inference correctness.
Next: P8-E bounded single-layer 7B graph correctness bring-up. Full 28-layer inference remains forbidden.

## 2026-09-19 — P8-E design frozen
Question: can exactly layer 0 at the frozen 7B dimensions execute through the P8-D graph-binding resolver using the frozen P7-L kernel path and match an independent CPU reference?
Design: execute one layer only, sequence length 4, position base 0, deterministic synthetic hidden input, all 19 weight arenas resident. The frozen chain has exactly 15 operations: attention RMSNorm; Q/K/V projections; Q/K RoPE; layer-0 KV store; causal GQA; output projection; attention residual; FFN RMSNorm; fused P7-L gate+up; SwiGLU; P7-G FFN down; FFN residual. Compare CPU-vs-GPU at all major checkpoints and final layer output with max_abs<=0.02 and RMSE<=0.005. Exact execution gate is 15 dispatches, one submit, exactly one decoder layer, layer index 0.
Exclusions: no embedding, output_norm, LM-head, decode, sampling, generation, second layer or performance trial.
Decision: P8-E contract is DESIGN_FROZEN and implementation is permitted. It is not READY_TO_RUN until implementation plus static QA are committed.
Next on PASS: P8-F bounded multi-layer-prefix correctness design. Full 28-layer inference remains forbidden.


## 2026-09-19 — P8-E implementation / READY_TO_RUN
Implementation: added a bounded layer-0 executable at the exact frozen 7B geometry. The executable independently rebuilds the P8-D 339-tensor binding census and exact 19-arena / 341-piece map, requires every layer-0 tensor to resolve to exactly one slice, keeps all 19 weight arenas resident, and uses resolved arena/base pairs for every GPU weight access. CPU reference values are generated directly from original GGUF bytes and never from GPU intermediates.
Execution: frozen synthetic hidden input, seq=4, pos_base=0, layer=0 only. The prepared GPU chain is exactly 15 dispatches in one submit: attention RMSNorm; Q/K/V projections; Q/K RoPE; layer-0 KV store; causal GQA; output projection; attention residual; FFN RMSNorm; fused P7-L gate+up; SwiGLU; P7-G FFN down; FFN residual. The V projection and FFN-down select Q4_K or Q6_K from the exact target format. No embedding, output_norm, LM-head, decode, generation, second layer or performance path is constructed.
Observation contract: 17 CPU-vs-GPU checkpoints are preserved, including K/V cache rows for positions 0..3 and mandatory final layer output. Every floating checkpoint retains max_abs<=0.02 and RMSE<=0.005 plus finite-value requirement. Exact execution gates remain 15 dispatches, one submit, exactly one decoder layer, layer index 0.
Provenance: runner verifies byte-exact authoritative P8-D evidence before static QA/build/run and produces fresh provenance for the 11 required frozen shaders using pinned glslang 16.5.0.
Decision: P8-E is READY_TO_RUN. No scientific verdict exists until target-machine JSON is returned. Build/compiler/environment errors remain infrastructure failures only. A numerical FAIL is adjudicated at the first failing checkpoint; no additional layers may run.
Next on PASS: P8-F bounded multi-layer-prefix correctness design. Full 28-layer inference remains forbidden.


## 2026-09-19 — P8-E static-QA false-positive repair
Observed before runtime: tests/test_p8e_package.py rejected the source because it globally banned the text `for(uint32_t l=0;l<28`. The only matching loop is inside `p8e_graph_names()`, where all 28 model layers are enumerated solely to reconstruct the frozen 339-tensor graph census required by P8-D/P8-E binding QA. It does not dispatch or execute decoder layers. The actual P8-E execution builder remains a statically enumerated set of exactly 15 `addop("L0....")` operations for layer 0.
Repair: replace the over-broad text ban with two scoped checks: require the 28-layer loop inside the graph-name metadata function, and require no layer loop inside the DispatchOp construction region while retaining exactly 15 layer-0 addop calls.
Classification: package/static-QA defect only. No shader, runtime, numerical gate, P8-E contract, target evidence, or scientific hypothesis changed. No scientific run occurred because the runner stopped at static QA.
Decision: P8-E remains READY_TO_RUN under the original frozen contract.


## 2026-09-19 — P8-E PASS / bounded single-layer correctness frozen
Authoritative evidence: p8e_shader_provenance SHA256 73916DE149A413B835541B95F01FEAF2B87DDDE03C039C3D1ABC0E2A3A115861; p8e_single_layer_results SHA256 992A986081FAFC81AC2E6E1A38063384434DE3138CAE469A04A57FED57BDA52B; p8e_summary SHA256 EBC4C8088B0992A30D72973DC7485CCF7AA0618B354509A506CC9FDE294B5B9D.
Result: status=PASS on the exact 7B target. Binding retained 19 arenas, 341 physical pieces, two segmented logical tensors, span/global coverage PASS, and all layer-0 tensors were single-piece. The exact target formats observed for layer 0 were Q6_K for attn_v and Q6_K for ffn_down. All 17 CPU-vs-GPU checkpoints were finite and PASS. Worst numerical checkpoint was k_rope (and identically the stored K-cache rows), max_abs=0.00276947021484 and RMSE=0.000176462817402. Final layer output max_abs=0.000296622514725 and RMSE=1.74611629593e-05. Execution was exactly 15 dispatches, one submit, one fence wait, layer index 0, executed_layer_count=1. Every P8-E gate is true and first_failing_checkpoint is empty. Summary process_exit_code=0, p8e_pass=true, full_inference_permitted=false.
Decision: P8-E is a genuine PASS and is FROZEN. This proves bounded layer-0 correctness at the 7B geometry but does not establish multi-layer or full-model inference correctness.
Next: P8-F bounded two-layer-prefix correctness. Full 28-layer inference remains forbidden.

## 2026-09-19 — P8-F design frozen
Question: does the validated P8-E layer path compose correctly across one real layer boundary when layer-0 GPU output becomes layer-1 GPU input?
Design: execute exactly layers [0,1], seq=4, same deterministic input, same 19-arena / 341-piece resolver, same P7-L/P7-G kernels and same numerical gates. CPU reference computes layers 0 then 1 independently from original GGUF bytes; GPU layer 1 must consume the unmodified GPU layer-0 output. Record the same 17 observations per layer, 34 total. Exact execution gate is 30 dispatches in one submit and exactly two executed decoder layers.
Failure localization: any layer-0 failure is a P8-E regression; layer-0 PASS followed by a layer-1 checkpoint failure is the first genuine cross-layer composition obstruction.
Exclusions: no embedding, output_norm, LM-head, decode, sampling, generation, layer 2+, or performance trial.
Decision: P8-F contract is DESIGN_FROZEN and implementation is permitted. It is not READY_TO_RUN until implementation plus static QA are committed.
Next on PASS: P8-G larger bounded-prefix correctness design. Full 28-layer inference remains forbidden.


## 2026-09-19 — P8-F implementation / READY_TO_RUN
Implementation: added a bounded two-layer-prefix executable at the exact frozen 7B geometry. It independently reconstructs the 339-tensor graph binding census and exact 19-arena / 341-piece map, validates target formats for layers 0 and 1, and requires every executed-layer tensor to resolve to exactly one physical slice. All 19 weight arenas remain resident.
CPU reference: the exact P8-E synthetic hidden input is used. CPU layer 0 is computed from original GGUF bytes; CPU layer 1 consumes CPU layer-0 output. No GPU intermediate contributes to CPU expected values.
GPU composition: one prepared chain contains exactly 30 dispatches and is submitted once. Layer 0 writes gpu[0].out. Layer 1 is wired explicitly with layer1_input=&gpu[0].out and consumes that buffer directly; no host correction, CPU injection, intermediate resubmission, or layer loop is used to invoke execution. The shared per-layer builder contains exactly the frozen 15 P8-E operations and is called explicitly only for layers 0 and 1.
Evidence preservation: each layer has its own intermediate/checkpoint buffers. Shared K/V caches are sized for exactly two layer regions and use the actual layer index. The run records 17 checkpoints per layer, 34 total, including per-layer K/V rows and final output. Frozen numerical gates remain max_abs<=0.02, RMSE<=0.005 and finite values.
Provenance: runner verifies byte-exact P8-E parent artifacts, recompiles the same 11 frozen shaders with pinned glslang 16.5.0, and writes fresh P8-F provenance.
Decision: P8-F is READY_TO_RUN. No scientific verdict exists until target-machine JSON is returned. Build/compiler/environment/static-QA failures are infrastructure only. A layer-0 numerical failure is a P8-E regression; layer-0 PASS followed by a layer-1 first failure is the new cross-layer composition obstruction.
Next on PASS: P8-G larger bounded-prefix correctness design. Full 28-layer inference remains forbidden.


## 2026-09-19 — P8-F parent-evidence transport repair
Observed before P8-F runtime: static QA and run_p8f.ps1 both rejected the P8-E shader-provenance parent because the SHA256 of the checked-out .json working-tree file did not equal the already frozen authoritative raw SHA256 73916DE149A413B835541B95F01FEAF2B87DDDE03C039C3D1ABC0E2A3A115861. The user-supplied raw P8-E artifact itself still hashes exactly to that frozen value; the scientific parent evidence and P8-E verdict are unchanged.
Cause class: repository transport / working-tree byte preservation. JSON evidence paths are text files and are not a safe byte-exact transport boundary across Windows line-ending handling.
Repair: preserve the same authoritative Git blobs under new inputs/*.authoritative.raw paths and add .gitattributes with -text for that transport class. P8-F static QA, runner parent hashing, P8E_SHA256SUMS.txt and C++ parent inputs now use the .raw paths. The decoded/content semantics, frozen SHA256 values, P8-F contract, target, numerical gates, dispatch count, submit count and layer scope are unchanged.
Classification: package/provenance transport repair only. No P8-F scientific run occurred because execution stopped before static QA/runtime.
Decision: P8-F remains READY_TO_RUN under the original frozen contract.


## 2026-09-19 — P8-F parent shader-provenance byte repair
Root cause confirmed by byte-level comparison against the originally uploaded P8-E shader provenance. The authoritative GitHub shader blob was 5101 bytes, while the uploaded raw artifact is 5107 bytes. All lines matched except compiler_asset_sha256: the GitHub blob contained the truncated value 06B71298B750268C127F2EE7525E2068120C6C8A3A08B2F58CA6F325CE, missing exactly six characters AE0EF7. The original uploaded artifact contains 06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE and hashes to the already frozen SHA256 73916DE149A413B835541B95F01FEAF2B87DDDE03C039C3D1ABC0E2A3A115861.
Repair: replace both p8e_shader_provenance.authoritative.json and p8e_shader_provenance.authoritative.raw with the exact 5107-byte uploaded content. The resulting Git blob SHA is 8260be58506e38df2d51dd49529d289211bf1e4f, independently predicted from the uploaded bytes before the repository write. The P8-E result and summary raw blobs were checked and already matched their expected exact Git blob SHAs feba036e91c41a3238a766f5400e5105f65e79c6 and 5fd0a29b6d22aa89f1a8fb410c1dd5e9396cf023.
Also restore the pre-existing .gitattributes rules that were accidentally overwritten by the previous transport-repair commit, then append the authoritative.raw -text rule.
Classification: provenance/package repair only. No P8-F scientific run occurred; execution still stopped before runtime. P8-E scientific evidence, frozen SHA256 values, P8-F hypothesis, scope, numerical gates, 30-dispatch/one-submit contract and full-inference prohibition are unchanged.
Decision: P8-F remains READY_TO_RUN.


## 2026-09-19 — P8-F static parent-path assertion repair
Observed before runtime: test_p8f_package.py still expected the old P8-E parent filenames ending in .authoritative.json inside run_p8f.ps1, while the runner had already been intentionally switched to the byte-preserving .authoritative.raw paths. Parent hashes themselves were correct and the local raw shader artifact now verifies to the frozen SHA256.
Repair: update only the three stale runner-path assertions in static QA from .json to .raw.
Classification: static-QA packaging defect only. No P8-F source, contract, scientific gate, parent hash, execution scope, or runtime behavior changed.
Decision: P8-F remains READY_TO_RUN.


## 2026-09-19 — P8-F PASS / two-layer prefix correctness frozen
Authoritative evidence SHA256: shader provenance 9686F747B6A4D7380F4621B1A3EEC09B82DE7832461B1A9C80548D21D7D70A41; two-layer results F172E1B0B00558BDE53EB6994BDC1E5BFFC417F96A33FA1C53B0983BB494AE98; summary 1F3EF56BC723FE2D8D8E567ADCB89BC23AE212CB0BBBB51CBC5B83E81DF9FC70.
Result: status=PASS on the exact 7B target. Binding retained 19 arenas / 341 pieces / two globally segmented logical tensors, while all tensors used by layers 0 and 1 were single-piece. Direct GPU L0->L1 handoff was true. All 34 checkpoints were finite and PASS. Worst max_abs was L1.ffn_gate=0.00325441360474; worst RMSE was L0.k_rope=0.000176462817402. L1 final output max_abs=0.000379204750061 and RMSE=2.92844861807e-05. Execution was exactly 30 dispatches, one submit, one fence wait and exactly layers [0,1]. first_failing_checkpoint was empty.
Decision: P8-F is a genuine PASS and is FROZEN. This supports bounded cross-layer composition across one real decoder boundary; it does not establish full-model correctness.
Next: P8-G larger bounded prefix. Full 28-layer inference remains forbidden.

## 2026-09-19 — P8-G design frozen
Question: does the validated prefix remain correct when depth doubles from two layers to four layers while hidden states stay GPU-resident and unmodified?
Design: execute exactly layers [0,1,2,3], seq=4, same deterministic input, same graph binding, same 19 arenas / 341 pieces, same frozen kernels and same numerical gates. CPU reference computes the same four-layer prefix independently from original GGUF bytes. GPU execution uses direct handoff L0->L1->L2->L3. Record the same 17 checkpoints per layer, 68 total. Exact execution gate is 60 dispatches in one submit and exactly four executed layers.
Failure localization: L0/L1 failure is a P8-F regression; L0/L1 PASS followed by first failure in L2/L3 is the new deeper-prefix composition obstruction.
Exclusions: no embedding, output_norm, LM-head, decode, generation, layer 4+, or performance trial.
Decision: P8-G contract is DESIGN_FROZEN and implementation is permitted. It is not READY_TO_RUN until implementation plus static QA are committed.
Next on PASS: P8-H bounded eight-layer prefix correctness design. Full 28-layer inference remains forbidden.


## 2026-09-19 — P8-G implementation / READY_TO_RUN
Implementation: extended the frozen P8-F path mechanically from two to exactly four consecutive decoder layers without changing kernels, quantization, sequence length, graph binding, memory plan, numerical gates or submission model.
CPU reference: computes CPU L0 -> L1 -> L2 -> L3 sequentially from original GGUF bytes. GPU intermediates never construct CPU expected values.
GPU composition: the unchanged 15-operation per-layer builder is invoked explicitly for L0, L1, L2 and L3. Hidden-state inputs are wired directly as synthetic input -> gpu[0].out -> gpu[1].out -> gpu[2].out. No CPU correction, host replacement, intermediate re-upload or additional submit is introduced. The prepared chain must contain exactly 60 dispatches and execute in one submit.
Binding/residency: exact 339-tensor graph census, 19 weight arenas and 341 physical pieces are retained. Every executed-layer weight for L0..L3 must resolve to one physical slice. K/V caches allocate four layer regions and preserve rows 0..3 independently for each layer.
Evidence: each layer retains the same 17 checkpoints used by P8-E/P8-F, for 68 total. Gates remain finite values, max_abs<=0.02 and RMSE<=0.005. Any L0/L1 numerical failure is a P8-F regression; after L0/L1 PASS, first failure in L2/L3 is the new deeper-prefix composition obstruction.
Provenance: runner verifies byte-exact authoritative P8-F parent evidence, recompiles the same 11 frozen shaders with pinned glslang 16.5.0, and writes fresh P8-G provenance/results/summary.
Decision: P8-G is READY_TO_RUN. No P8-G scientific verdict exists until target-machine evidence is returned. Full 28-layer inference remains forbidden.
Next on PASS: P8-H bounded eight-layer prefix correctness design.


## 2026-09-19 — P8-G native-build cleanup-symbol repair
Observed before runtime: static QA and shader compilation passed, but MSVC failed in p8g_four_layer_prefix_correctness.cpp with C3861 because destroy_layer was called in the cleanup path without a local definition. The helper had been present in P8-F but was accidentally removed when the P8-G output/cleanup block was mechanically replaced.
Repair: restore the same bounded per-layer buffer-destruction lambda immediately before the reverse cleanup loop, and add static QA requiring the helper definition to precede its only invocation.
Classification: native-build/package defect only. No P8-G GPU dispatch occurred; therefore no scientific P8-G evidence or verdict exists. The frozen hypothesis, parent evidence, four-layer scope, 68 checkpoints, numerical gates, 60-dispatch/one-submit contract and full-inference prohibition are unchanged.
Decision: P8-G remains READY_TO_RUN.


## 2026-09-19 — P8-G genuine FAIL frozen
Authoritative evidence SHA256: shader provenance C258C76816BDE57CC9D56A3C73955019716A1F01637A11D23197129CC53EA1B9; four-layer results BDB7F4C1480CF960A89EBB29FACE58FC751BA8EF6AF76812F23A7C5ED0E94129; summary 0B1D4CF0355FBB7987217CC1DA8133BD81CA5332B37D5542E69A427A7C196BFE.
Execution validity: exact target, 19 arenas / 341 pieces, layers [0,1,2,3], direct GPU handoff, exact 60 dispatches / one submit, all K/V rows PASS. L0-L2 all checkpoints PASS. L3 remained PASS through swiglu. First failing checkpoint: L3.ffn_down with max_abs=0.0283279418945 > frozen 0.02 and RMSE=0.000364843778882 < 0.005. L3 final output also FAIL with max_abs=0.0276565551758. No gate is relaxed.
Format observation: L0-L2 ffn_down are Q6_K; L3 ffn_down is Q4_K. L3 attn_v is also Q4_K but v_proj PASS, so the evidence does not support a generic claim that all Q4_K execution is broken.
Decision: P8-G is a genuine FAIL and FROZEN. P8-H is blocked. The result localizes the first obstruction to the L3 FFN-down boundary but does not yet distinguish deeper-prefix error amplification from a P7-G Q4 tiled16 kernel-specific defect.

## 2026-09-19 — P8-G1 causal decomposition design frozen
Question: why does L3.ffn_down cross the frozen max_abs gate?
Pre-registered hypotheses: H-KERNEL (P7-G Q4 tiled16 implementation defect), H-AMPLIFICATION (upstream L3.swiglu perturbation amplified by a correct down operator), H-COMMON-Q4 (common Q4 decode/reference/binding issue), H-INTERACTION (neither factor alone fails, combination does).
Design: reproduce X_CPU and X_GPU at L3.swiglu, then compute exact-weight CPU and GPU down projections on both inputs. GPU A/B uses P7-G q4 tiled16 and P7-C q4 tiled8 with the same blk.3.ffn_down.weight and exact n=18944, rows=3584, batch=4 geometry. The two shaders share the same Q4_K dequantization logic but differ in tiling/shared-X execution structure, making this a controlled implementation discriminator.
Required outputs: Y_CC, Y_CG, Y_GC_P7G, Y_GG_P7G, Y_GC_P7C, Y_GG_P7C. Parent P8-G actual-path failure must reproduce. Frozen correctness gates remain max_abs<=0.02, RMSE<=0.005, finite values. Teacher-forced CPU input is diagnostic only and cannot revise the frozen P8-G verdict.
Decision: P8-G1 DESIGN_FROZEN / READY_FOR_IMPLEMENTATION. Target run is not yet permitted. P8-H and full inference remain forbidden.


## 2026-09-19 — P8-G1 implementation / READY_TO_RUN
Implementation follows the frozen causal-decomposition contract without shader or gate changes.
Stage 1 reconstructs the original direct GPU prefix through L3 SwiGLU only: L0-L2 execute the full 15-op decoder chain and L3 executes the first 13 ops through SwiGLU, for exactly 58 dispatches in one submit. Direct hidden-state handoff remains L0->L1->L2->L3. CPU teacher forcing is absent from this prefix stage.
Independent CPU reference computes the same four-layer prefix and retains X_CPU=L3 CPU SwiGLU. After Stage 1, X_GPU is read from the actual GPU L3 SwiGLU buffer. X_GPU vs X_CPU is required to reproduce the frozen P8-G L3.swiglu metrics.
Focus binding is blk.3.ffn_down.weight only. The executable requires Q4_K, n=18944, rows=3584, batch=4, one physical slice, and exact equality between the P8-D/P8-G arena mapping and the original GGUF tensor byte span.
CPU outputs are Y_CC=CPU_down(X_CPU) and Y_CG=CPU_down(X_GPU).
Stage 2 uses the exact same resident weight to execute exactly four GPU diagnostic dispatches in one separate submit: P7-G Q4_K tiled16 on X_CPU/X_GPU and P7-C Q4_K tiled8 on X_CPU/X_GPU. No weight conversion or re-encoding occurs.
Comparisons C0-C6 retain finite values, max_abs<=0.02 and RMSE<=0.005. C0 must reproduce the original P8-G L3.ffn_down failure; otherwise the diagnostic is INVALID and no causal adjudication is accepted.
Classification logic implements H-KERNEL, H-COMMON-Q4, H-AMPLIFICATION, H-INTERACTION, the separately named input-dependent tiled-kernel sensitivity case, INCONCLUSIVE, and PARENT_REPRODUCTION_FAILED exactly as bounded by the contract.
P8-G remains frozen FAIL regardless of P8-G1 classification. P8-H and full inference remain forbidden.
Decision: P8-G1 READY_TO_RUN. No P8-G1 causal verdict exists until target-machine JSON evidence is returned.


## 2026-09-19 — P8-G1 static source-literal assertion repair
Observed before build/runtime: test_p8g1_package.py searched for the rendered JSON fragment "first_failing_checkpoint":"L3.ffn_down" inside C++ source text. The runtime guard is correctly encoded as a C++ string literal with escaped quotes: pr.find("\\"first_failing_checkpoint\\":\\"L3.ffn_down\\""). Therefore the static test representation, not the implementation, was wrong.
Repair: change only that source-text assertion to match the escaped C++ literal. Parent P8-G JSON parsing/assertions remain unchanged and already verify first_failing_checkpoint == L3.ffn_down directly.
Classification: static-QA packaging defect only. No P8-G1 build or GPU diagnostic run occurred. Frozen P8-G1 contract, parent hashes, 58+4 dispatch design, numerical gates, causal hypotheses, P8-G FAIL verdict, P8-H block and full-inference prohibition are unchanged.
Decision: P8-G1 remains READY_TO_RUN.


## 2026-09-19 — P8-G1 COMPLETE / H-AMPLIFICATION frozen
Authoritative evidence SHA256: shader provenance A4B095E1F2E7CD4AD78EE74C06A96323DF258C2ADB2CCA7DE827816191019740; causal decomposition 416A97CD13A585BD4CAB397A3B3503BA8A87BD94F13BB1F5664F384603359499; summary 1E04C9BF94DA57258D9DEA36FAE3F71B896D9F0CA773BF360305742397C27B1C.
Validity: COMPLETE, diagnostic_valid=true, exact focus blk.3.ffn_down.weight Q4_K, one physical slice and exact source span, 19 arenas / 341 pieces, prefix 58 dispatches / one submit, diagnostic 4 dispatches / one submit, direct GPU handoff, no layer4+.
Parent reproduction passed: X_GPU vs X_CPU reproduced L3.swiglu max_abs=0.00838851928711 and RMSE=7.10637175468e-05; C0 reproduced L3.ffn_down max_abs=0.0283279418945 and RMSE=0.000364843778882.
Kernel controls falsified the tested kernel-defect explanations: C1 P7-G(X_CPU) and C2 P7-C(X_CPU) both PASS at max_abs=6.103515625e-05 / RMSE=7.36516371841e-07; C4 P7-G(X_GPU) and C5 P7-C(X_GPU) both PASS at max_abs=3.0517578125e-05 / RMSE=6.0782396936e-07. P7-G vs P7-C is bit-identical on both inputs (C6a=C6b=0).
C3 CPU_down(X_GPU) vs CPU_down(X_CPU) alone reproduces the failure: max_abs=0.0283355712891, RMSE=0.000364919435264. Therefore P8-G1 supports H-AMPLIFICATION. Approximate observed directional gain is 3.3779x in max_abs and 5.1351x in RMSE.
Decision: P8-G1 causal classification H-AMPLIFICATION is FROZEN. P8-G remains historically FAIL; verdict_changed=false. P8-H and full inference remain forbidden.

## 2026-09-19 — P8-G2 amplification geometry design frozen
Question: is the P8-G gate crossing quantitatively explained by linear propagation of the already-observed upstream perturbation through the exact L3 Q4_K FFN-down operator, with kernel residual negligible?
Design retains the exact P8-G1 prefix through L3 SwiGLU. It records ΔX, ΔY_prop=CPU_down(X_GPU)-CPU_down(X_CPU), directional max/RMS gain, reference signal scales and normalized errors. It separately computes CPU_down(ΔX) and tests linear propagation closure at max_abs<=1e-4 / RMSE<=1e-6.
Kernel residual ε_kernel=P7-G_down(X_GPU)-CPU_down(X_GPU) is compared with propagated error. Propagation dominance requires kernel max and RMS contribution ratios <=1%.
A frozen CPU-only scaling series α={0.25,0.50,0.75,1.00} applies the observed ΔX direction and records historical-gate crossings. max_abs/α and RMSE/α must remain within 1% of α=1 values for linearity qualification.
P8-G2 does not define a replacement gate and cannot change P8-G FAIL. Classifications are limited to H-GEOMETRIC-AMPLIFICATION, H-NONLINEAR/UNEXPLAINED, H-KERNEL-CONTRIBUTION or PARENT-REPRODUCTION-FAILED.
Decision: P8-G2 DESIGN_FROZEN / READY_FOR_IMPLEMENTATION. P8-H and full inference remain blocked.


## 2026-09-19 — P8-G2 implementation / READY_TO_RUN
Implementation follows the frozen amplification-geometry contract without changing shaders, model weights, sequence length, historical correctness gates or prior verdicts.
Prefix reconstruction is identical to P8-G1 through L3 SwiGLU: L0-L2 execute the full 15-op chain and L3 executes through SwiGLU, exactly 58 dispatches in one submit with direct L0->L1->L2->L3 GPU handoff and no CPU teacher forcing.
A0 reproduces both frozen parent observables: X_GPU vs X_CPU at L3 SwiGLU and C3 CPU_down(X_GPU) vs CPU_down(X_CPU), using max_abs tolerance 1e-6 and RMSE tolerance 1e-7.
A1 forms dX=X_GPU-X_CPU and dY_prop=CPU_down(X_GPU)-CPU_down(X_CPU), recording max/RMS directional gains, X_CPU/Y_CC signal scales and normalized perturbation/error ratios. These measurements have no new correctness threshold.
A2 independently computes dY_direct=CPU_down(dX) and applies the preregistered linear-closure criterion max_abs<=1e-4 / RMSE<=1e-6.
A3 executes exactly one additional production P7-G Q4_K tiled16 down-projection on X_GPU in one separate submit. Kernel residual Y_GG-Y_CG is compared with propagated error; both max and RMS contribution ratios must be <=1% for propagation dominance.
A4 runs the frozen CPU-only alpha series {0.25,0.50,0.75,1.00}. Historical 0.02/0.005 gate results are recorded descriptively. max_abs/alpha and RMSE/alpha must remain within 1% of the alpha=1 values for linearity qualification.
Classifications are exactly H-GEOMETRIC-AMPLIFICATION, H-NONLINEAR/UNEXPLAINED, H-KERNEL-CONTRIBUTION or PARENT-REPRODUCTION-FAILED.
P8-G remains frozen FAIL in all outcomes. P8-G1 remains frozen H-AMPLIFICATION. P8-G2 cannot define a replacement gate, permit P8-H, or permit full inference.
Decision: P8-G2 READY_TO_RUN. No P8-G2 scientific verdict exists until target-machine evidence is returned.


## 2026-09-20 — P8-G2 COMPLETE / H-NONLINEAR-UNEXPLAINED frozen
Authoritative evidence SHA256: shader provenance 43A8DEA2AD14FF516B7FDF3EB69EC3D807C455F84D53E67C7BBF00A20C4993F4; amplification geometry 2D183D80DD4A63CC75E10D2DC42BE08D7606B147A39FC15021E1D52E569CA168; summary B48FC0B48CC94363C24C0FD772058AC46C8BD3ED32203039F7FDEE230B0CB8A6.
Validity: COMPLETE, diagnostic_valid=true, A0 parent reproduction PASS, exact focus/binding PASS, prefix 58 dispatches / one submit, kernel-residual diagnostic one dispatch / one submit, direct GPU handoff, no layer4+.
A1 directional amplification: max=3.37789904502, RMS=5.13510196006. Normalized input/output perturbations remain approximately 4-5e-5 of reference signal scale.
A2 linear closure FAIL: max_abs=0.000383861362934 > 1e-4 and RMSE=5.5805988593e-06 > 1e-6. These are about 1.3547% and 1.5293% of propagated max/RMS error.
A3 propagation dominance PASS: production-kernel residual contributes 0.1077% of propagated max error and 0.1666% of propagated RMS error.
A4 alpha linearity FAIL narrowly. Relative to alpha=1 normalized ratios, alpha=0.25 differs 1.45396% in max_abs/alpha and 0.94511% in RMSE/alpha; alpha=0.50 differs 0.96931% / 0.80096%; alpha=0.75 differs 0.16155% / approximately 0.00172%. Only alpha=0.25 max_abs/alpha crosses the frozen 1% tolerance.
Decision: P8-G2 classification H-NONLINEAR/UNEXPLAINED is FROZEN under its preregistered rules. replacement_gate_defined=false. P8-G remains FAIL; P8-G1 remains H-AMPLIFICATION; P8-H and full inference remain forbidden.

## 2026-09-20 — P8-G3 arithmetic-precision attribution design frozen
Source audit: the frozen CPU Q4_K q4k_dot_row decodes weights to float, computes float products and sequentially accumulates 18,944 terms into float sum. Floating-point non-distributivity is therefore a prospective explanation for the P8-G2 A2/A4 misses; the P8-G2 label does not establish mathematical nonlinearity.
P8-G3 retains the exact prefix and observed X_CPU/X_GPU, then evaluates nested arithmetic regimes. R0 is the frozen FP32 path. R1 changes only accumulation to double while preserving float products, float input algebra and float output. R2 uses double product/accumulation/output while preserving R0 float dX and X_alpha. R3 additionally performs dX and alpha interpolation in double.
Every regime uses the unchanged P8-G2 closure criterion 1e-4/1e-6 and alpha normalized-ratio tolerance 1%. Closure and alpha each record their earliest passing regime. Overall classification distinguishes FP32 accumulation, broader dot rounding, input rounding, mixed finite precision, high-precision unexplained behavior, or parent reproduction failure.
Argmax output indices across alpha are descriptive only and add no gate.
Decision: P8-G3 DESIGN_FROZEN / READY_FOR_IMPLEMENTATION. No replacement correctness gate is defined. Historical P8-G/P8-G1/P8-G2 outcomes remain frozen. P8-H and full inference remain blocked.


## 2026-09-20 — P8-G3 implementation + static-QA lock / READY_TO_RUN
Implementation follows the frozen arithmetic-precision attribution contract without shader, weight, sequence-length, historical-gate or prior-verdict changes.
GPU scope is unchanged from P8-G2 through L3 SwiGLU only: exact 58 dispatches / one submit, direct L0->L1->L2->L3 handoff, no CPU teacher forcing and no layer4+ execution. P8-G3 adds no second GPU diagnostic chain; all precision controls are CPU-only.
To avoid repeated 18,944-term Q4_K decode work, one fused evaluator decodes each float Q4_K weight once per output coordinate and updates independent R0/R1/R2/R3 accumulators in the same k=0..18943 term order. Output coordinates may run across up to eight CPU worker threads; no cross-output reduction exists, so threading does not alter any per-dot accumulation order.
R0 exactly preserves float product, float sequential accumulator, float output and float dX/X_alpha algebra. R1 changes only accumulation to double while reusing the exact float product and casts final output to float. R2 uses double product/accumulation/output while preserving R0 float dX/X_alpha. R3 retains R2 dot precision and changes only dX/interpolation to double.
R0 is required to reproduce authoritative P8-G2 A2 plus all four A4 metrics within max_abs 1e-6, RMSE 1e-7 and normalized-ratio relative tolerance 0.1%; otherwise the diagnostic is INVALID.
Every regime is evaluated against the unchanged P8-G2 closure gate max_abs<=1e-4 / RMSE<=1e-6 and unchanged alpha normalized-ratio tolerance <=1%. Argmax absolute-error output coordinates are recorded descriptively and do not add a gate.
Closure and alpha each report earliest pass R1/R2/R3/NONE. Overall classifications are exactly H-FP32-ACCUMULATION, H-DOT-ROUNDING, H-INPUT-ROUNDING, H-MIXED-FINITE-PRECISION, H-HIGH-PRECISION-UNEXPLAINED, or PARENT-REPRODUCTION-FAILED.
Static QA locks exact parent hashes, R0-R3 arithmetic markers, 58/1 prefix scope, CPU-only precision controls, unchanged thresholds, governance fields, runner outputs, build sources and pinned 11-shader provenance.
No target experiment was run as part of this implementation/static-QA lock.
Decision: P8-G3 READY_TO_RUN only after this commit is pulled. P8-G remains FAIL; P8-G1 remains H-AMPLIFICATION; P8-G2 remains H-NONLINEAR/UNEXPLAINED; replacement gate remains undefined; P8-H and full inference remain forbidden.


## 2026-09-20 — P8-G3 COMPLETE / H-FP32-ACCUMULATION frozen
Authoritative evidence SHA256: shader provenance C10D0DB9444E584CDC76D939F5D134EC1229BD600CF3EED181C9D08505E955E8; arithmetic-precision result B0ADAAF9790018467C721599C2147AF7A6C0F69F0967B5C25F77631095571835; summary EF494284E7BFBED541380267EDB197CF53F9EBB7E86040A83546735F84209C4D.
Validity: COMPLETE, diagnostic_valid=true, exact focus/binding PASS, prefix 58 dispatches / one submit, direct GPU handoff, precision controls CPU-only, no layer4+.
R0 reproduced P8-G2 and retained closure FAIL / alpha FAIL. R1, which changes only sequential accumulation from float to double while preserving float products and float output, is the earliest regime to pass both closure and alpha-linearity. R1 closure max_abs=1.32623827084899e-05 and RMSE=2.76852946947452e-07. R2/R3 reduce closure to approximately 3.84e-12 max_abs and 4.9e-14 RMSE.
All alpha argmax indices remain 13322 across every regime and alpha. R1 normalized alpha ratios are stable within the frozen 1% criterion.
Decision: P8-G3 classification H-FP32-ACCUMULATION is FROZEN. Sequential FP32 accumulation is sufficient to explain the P8-G2 A2/A4 misses. This does not revise P8-G FAIL, P8-G1 H-AMPLIFICATION or P8-G2 H-NONLINEAR/UNEXPLAINED under their historical protocols. replacement_gate_defined=false. P8-H and full inference remain forbidden.

## 2026-09-20 — P8-G4 fresh-cohort compositional decomposition design frozen
Question: does the P8-G1 causal separation between propagated upstream state drift and same-input local FFN-down error replicate prospectively on fresh inputs after removing the P8-G3-identified FP32 oracle artifact?
Fresh cohort is fixed before target evidence: input IDs {17,29,43,61} with formula x_s[i]=0.13*sin((i+11+37*s)*0.009)+0.04*cos((i+5+19*s)*0.017)+0.02*sin((i+3+23*s)*0.0043). The original P8-G input is excluded.
Each seed executes only layers0..3 through L3 SwiGLU using the frozen 58-dispatch direct GPU prefix. The exact P8-G3 R1 arithmetic acts only as the CPU diagnostic oracle for L3 FFN-down. One production P7-G Q4_K tiled16 down dispatch on X_GPU yields the local GPU output.
Per seed: E_state=R1_down(X_GPU)-R1_down(X_CPU), E_local=GPU_down(X_GPU)-R1_down(X_GPU), E_total=GPU_down(X_GPU)-R1_down(X_CPU). Decomposition closure must pass 1e-5 / 1e-7. Same-input local error must pass historical 0.02 / 0.005. Prospective dominance requires local/state <=5% on both max and RMS.
Historical total-error pass/fail is recorded descriptively only and is excluded from P8-G4 adjudication.
Decision: P8-G4 DESIGN_FROZEN / READY_FOR_IMPLEMENTATION. No replacement correctness gate is defined. P8-H and full inference remain blocked.


## 2026-09-20 — P8-G4 implementation + static-QA lock / READY_TO_RUN
Implementation follows the frozen fresh-cohort compositional-decomposition contract without shader, model-weight, sequence-length, historical-gate or prior-verdict changes.
Fresh cohort is exactly IDs {17,29,43,61} with the preregistered trigonometric input formula. The original P8-G input formula is absent from the P8-G4 source and static QA forbids its reintroduction.
One frozen prefix chain is prepared once and reused for all four seeds. Per seed GPU scope is exactly L0-L2 full plus L3 through SwiGLU: 58 dispatches / one submit, direct GPU handoff, no CPU state injection and no layer4+. Input buffer memory is host-visible/coherent and receives each fresh x_s by memcpy before prefix execution.
CPU reference L0-L2 executes fully; L3 explicitly stops after SwiGLU. The inherited FP32 L3 down call is disabled to keep the CPU reference scope exact and avoid unnecessary 18,944x3,584 work.
The R1 oracle independently implements the P8-G3-qualified arithmetic on blk.3.ffn_down.weight: Q4_K weight decoded to float, product rounded as float, double sequential accumulator, final output cast to float. X_CPU and X_GPU are evaluated together while each Q4_K weight is decoded once. Up to eight worker threads split independent output coordinates only; within-dot k=0..18943 order is unchanged.
A second prepared chain contains exactly one production p7g_ffn_q4k_tiled16 down dispatch on the actual X_GPU buffer. Per seed it executes exactly one dispatch / one submit. No CPU teacher-forced vector is uploaded to this chain.
Per seed decomposition is E_state=Y_CG-Y_CC, E_local=Y_GG-Y_CG, E_total=Y_GG-Y_CC and E_reconstructed=E_state+E_local. D0 closure remains 1e-5 / 1e-7. D1 local same-input correctness uses historical 0.02 / 0.005. D2 preregistered mechanistic dominance requires local/state<=5% on both max and RMS.
D3 historical total-error 0.02/0.005 is recorded descriptively only. Static QA verifies seed_pass is exactly D0&&D1&&D2 and that D3 does not appear in the cohort classification branch.
Cohort classification is locked to H-COMPOSITIONAL-SEPARATION-REPLICATED, H-HETEROGENEOUS-SEPARATION, H-LOCAL-ERROR-NONNEGLIGIBLE, or DECOMPOSITION-INVALID. Structural/execution invariant failure or any D0 failure forces DECOMPOSITION-INVALID.
Static QA additionally locks exact P8-G3 parent hashes/semantics, fresh IDs/formula, build source/executable, pinned 11-shader provenance, one prefix + one local GPU execute call site, prepared-chain reuse, governance fields and no layer4+/embedding/LM-head/decode path.
No fresh-cohort target experiment was run as part of this implementation/static-QA lock.
Decision: P8-G4 READY_TO_RUN only after this commit is pulled. P8-G/P8-G1/P8-G2/P8-G3 historical outcomes remain frozen. replacement_gate_defined=false. P8-H and full inference remain forbidden.


## 2026-09-20 — P8-G4 COMPLETE / H-LOCAL-ERROR-NONNEGLIGIBLE frozen
Authoritative evidence SHA256: shader provenance 1C146FCDD60D14A782512687865AC403D6DEE5C72FC125F8A74C5D4A920D5C7C; fresh compositional result 9255B70BA50DF316B7D8BA04CB6696DA9FB2CB97D82F7A559DCF166F7E76E757; summary 6347545710333A3CF9856399DF040E57A6C140E3463C1E50E5A8A432AF82FFD0.
Validity: COMPLETE, diagnostic_valid=true, exact focus/binding PASS, fresh IDs {17,29,43,61}, all per-seed prefix executions 58/1 and local-down executions 1/1, no layer4+.
All 4/4 D0 decomposition closures PASS and all 4/4 D1 same-input historical local gates PASS. D2 fails 4/4: seed17 local/state=0.04673 max / 0.053996 RMS; seed29=0.37705 / 0.17528; seed43=0.03292 / 0.06402; seed61=0.06180 / 0.05231. Therefore preregistered P8-G4 cohort verdict is H-LOCAL-ERROR-NONNEGLIGIBLE with seed_pass_count=0.
D3 historical total-error outcomes remain descriptive only: seeds 29 and 61 pass, 17 and 43 fail. They do not participate in adjudication.
Decision: P8-G4 H-LOCAL-ERROR-NONNEGLIGIBLE is FROZEN. The 5% D2 threshold is not changed. Prior P8-G/P8-G1/P8-G2/P8-G3 outcomes remain frozen. replacement_gate_defined=false. P8-H and full inference remain forbidden.

## 2026-09-20 — P8-G5 production-semantic local-error attribution design frozen
Source audit of p7g_ffn_q4k_tiled16.comp shows production down arithmetic uses float dequantized weights, float products and float sequential accumulators acc_a/acc_b, traversing k in order via k0+=32 and kk=0..31. This is semantically aligned with R0 rather than the R1 double-accumulator oracle used in P8-G4.
Therefore P8-G4 E_local_R1=GPU-R1 potentially mixes actual production residual with the R0->R1 oracle precision shift.
P8-G5 replays the exact frozen P8-G4 cohort only for retrospective causal attribution. It computes R0 and R1 down outputs for X_CPU/X_GPU and production GPU down(X_GPU), then decomposes E_local_R1 into E_prod=GPU-R0 and E_oracle=R0-R1.
P0 must reproduce P8-G4 R1 state/local metrics and four D2 failures. P1 requires exact local-term reconstruction under 1e-5/1e-7. P2 prospectively requires production-matched GPU-vs-R0 residual <=1e-4 max_abs and <=1e-6 RMSE on every seed. P3 uses the unchanged 5% P8-G4 dominance threshold on the R0 production-semantic decomposition.
Classification is locked to H-R1-ORACLE-SEMANTIC-MISMATCH, H-RATIO-SENSITIVITY, H-PRODUCTION-LOCAL-RESIDUAL, or ATTRIBUTION-INVALID.
Decision: P8-G5 DESIGN_FROZEN / READY_FOR_IMPLEMENTATION. No threshold relaxation and no replacement correctness gate. P8-H and full inference remain blocked.


## 2026-09-20 — P8-G5 implementation + static-QA lock / READY_TO_RUN
Implementation follows the frozen production-semantic local-error attribution contract without shader, model-weight, cohort, historical-threshold or prior-verdict changes.
The exact P8-G4 cohort IDs {17,29,43,61} and input formula are reused. This is retrospective attribution only; it is not fresh confirmation and cannot establish generalization.
GPU execution is unchanged from P8-G4: one prepared 58-dispatch prefix / one submit per seed through L3 SwiGLU, followed by one prepared production P7-G Q4_K tiled16 down dispatch / one submit on X_GPU. No CPU teacher forcing, no layer4+, decode, generation or performance path is introduced.
A joint CPU oracle decodes each focus Q4_K weight once and updates R0 and R1 outputs for both X_CPU and X_GPU in the exact k=0..18943 order. R0 uses float product + float accumulator + float output. R1 uses the same float product but double accumulator + float output. Up to eight threads split independent output coordinates only.
P0 hard-codes the authoritative P8-G4 R1 E_state/E_local max/RMS and local/state ratios for all four seeds. Reproduction tolerances are max_abs metric 1e-6, RMSE metric 1e-7 and ratio relative tolerance 0.1%; all four R1 D2 outcomes must reproduce FAIL.
P1 decomposes the P8-G4 local term as E_prod=GPU-R0 plus E_oracle=R0-R1 and requires reconstruction closure <=1e-5 max_abs / <=1e-7 RMSE.
P2 compares GPU down(X_GPU) against production-matched R0 down(X_GPU) under the preregistered diagnostic gate <=1e-4 max_abs / <=1e-6 RMSE.
P3 recomputes state/local/total under R0 semantics, requires closure <=1e-5 / <=1e-7 and applies the unchanged P8-G4 5% local/state threshold on both max and RMS.
Classification priority is statically locked: invalid structural/P0/P1/P3-closure -> ATTRIBUTION-INVALID; any P2 fail -> H-PRODUCTION-LOCAL-RESIDUAL; otherwise all P3 dominance pass -> H-R1-ORACLE-SEMANTIC-MISMATCH; else H-RATIO-SENSITIVITY.
Static QA locks exact parent hashes/semantics, cohort/formula reuse, R0/R1 arithmetic markers, P0 constants/tolerances, P1/P2/P3 gates, classification order, two GPU execute call-sites, pinned 11-shader provenance, build source/executable and immutable governance.
No P8-G5 target attribution experiment was run as part of this implementation/static-QA lock.
Decision: P8-G5 READY_TO_RUN only after this commit is pulled. Historical P8-G/P8-G1/P8-G2/P8-G3/P8-G4 outcomes remain frozen. replacement_gate_defined=false. P8-H and full inference remain forbidden.


## 2026-09-20 — P8-G5 COMPLETE / H-R1-ORACLE-SEMANTIC-MISMATCH frozen
Authoritative evidence SHA256: shader provenance 6483D82540EC31F3CE058B51FC48C9BABFED9983F7F59A090D9AE14917176F9A; production-semantic attribution 64565C94AD2D9EF85D1DFF8CE972FD263C2C1B4A28A28B5F6830F6EB9AD6E096; summary 1732663E0A9CF90848D54487E2C2D031EB1A5C2A90808A34ABB1C06D57995751.
Validity: COMPLETE, diagnostic_valid=true, exact target/focus/binding PASS, retrospective cohort {17,29,43,61}, all executions 58/1 prefix + 1/1 local-down, no layer4+.
P0 reproduces the frozen P8-G4 R1 state/local metrics and all four D2_R1 failures. P1 reconstruction passes exactly on all four seeds. P2 production GPU vs R0 passes all four seeds under 1e-4 / 1e-6. P3 R0 decomposition closure and unchanged 5% dominance pass all four seeds.
Production residuals are only 3.0517578125e-05 to 6.103515625e-05 max_abs and 4.70945215263808e-07 to 8.94539287055447e-07 RMS. The R0->R1 oracle shift is much larger and explains the P8-G4 local term.
Under R0 semantics local/state ratios are seed17 0.001899/0.002070, seed29 0.007905/0.004534, seed43 0.002703/0.003013, seed61 0.001866/0.001863, all far below 0.05.
Decision: P8-G5 H-R1-ORACLE-SEMANTIC-MISMATCH is FROZEN. P8-G4 remains H-LOCAL-ERROR-NONNEGLIGIBLE under its historical R1-based protocol; no historical verdict is rewritten. replacement_gate_defined=false. P8-H and full inference remain forbidden.

## 2026-09-20 — P8-G6 fresh production-semantic separation confirmation design frozen
P8-G5 is retrospective attribution only, so a fresh prospective confirmation is required before any correctness-metric qualification.
Fresh cohort is fixed before target evidence: IDs {73,89,107,131}, disjoint from P8-G4/P8-G5 {17,29,43,61}, using the same deterministic input family.
Primary comparator is R0 production-semantic arithmetic: float Q4_K decode, float product, sequential float accumulator, float output. R1 remains a side diagnostic only and is excluded from adjudication.
Per fresh seed GPU scope remains 58-dispatch direct prefix / one submit through L3 SwiGLU plus one production local-down dispatch / one submit. No layer4+.
C1 requires GPU-vs-R0 <=1e-4 max_abs / <=1e-6 RMSE. C2 requires R0 decomposition closure <=1e-5 / <=1e-7. C3 retains the unchanged 5% local/state criterion on both max and RMS. No denominator flooring or threshold relaxation is permitted.
Classification is locked to H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED, H-FRESH-RATIO-SENSITIVITY, H-FRESH-PRODUCTION-LOCAL-RESIDUAL, or CONFIRMATION-INVALID.
Decision: P8-G6 DESIGN_FROZEN / READY_FOR_IMPLEMENTATION. No replacement correctness gate. P8-H and full inference remain blocked.


## 2026-09-20 — P8-G6 implementation + static-QA lock / READY_TO_RUN
Implementation follows the frozen fresh production-semantic confirmation contract. The only scientific variable changed from the P8-G5 production-semantic decomposition is the confirmatory cohort.
Executable fresh IDs are exactly {73,89,107,131}. Prior IDs {17,29,43,61} appear only as provenance metadata and are statically forbidden as the executable seed array.
Per seed GPU scope is unchanged: one prepared 58-dispatch prefix / one submit through L3 SwiGLU, direct GPU handoff, followed by one production P7-G Q4_K tiled16 local-down dispatch / one separate submit. No CPU teacher forcing and no layer4+.
The joint CPU oracle reuses the P8-G5 arithmetic: each Q4_K weight is decoded once; R0 uses float product + sequential float accumulator + float output; R1 uses the same float product + double accumulator + float output. Within-dot order remains k=0..18943.
R0 is the primary comparator. C1 compares GPU down(X_GPU) against R0 under 1e-4 / 1e-6. C2 verifies R0 state+local decomposition under 1e-5 / 1e-7. C3 applies the unchanged 5% local/state criterion on max and RMS, without denominator flooring or post-hoc normalization.
R1 state/local and R0->R1 shifts are descriptive-only. Static QA excludes R1 from C1/C2/C3 and classification. Per the frozen contract, finite R1 output is still checked in C0 as a structural diagnostic validity requirement.
Descriptive outputs also include signal-normalized R0 errors, max-error coordinates and historical total 0.02/0.005 pass/fail; none can influence classification.
Classification priority is statically locked: structural/C0/C2 failure -> CONFIRMATION-INVALID; any C1 fail -> H-FRESH-PRODUCTION-LOCAL-RESIDUAL; otherwise all C3 pass -> H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED; else H-FRESH-RATIO-SENSITIVITY.
Static QA locks exact P8-G5 parent hashes/semantics, fresh-cohort disjointness, unchanged input formula, R0/R1 arithmetic markers, C1/C2/C3 gates, R1 exclusion from decision metrics, two GPU execute call-sites, pinned 11-shader provenance, build source/executable and immutable governance.
No P8-G6 target experiment was run as part of this implementation/static-QA lock.
Decision: P8-G6 READY_TO_RUN only after this commit is pulled. Historical P8-G through P8-G5 outcomes remain frozen. replacement_gate_defined=false. P8-H and full inference remain forbidden.


## 2026-09-20 — ArcLLM research-convergence governance activated
Added docs/ARC_LLM_RESEARCH_GOVERNANCE.md as the project-level research governance with priority above future P8/P9 roadmaps and phase plans.
Central objective is now mandatory convergence to a falsifiable final verdict through Q1 real-model end-to-end feasibility, Q2 frozen performance/resource envelope, Q3 matched-baseline regime advantage, fresh reproduction and final adjudication.
New architecture experiments require all six convergence conditions: real-model execution, end-to-end identified bottleneck, direct Q1/Q2/Q3 blockage, explicit causal hypothesis, falsification condition and stop condition.
Phase proliferation, outcome-driven tuning, weak baselines and microbenchmark substitution for real-model evidence are explicitly prohibited.
Stop rules STOP-A through STOP-D and final verdict set {FEASIBILITY_NOT_ESTABLISHED, FEASIBLE_NO_DEMONSTRATED_ADVANTAGE, REGIME_ADVANTAGE_SUPPORTED, UNRESOLVED} are now project governance.
Transition rule: the already frozen P8-G6 contract/metrics remain immutable and P8-G6 is still adjudicated under experiment-lock commit 7da34af2241091459b50905bfc008c7c6ba623ef. Governance applies to all planning after P8-G6 adjudication and does not retroactively rewrite historical P8 evidence.
No source, shader, runner, static test or manifest content changed in this governance commit.


## 2026-09-20 — Deferred-investigation governance added
Added an append-only deferred-investigation mechanism to prevent technically interesting findings from expanding the ArcLLM research tree before the central validation converges.
New registry: docs/ARC_LLM_DEFERRED_INVESTIGATIONS.md.
Default rule: a finding that does not directly block Q1/Q2/Q3 or invalidate active evidence is recorded as DEFERRED and is not investigated deeply, benchmarked further, or promoted into a new phase/study during the main validation path.
Early promotion is allowed only when new evidence establishes a direct Q1/Q2/Q3 blocker, a frozen-evidence integrity problem, or an implementation/measurement defect that prevents correct execution of a frozen study. Promotion requires explicit evidence, causal hypothesis, falsification and stop condition.
After final adjudication, the full registry is reviewed for impact before any additional investigation budget is spent. Dispositions are ARCHIVE_NO_ACTION, ENGINEERING_FOLLOWUP, DEFER_FURTHER or RESEARCH_REOPEN_CANDIDATE. RESEARCH_REOPEN_CANDIDATE still requires a separately frozen study before execution.
No P8-G6 source, shader, runner, static test, manifest, metrics, threshold or target-run permission changed.


## 2026-09-20 — P8-G6 COMPLETE / H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED frozen
Authoritative byte-exact evidence SHA256: shader provenance 1490475D0D7EAA0498FEEA5CD0A37460C4881FFFF676A7C912E0E113E2CAAC84; fresh production-semantic result 0526D3F1400080AF6B68D1D44CBD2AF897671B727EA924EC0D0C953C16FDD259; summary 13C36E5EB14D08F60C3DC9277F7A21EE4033DB50D84806839DE62B3E9F7EE303.
P8-G6 status COMPLETE, diagnostic_valid=true, structural_valid=true. Fresh cohort {73,89,107,131} is disjoint from {17,29,43,61}; R0 remains primary and R1 descriptive-only.
All C0/C1/C2/C3 gates PASS 4/4. Per-seed production local R0 max_abs is 3.0517578125e-05 to 6.103515625e-05 and RMSE is 6.0679760680854e-07 to 9.11221665979798e-07, all within frozen 1e-4/1e-6. R0 decomposition closure is exact in the recorded metrics. Fresh local/state ratios are all far below 5%.
Decision: P8-G6 classification H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED is FROZEN. Historical P8-G through P8-G5 outcomes remain unchanged.

## 2026-09-20 — Post-P8-G6 governance transition to Q1
The pre-governance P8-G6 contract suggested a separate correctness-metric qualification after a positive confirmation. That future-planning suggestion is superseded by the subsequently activated project governance; P8-G6 evidence and classification are not changed.
Governance requires immediate return to Q1 -> Q2 -> Q3. P8-G6 has resolved the bounded L3 FFN-down production-semantic blocker and does not justify another subsystem micro/metric study before real-model execution.
Frozen Q1 contract: docs/P8_Q1_END_TO_END_CONTRACT.md.
Q1 execution boundary is pretokenized token IDs through full 28-layer model and four autoregressive greedy decode tokens. Tokenizer/API is excluded to avoid adding an unrelated P9 dependency.
Frozen input token IDs are [1,133151,133152,152062], deliberately crossing the segmented embedding boundary. Two independent reset executions A/B are required.
Inherited P7 production graph census is frozen as 441 prefill dispatches and 469 dispatches per cached decode step. Full segmented embedding/output, all 28 layers, GPU-resident KV, final norm, full 152064 logits and greedy feedback must execute with no CPU model-math fallback or teacher forcing.
Decision: Q1 DESIGN_FROZEN / READY_FOR_IMPLEMENTATION. No Q1 target run is permitted until implementation + static QA are locked. On Q1 PASS, next is Q2 matched performance/resource benchmark; no further subsystem study is allowed unless Q1 identifies a direct blocker under governance.


## 2026-09-20 — Q1 pre-execution clarification v0.1a + implementation lock
No Q1 target evidence existed at this point.
Clarification 1: prefill logits already predict token position 4; four cached-decode inputs at positions 4,5,6,7 predict positions 5,6,7,8. Therefore the frozen deterministic sequence contains five generated token IDs total, not four. Input IDs, decode-step count and 441/469 dispatch census are unchanged.
Clarification 2: Q1 records requested residency and requires it to remain inside the P8-B proven weight/KV/working envelope and the 15.25 GiB usable budget. The inherited Vulkan wrapper does not expose aggregate VkMemoryRequirements allocation bytes, so no new unmeasurable post-hoc allocation gate is invented.
Implementation composes the P8 segmented resolver/endpoints with the frozen P7 production prefill/decode graph. One new integration shader computes chunked full-vocabulary segmented Q6_K output rows; no weight re-encoding or merged >256 MiB allocation is introduced.
Static review locks: exact 7B target; [1,133151,133152,152062] input; 28 layers; 441 prefill dispatches; four 469-dispatch decode steps; full 152064 logits; greedy feedback; A/B reset repeat; no CPU model-math fallback; no teacher forcing; exact P8-G6 parent hashes; F3 evidence packaging before final classification.
Decision: Q1 IMPLEMENTATION_LOCKED / TARGET RUN AUTHORIZED. Q2 and Q3 remain closed.


## 2026-09-20 — Q1 Windows BuildOnly audit
BuildOnly workflow was added as infrastructure QA only; it does not execute the frozen 7B model and is not Q1 scientific evidence.
Run #1 / workflow run 35484999941: FAIL at static package contract before shader compile or native build. Root cause was a test-only source-representation assertion: the Python test searched for unescaped JSON text while the C++ source necessarily stores escaped quotes inside a string literal. No executable semantics, model contract, shader or gate failed.
Bounded repair: commit 1e95f7def19c3ca6bf486d36d4fe756a13076b48 changes only the two affected static assertions.
Run #2 / workflow run 35485109178 on commit 1e95f7def19c3ca6bf486d36d4fe756a13076b48: PASS.
Passed steps: static package contract; pinned 16-shader compile; native Windows C++ build; executable existence.
BuildOnly artifact: id 10597263511, digest sha256:77eeb69b9d3f076f40287f7722680aba254280c0521f252cc6739d492f3005cc.
No target model was downloaded or executed in BuildOnly.
Decision: Q1 implementation/package is TARGET-RUN READY. Q2/Q3 remain closed; next evidence must be the exact Q1 end-to-end target run.


## 2026-09-20 — Q1 parent-evidence packaging repair before target execution
The first local Q1 invocation at HEAD faa7e4d450a3774ce9a0281ad026d4fef855dcb4 stopped fail-closed before shader compilation/native model execution with "Q1 parent result SHA mismatch".
This is not Q1 scientific evidence and not Q1_EXECUTION_NOT_ESTABLISHED: the model path was resolved, but target execution never started.
Audit against the original user-uploaded P8-G6 artifacts found:
- shader provenance original SHA256 = 1490475D0D7EAA0498FEEA5CD0A37460C4881FFFF676A7C912E0E113E2CAAC84, repo blob already correct;
- summary original SHA256 = 13C36E5EB14D08F60C3DC9277F7A21EE4033DB50D84806839DE62B3E9F7EE303, repo blob already correct;
- fresh production-semantic result original SHA256 = 0526D3F1400080AF6B68D1D44CBD2AF897671B727EA924EC0D0C953C16FDD259, while the committed raw result bytes were not byte-identical.
Repair is evidence-packaging only: replace inputs/p8g6_fresh_production_semantic_results.authoritative.raw with the exact original uploaded bytes. Q1 source, shader, graph, input, dispatch census, gates and contract are unchanged.
Static QA is strengthened to recompute SHA256 of all three authoritative P8-G6 inputs before BuildOnly/target execution.
Decision: rerun Windows BuildOnly after this repair. Target execution remains blocked until BuildOnly PASS on the repaired commit.


## 2026-09-20 — Q1 parent-evidence repair corrected to exact original Git blob
The first repair commit 5590eb8a4d0413f4e4ff1a892393ee15b3bd87e0 still failed BuildOnly because its replacement result blob was not byte-identical to the original upload. No target model execution occurred.
The original uploaded p8g6_fresh_production_semantic_results.json was re-read directly from conversation file storage and reconstructed with its required trailing LF. Independent Git-blob calculation on the original bytes gives:
- original file bytes = 5987;
- SHA256 = 0526D3F1400080AF6B68D1D44CBD2AF897671B727EA924EC0D0C953C16FDD259;
- Git blob SHA1 = 0124c3378614c3e0f4cef1a58fa43b90217d343c.
The corrected repository blob is exactly 0124c3378614c3e0f4cef1a58fa43b90217d343c.
Shader and summary repository blobs already matched the original uploaded Git blobs and require no byte change.
Decision: rerun BuildOnly with SHA256 guards active. Target model execution remains blocked until PASS.


## 2026-09-20 — Q1 target-run authorization after exact parent repair
The corrected P8-G6 result blob is exact: Git blob 0124c3378614c3e0f4cef1a58fa43b90217d343c, SHA256 0526D3F1400080AF6B68D1D44CBD2AF897671B727EA924EC0D0C953C16FDD259, 5987 bytes.
A connector-side equivalent audit of every static-package assertion passes on commit c769dcc937f857085dc7dd62da44de45af539266, including all parent blobs and Q1 graph/gate invariants.
GitHub BuildOnly run 35486456429 failed twice before exposing any job step or log; this is treated as GitHub runner/infrastructure failure, not package evidence.
The last successful Windows BuildOnly remains run 35485109178 on commit 1e95f7def19c3ca6bf486d36d4fe756a13076b48. Compare from that commit to c769dcc937f857085dc7dd62da44de45af539266 changes only README.md, lineage.md, manifest.json, tests/test_q1_package.py, and the repaired P8-G6 raw result. Q1 source, shaders, shader compiler script, native build script and run_q1.ps1 are unchanged.
The local Q1 runner itself still fail-closes through static package QA, exact parent SHA checks, pinned 16-shader compilation and native Windows build before the model executable is launched.
Decision: Q1 TARGET-RUN READY again. Q2/Q3 remain closed.


## 2026-09-20 — Q1 COMPLETE / Q1_FEASIBILITY_ESTABLISHED
Authoritative Q1 target execution commit: ec83bf42727f799e31d3900a7545e2642b3b90eb.
Exact target SHA256 60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463, 4,683,074,048 bytes.
F0 PASS A/B; F1 PASS A/B; F2 exact repeat PASS; F3 evidence complete PASS.
Both independent executions generated [128275,128301,128275,128301,128275]. Per-step logits hashes and final normalized hidden hashes are also identical across A/B.
Frozen production census was observed exactly: 441 prefill dispatches and 469 dispatches for each of four cached decode steps. No CPU model-math fallback and no teacher forcing.
Binding/residency: 19 arenas, 341 pieces, two segmented logical tensors; requested residency 5,149,055,000 bytes inside the frozen P8-B envelope and usable budget.
Authoritative returned Q1 archive SHA256: DFB3E86C4F51D06290A2AE1ED1479C96F35AE0D329CFA5E2B7D50289DC641B43.
Raw result SHA256 FAD892B25C3A82F62C0CC8060F413792B8B0B73F78A7C3B4CF969E19753E63FA; final summary SHA256 36BFB413BCE31A1A6E2B77F96071162809B48FC7F566D61B34C779D90388AC95; evidence manifest SHA256 89B22986B6BF5F4776413BD13D0F3CDAEF8B13B9EAA1B11000D9027F0F8F9548.
Decision: Q1 CLOSED as Q1_FEASIBILITY_ESTABLISHED. Q1 timings remain descriptive and are prohibited from serving as Q2/Q3 performance evidence.

## 2026-09-20 — Q2 matched benchmark DESIGN_FROZEN
Q2 is opened directly by governance after Q1 PASS; no subsystem phase is inserted.
Matched baseline frozen to ggml-org/llama.cpp release v0.4.1, commit 391fac16460f15233a7740550d858ac96df3419d, Vulkan, exact same GGUF bytes, raw token input, F32 KV, 8 CPU threads, context 4096, batch/ubatch 256, all layers requested for GPU offload.
Two workloads are frozen: W-S prompt 4 / output 32; W-C prompt 256 / output 32 with deterministic raw-token formula. Each system x workload cell uses one warmup and five measured attempts.
Q2 collects TTFT, decode throughput, end-to-end latency, RAM/CPU resource metrics, GPU process memory/utilization where valid Windows counters are available, stability/error rate, plus ArcLLM architecture movement counters.
Q2 is characterization only and cannot declare advantage. Q3 remains blocked.
Decision: Q2 DESIGN_FROZEN / READY_FOR_IMPLEMENTATION. No Q2 measurement may run before implementation + baseline qualification + static QA lock.


## 2026-09-20 — Q2 implementation candidate / measurement still blocked
Implemented the frozen Q2 matched-characterization design without executing the target model.
ArcLLM harness generalizes only prompt extent (4/256), output count (32) and measurement instrumentation while retaining the Q1 production graph, 441/469 dispatch census, all 28 layers, segmented endpoints and GPU-resident FP32 KV.
Pinned llama.cpp baseline adapter uses raw token IDs directly through llama_batch_get_one/llama_decode at v0.4.1 commit 391fac16460f15233a7740550d858ac96df3419d; tokenizer/chat paths are absent. Frozen context is 4096, batch/ubatch 256, threads 8, F32 K/V, n_gpu_layers=-1, greedy, no EOS stop/speculative decode.
Windows resource sampler records working set/private bytes and CPU from Win32 APIs at 100 ms target cadence, and conditionally attempts GPU Engine/GPU Process Memory counters with explicit unavailable/error evidence.
Runner hard-locks cell order Arc W-S -> baseline W-S -> baseline W-C -> Arc W-C, one warmup + five measured attempts each, and verifies exact model/Q1 archive/hardware/driver/baseline executable before measurements.
Summarizer emits median/min/max/MAD and descriptive ratios only; it cannot declare a winner or open Q3.
Decision: Q2 remains IMPLEMENTATION_CANDIDATE. No Q2 measured attempt is authorized until Windows BuildOnly + pinned baseline build/API qualification PASS.


## 2026-09-20 — Q2 implementation STATIC_LOCK / local zero-measurement preflight required
Three Q2 GitHub Actions runs (35487793657, 35487864134, 35487956962) terminated before exposing any job step. They provide no compile/package evidence and are classified as infrastructure failures, not scientific or implementation failures.
Q2 package was therefore hardened for an authoritative target-local preflight instead of treating absent CI as PASS.
Baseline adapter now supports --qualify-only. It loads the exact 7B model and frozen F32-KV context, captures llama.cpp runtime logs, requires Vulkan evidence and the upstream "offloaded X/X layers to GPU" report, materializes exact W-S/W-C raw-token prompts, and exits before any llama_decode call. Qualification evidence explicitly records decode_executed=false and measured_attempts=0.
Pinned llama.cpp source/API compatibility was independently checked at commit 391fac16460f15233a7740550d858ac96df3419d: llama.h blob 3ab935939c6d183e5862aba93f346691e658403b, llama-model.cpp blob 3b2536283c5712de811f82cdef6390f7499b95d4, upstream Vulkan workflow blob 21d2a773531f81849a430880b18ce6f27dfb73fa. Required raw batch/decode/F32-KV/memory-clear/greedy/GPU-offload APIs and full-offload log are present; upstream Windows Vulkan SDK is 1.4.357.0.
Baseline build qualification script now pins/bootstraps Vulkan SDK 1.4.357.0, exact tag+commit, clean source, Visual Studio 2022 x64 build and executable SHA.
GPU sampler now accepts both Windows GPU Engine Compute and 3D instances and resource parser handles UTF-8 BOM safely.
run_q2_preflight.ps1 now performs exact-model/static/shader/ArcLLM-build/baseline-build/runtime-qualification checks and writes a SHA-bound q2_preflight_lock.json with zero measurements. run_q2.ps1 refuses all 20 attempts unless that lock matches HEAD, critical-file hashes, model bytes and baseline executable, and both W-S/W-C runtime qualifications are full-offload PASS.
Decision: IMPLEMENTATION_STATIC_LOCKED. Local preflight is permitted; Q2 measured run remains forbidden. Q3 remains blocked.


## 2026-09-20 — Q2 local preflight tooling hardening
Baseline qualifier now resolves CMake from PATH or the installed Visual Studio 2022 CMake bundle via vswhere. This is packaging/tooling hardening only; baseline pin, Vulkan backend, workloads, timing definitions, gates and Q2 authorization are unchanged. No target model or measured attempt was executed.


## 2026-09-20 — Q2 independent audit correction before target-local preflight
Independent re-audit of static-lock commit df33d95bbe32a3cf43fd9eb05f831ddec2f324cb found measurement authorization and artifact-binding gaps: successful preflight alone could reach the 20-attempt runner, and the runner rebuilt ArcLLM/shaders without proving the measured binaries matched preflight hashes. It also found a baseline timing asymmetry where logits were scanned for finiteness and then copied/scanned again by llama_sampler_sample, while ArcLLM used one direct full-vocabulary scan.
Correction: measurement now requires a separate committed config/q2_execution_authorization.json after independent preflight adjudication; runner accepts a later governance-only commit only when every implementation-critical hash remains preflight-identical. Runner no longer rebuilds ArcLLM/shaders and instead verifies exact Arc executable, baseline executable, shader provenance, 16 source hashes and 16 SPIR-V hashes.
Baseline greedy selection now uses one direct finite+argmax pass over llama.cpp logits, preserving greedy semantics while removing the extra host copy/second scan from primary timing. Target-local preflight additionally binds exact raw-prompt hashes, OS build, power scheme, AC state, and complete implementation dependencies.
GPU counter probing now tolerates either Compute or 3D availability without summing aliased classes. Vulkan SDK 1.4.357.0 bootstrap now verifies the official Windows x64 installer SHA256 before execution.
Decision: static package repaired; local preflight may be attempted only after this commit is pulled. Twenty measured attempts remain blocked. Q3 remains closed.


## 2026-09-20 — Q2 authorization-state QA correction
Post-commit QA found that the static test itself hard-coded the pre-authorization manifest state. That would have forced a test modification after target-local preflight, invalidating the implementation-critical SHA set that preflight was meant to freeze.
Correction: static QA now accepts exactly two governed states without code changes: IMPLEMENTATION_STATIC_LOCKED with no authorization file, or Q2_MEASUREMENT_AUTHORIZED with a committed valid authorization file and zero prior measurements. run_q2.ps1 additionally requires the manifest to be in the explicit authorized state.
Decision: this correction is implementation-critical and therefore must precede local preflight. No target model or measured attempt was executed.


## 2026-09-20 — Q2 host-postprocess and evidence-chain finalization
Final static review tightened the matched host postprocess: llama.cpp now performs the same one-pass finite + top-2 scan shape used by ArcLLM Q2, using top-1 as the greedy token. This removes both the earlier extra sampler-chain copy/scan and the later top-1-only asymmetry.
Measured evidence packaging was also closed before preflight: q2_run_meta.json is emitted before summarization, and the evidence manifest now includes/hash-binds shader provenance, run metadata, preflight lock, final authorization and both zero-decode baseline runtime qualification records.
Decision: no further Q2 implementation change is planned before target-local preflight unless the preflight itself exposes an implementation/package failure. Zero measured attempts remain mandatory; Q3 remains closed.


## 2026-09-20 — Q2 local preflight attempt #1 blocked by Vulkan SDK elevation; packaging repair only
Target-local preflight on commit 56d425be480ff19b8f1f5306dec028fb4a854c76 verified clean HEAD and static package PASS, compiled all 16 Q2 shaders PASS, and built arcllm_q2.exe PASS. It then stopped fail-closed before baseline build/API qualification because LunarG Vulkan SDK 1.4.357.0 ordinary unattended installation attempted privileged system operations and aborted with "Cannot elevate access rights while running from command line." The installer rolled back.
This is not Q2 scientific evidence and not a baseline/runtime failure: llama.cpp baseline build was not reached, baseline --qualify-only runtime was not reached, llama_decode was not executed, measured attempts remained 0, and Q3 remained closed.
Bounded repair: keep the exact Vulkan SDK version and installer SHA256, but bootstrap with LunarG-supported copy_only=1 plus --root into repo-local .q2_toolchains/VulkanSDK/1.4.357.0. This avoids registry/system-PATH mutation and Administrator elevation. Qualification now validates glslc.exe, vulkan.h and vulkan-1.lib before CMake configuration and records the bootstrap mode.
No model, workload, llama.cpp commit, Vulkan backend, CPU threads, F32 KV, offload policy, timing definition, resource metric, cell order, threshold or authorization gate changed.
Decision: rerun the same zero-measurement preflight after pulling this packaging repair. Twenty measured attempts remain blocked. Q3 remains closed.


## 2026-09-20 — Q2 portable toolchain workspace hygiene
Added .q2_toolchains/ to .gitignore so the repo-local Vulkan SDK copy used by zero-measurement preflight cannot pollute git status or be accidentally staged. This is workspace hygiene only; no Q2 implementation, model, workload, baseline configuration, metric, gate or authorization changed.


## 2026-09-20 — Q2 local preflight attempt #2 blocked by PowerShell tag-refspec interpolation
Target-local preflight after the portable Vulkan bootstrap repair again passed ArcLLM native build. The llama.cpp exact commit fetch also succeeded, then the script stopped fail-closed before checkout/build because PowerShell interpreted the colon-adjacent variable in "refs/tags/$PinnedRelease:refs/tags/$PinnedRelease" as scoped-variable syntax, yielding the invalid refspec "refs/tags//tags/v0.4.1".
This is a packaging/script defect only. It is not a llama.cpp source, Vulkan runtime, ArcLLM runtime, model, or Q2 measurement failure. Baseline checkout/build/API qualification was not reached; baseline runtime qualification was not reached; llama_decode remained unexecuted; measured attempts remained 0; Q3 remained closed.
Bounded repair: brace the PowerShell variable on both sides of the refspec colon: "refs/tags/${PinnedRelease}:refs/tags/${PinnedRelease}". Static QA now asserts the corrected exact string and forbids the unbraced form.
No model, workload, baseline commit/tag, Vulkan configuration, timing, metric, cell order, threshold, science contract or authorization state changed.
Decision: rerun the same zero-measurement preflight from the repaired HEAD. Twenty measured attempts remain blocked.


## 2026-09-20 — Q2 pre-measurement baseline identity correction: v0.4.1 annotated tag
Local preflight after the PowerShell refspec repair successfully fetched commit 391fac16460f15233a7740550d858ac96df3419d and annotated tag v0.4.1, then stopped at the exact tag/commit equality gate. Independent upstream verification established that annotated tag object 29aaf1c27faa48292357cea2120d94114a545006 dereferences to commit b29c606e28a01b1bc8c1351026a0fa6e616bf6c4; 391fac16460f15233a7740550d858ac96df3419d is five commits after that release tag.
This revealed an inconsistency in the frozen draft baseline identity, not a runtime failure. The upstream release page identifies v0.4.1 with b29c606, and llama.cpp release semantics use the annotated Git tag as the release artifact. No Q2 measured attempt had executed, baseline build/API qualification had not started, baseline runtime qualification had not started, llama_decode remained unexecuted, and Q3 remained closed.
Upstream comparison from b29c606e28a01b1bc8c1351026a0fa6e616bf6c4 to 391fac16460f15233a7740550d858ac96df3419d contains five commits. The changed paths are release/CMake/API-ABI tooling plus src/models/qwen4exp.cpp; include/llama.h and src/llama-model.cpp have identical Git blobs at both points, and no Qwen2.5 runtime-path file appears in the compare.
Correction: Q2 release baseline is now v0.4.1 at exact tag commit b29c606e28a01b1bc8c1351026a0fa6e616bf6c4, annotated tag object 29aaf1c27faa48292357cea2120d94114a545006. baseline/CMakeLists.txt sets LLAMA_BUILD_IS_DEV=OFF. The qualifier requires HEAD == tag commit == pinned commit and tag object == 29aaf1c27faa48292357cea2120d94114a545006.
No GGUF, workload, raw-token input, Vulkan backend, F32 KV, n_ctx, batch/ubatch, CPU threads, offload policy, greedy postprocess, timing definition, resource metric, cell order, threshold or Q2 authorization gate changed.
Decision: previous partial preflights are non-authoritative. Rerun zero-measurement preflight from the corrected HEAD. Twenty measured attempts remain blocked; Q3 remains closed.


## 2026-09-20 — Q2 preflight reached baseline runtime qualification; system-power gate corrected
On commit a77e5fadc3894837060875a301d894e7afda6620, local zero-measurement preflight progressed through exact v0.4.1 release-tag baseline build and runtime qualification. llama.cpp loaded the exact target model, reported Vulkan full offload 29/29 layers, placed all 28 KV-cache layers on Vulkan0, and allocated F32 K/V cache totaling 448.00 MiB at n_ctx=4096. The qualify-only path remained before llama_decode, so measured attempts remained 0.
The preflight then stopped because the environment gate treated root\wmi BatteryStatus.PowerOnline from each battery device as the authoritative system AC signal. This is the wrong abstraction for the contract's "AC power connected" requirement; Windows documents GetSystemPowerStatus as the preferred system-level power-source query.
Correction: preflight and measurement runner now require GetSystemPowerStatus ACLineStatus=1. Offline (0) and Unknown (255) fail closed. Per-battery BatteryStatus remains recorded only as descriptive evidence and cannot override the system-level AC result. Active Windows power scheme matching remains mandatory.
This does not relax the environmental control, alter model/runtime settings, or permit any measurement. Because run_q2_preflight.ps1 and run_q2.ps1 are implementation-critical, the prior partial qualification is not authoritative for execution authorization and must be rerun from the corrected HEAD.
Decision: zero measured attempts; Q3 closed; rerun full preflight.


## 2026-09-20 — Q2 preflight environment gate moved to fail-fast position
The system-level AC gate correctly reported Offline on the target and therefore blocked Q2. Review of run_q2_preflight.ps1 found the environment gate was ordered after shader/native build and both baseline qualify-only model loads, causing unnecessary repeated 7B load cost when an environmental prerequisite was already false.
Bounded repair: move OS/CPU/GPU/driver/power-scheme/GetSystemPowerStatus qualification immediately after critical Git cleanliness checks, before Q1 archive/model hashing, static QA, shader compile, ArcLLM build, baseline build, or baseline model loading.
No environmental criterion is relaxed: ACLineStatus must still equal 1, frozen CPU/GPU/driver remain mandatory, and power-scheme evidence is still bound into the preflight lock. Zero measurements remain mandatory; Q3 remains closed.


## 2026-09-20 — Q2 fail-fast static-order guard corrected
Post-commit source QA found the new regression assertion searched for the first textual occurrence of "qualify_q2_baseline.ps1", which appears earlier inside the ImplementationCritical filename list and therefore did not represent execution order. The runtime script ordering itself was correct. Static QA now compares GetSystemPowerStatus against the actual baseline invocation construction "$QualArgs=@(" instead. No runtime behavior or scientific contract changed.


## 2026-09-20 — Q2 zero-measurement preflight PASS / measurement authorization opened
Authoritative returned preflight bundle SHA256: 631B5C0B01AF30B7005CFC17705A7529A1C718FB5F1C9F2958B1E14E258D699D. Preflight lock SHA256: 3973BE933EE27E93803171856189B0406D56F35E6745E8520CD3EFE030153BEB. Implementation commit: 43afd71161c4dc8c766c09c3b55d5eca48352bde.
Exact target SHA/size and authoritative Q1 archive hash PASS. Baseline is exact llama.cpp v0.4.1 annotated-tag commit b29c606e28a01b1bc8c1351026a0fa6e616bf6c4; build/API qualification PASS. W-S and W-C runtime qualifications both PASS with exact prompt hashes 93833ffb49890aba / 5973d0cfd8ad6313, Vulkan full offload 29/29, F32 KV configuration, decode_executed=false and measured_attempts=0.
ArcLLM executable, baseline executable, 16-source/16-SPIR-V shader provenance and all 19 implementation-critical hashes are bound. Repository-vs-Windows hash review confirmed the apparent SHA differences on selected PowerShell files are exactly LF-versus-CRLF checkout normalization; reconstructed CRLF hashes equal the preflight lock and no implementation drift exists.
Environment PASS: Windows 10.0.26200 build 26200, Core Ultra 7 258V, Arc 140V driver 32.0.101.8860, Balanced power scheme, system AC Online via GetSystemPowerStatus. Preflight remained zero-measurement and Q3 remained closed.
Decision: Q2_MEASUREMENT_AUTHORIZED. A governance-only authorization commit may open exactly the frozen four cells × five measured attempts. No advantage verdict is permitted; Q3 remains closed until returned Q2 evidence is independently adjudicated.


## 2026-09-20 — Q2 formally closed: Q2_MATCHED_CHARACTERIZATION_COMPLETE
Authoritative returned measurement bundle SHA256 A802BFA44FE7FEE5723B11E90013B23E1B0F42E51DA877E5889557326726F730; evidence manifest SHA256 559B58ED1E262EBA72D5F162540800D61539B7E98351CB65C22277808265FE95; summary SHA256 719576899733C78A9FA128E8F7E57FCE06104287F983A8CCE94D9B7673854381.
All four frozen cells contain 5/5 successful measured attempts (20/20 total). Exact target/input/runtime invariants, pinned llama.cpp v0.4.1 baseline, Vulkan full offload, primary timing, mandatory RAM/CPU traces and authorization chain PASS.
Descriptive medians show ArcLLM/baseline ratios W-S: TTFT 9.154x, decode TPS 0.02499x, E2E 38.758x, working set 1.832x, private bytes 0.9688x; W-C: TTFT 10.331x, decode TPS 0.03061x, E2E 23.940x, working set 1.825x, private bytes 0.9665x. Q2 does not declare a winner.
Two non-fatal evidence notes are frozen: Windows GPU Engine peak >100% for some ArcLLM samples, so GPU peak is unreliable/conditional; q2_run_meta.cell_exit_codes contains stdout plus trailing 0, while each resource trace independently records process_exit_code=0 and all result/log artifacts PASS.
Decision: Q2_MATCHED_CHARACTERIZATION_COMPLETE. Q2 closed. No tuning is permitted before Q3 design lock. Q3 execution remains closed; next is a predeclared no-practical-advantage confirmatory design with fresh reproduction.


## 2026-09-20 — Q3 no-practical-advantage confirmatory design frozen
Q2 established a valid matched head-to-head but showed a large descriptive performance/resource gap. Q3 is frozen as a falsification study, not an optimization phase: unchanged ArcLLM, same exact model/baseline/hardware and only W-S/W-C.
Two fresh independent sessions are required, each with 4 cells × 5 measured attempts = 40 fresh attempts total. Session order is counterbalanced. A practical advantage requires the same workload and same primary benefit dimension to reproduce in both sessions.
Primary threshold: >=10% TTFT/decode/E2E improvement or >=15% working-set reduction, while all blocking-harm guards remain within 10% of baseline and both systems are 5/5 stable in the candidate cell. Private bytes, CPU utilization and GPU counters cannot establish advantage alone.
If no candidate passes, final verdict is FEASIBLE_NO_DEMONSTRATED_ADVANTAGE and the current ArcLLM architecture line closes. No Q3-A/Q3-B search/tuning is permitted.
NEXUS is explicitly excluded from Q3. Only after a negative final ArcLLM verdict may a separate Architecture Intervention Review use independently established NEXUS findings as hypothesis sources; they cannot be transferred as ArcLLM evidence.
Q3 execution remains closed. Next: implement runner + zero-measurement qualification only.


## 2026-09-20 — Q3 implementation static lock / zero-measurement preflight package
Implemented Q3 without changing the ArcLLM inference architecture, kernels, workloads or pinned baseline. The Q3 preflight verifies every Q2 implementation-critical worktree SHA against the committed Q2 execution authorization before rebuild/qualification; any runtime or instrumentation drift fails closed.
Q3 uses two mandatory separate PowerShell invocations: run_q3.ps1 -Session A and -Session B. Fixed session directories refuse overwrite/rerun, runner PID is recorded, and counterbalanced orders are code-enforced. Each session contains 20 measured attempts; total remains the frozen 40.
The measured runner never rebuilds artifacts. It requires a later committed q3_execution_authorization.json and binds preflight lock, design SHA, Q3 critical hashes, Q2 frozen runtime hashes, exact model, Arc/baseline executables, shader provenance and SPIR-V.
Added deterministic threshold adjudicator and evidence packager. The adjudicator can emit only a candidate verdict; final independent adjudication remains external. Primary advantage dimensions are restricted to TTFT/decode/E2E/working-set; private bytes, CPU and GPU counters cannot establish advantage.
Decision: Q3_IMPLEMENTATION_STATIC_LOCKED. Q3 execution remains closed. Next: target-local zero-decode/zero-measurement preflight only.


## 2026-09-20 — Q3 post-commit package hardening before target preflight
Static review of the first Q3 implementation commit found three evidence-layer defects before any target execution: several regression-test literals did not match the actual source form; run_q3.ps1 created the fixed session directory before authorization gates, which could leave a false session marker after a blocked invocation; and session evidence did not directly bind the SHA256 of q3_execution_authorization.json.
Repair: session directory creation now occurs only after all authorization/artifact/environment gates pass. Runner records execution-authorization SHA in environment and session metadata, and verifies authorization copies of Q2-frozen runtime hashes and compiled-shader hashes. Candidate adjudicator now validates session-complete file hashes, warmup success, exact model/baseline/runtime invariants, scalar process exit codes, authorization SHA binding and distinct session runner PIDs.
No model execution, Q3 attempt, architecture/kernel/workload/threshold or NEXUS boundary changed. Prior Q3 implementation commit is superseded before preflight.


## 2026-09-20 — Q3 zero-measurement preflight PASS / execution authorized
Authoritative preflight bundle SHA256 0807785D6C6250E291BB3005E177A4E6F37C3DBF0CD9D174857AC287C76E58EB; preflight lock SHA256 9105FA31E44029A63A316C13743F2FBB172D20F2C1022C091D70F25CFEDDF76E; implementation commit 032688ee1b188f5ef23c3b3d02466cbf2391eb96.
Independent review verified exact model/size, Q2 formal parent, pinned llama.cpp v0.4.1 baseline, W-S/W-C prompt hashes, full Vulkan offload 29/29, static QA PASS, 16 shader source/SPIR-V bindings, 9 Q3 critical-file hashes, and no Q2 runtime-critical or shader-source drift since the Q2 implementation lock.
Both baseline runtime qualifications remained --qualify-only with decode_executed=false and measured_attempts=0. Preflight recorded sessions_executed=[] and system AC Online on the frozen hardware/driver/power scheme.
Decision: Q3_EXECUTION_AUTHORIZED. Only the frozen two-session protocol is opened: Session A and Session B must run as separate PowerShell invocations, 20 measured attempts each, followed by one evidence packaging step. No rerun, tuning, workload search or NEXUS input is authorized.


## 2026-09-20 — Q3 final adjudication / current ArcLLM architecture closed
Authoritative Q3 returned bundle SHA256 8E19970779A96A68073F6F274A44FC41E6A0DDDB71156181FB23FB2BFC02DC15; evidence manifest SHA256 36CC5966E7C7EE5EB15A6FDFDF38FEF7C885B20D27726FA91D42AFCD8D93ED0B; candidate adjudication SHA256 986A6BD43F91289AD4F1B02232FEC77036E647FF89ECFF377062F56CBA826D88.
Independent audit verified zero packaged hash mismatches, two distinct runner processes (A PID 40168, B PID 51840), exact counterbalanced orders, matched model/baseline/environment, all warmups successful and 40/40 measured attempts successful with valid mandatory timing/resource evidence.
Independent threshold recomputation found no primary benefit dimension passing in any session/workload cell. Arc/base ratios were: A W-S TTFT 12.422x, decode 0.02473x, E2E 39.209x, working set 1.836x; A W-C 9.198x, 0.02172x, 31.355x, 1.829x; B W-S 14.801x, 0.01761x, 55.009x, 1.836x; B W-C 9.605x, 0.02800x, 26.451x, 1.829x. Blocking-harm guard failed in all four cells.
Private bytes remained about 3% lower and process CPU mean much lower, but these were preregistered supporting-only metrics and cannot establish advantage.
Decision: H-NPA NOT FALSIFIED. Final verdict FEASIBLE_NO_DEMONSTRATED_ADVANTAGE. Current ArcLLM architecture line CLOSED. No iterative tuning, workload search, Q3 extension or NEXUS rescue is permitted. Any future work requires a separate post-verdict Architecture Intervention Review; NEXUS may seed hypotheses only after explicit mechanistic mapping.


## 2026-09-20 — Post-verdict Architecture Intervention Review complete / SA-H1 selected specification-only
Reviewed final ArcLLM Q3 evidence, P7/Q2 decode topology, canonical NEXUS R4/R5 findings, confirmed Event-Ledger EL-L4 evidence, LD64 structural/semantic evidence and unresolved LD64-2 performance evidence, plus external LLM-inference literature.
Key causal correction: NEXUS Event Ledger is not directly transferable to the current dense Qwen runtime. ArcLLM already submits one prepared command chain per decode token and has no full-N active-frontier scan. Event Ledger is retained as evidence for representation/scheduler co-design, not as a dense-Qwen speedup claim.
Frozen ArcLLM decode contains 469 dispatches/token = 28 layers × 16 ops + 21 endpoints, including 196 Q/K/V/O/gate/up/down GEMM dispatches. Decode uses generic batch-1 Q4_K/Q6_K GEMM paths while prefill already has tiled/fused paths. Together with Q3 decode throughput only 0.0176-0.0280× llama.cpp and published flat-GEMM/quantized-inference evidence, this creates the strongest direct causal hypothesis.
Decision: select SA-H1 Decode-Specialized Packed-Quant Executor as RESEARCH_REOPEN_CANDIDATE only. Current ArcLLM architecture remains CLOSED. No implementation or target run is authorized. Event-ledger sparse scheduling, MoE/neuron sparsity, attention-only rewrite, KV paging and low-bit activation changes are deferred for lack of direct current causal support or because they change the model/precision contract.
Next scientific step: SA0 specification-only Decode Architecture Causal/Capability Qualification. It must prove hardware capability, cost-model plausibility, fusion legality and a best-case E2E upper bound before any new kernel is written.


## 2026-09-21 — SA0 specification-only causal/capability qualification
Opened SA0 only in ArcLLM; no NEXUS repository mutation is part of this phase. Bound the final Q3 result, frozen Q2/Q3 production graph, P7 closeout and SA-H1 candidate. NEXUS R4-C / EL-L4 artifacts are hypothesis support only; later NEXUS F01 telemetry work is explicitly excluded.
Static decode census reconfirmed 469 dispatches/token with one queue submit/token, 28 layers × 16 operations plus 21 endpoints. Projection GEMMs account for 196 dispatches/token. Therefore the hypothesis that ArcLLM pays 469 CPU submissions/token is rejected.
QKV + gate/up fusion can remove at most 84 dispatches by count (469 -> 385; 17.91%; uniform-cost count-only bound 1.21818x). Ideal normalized-input reread reduction is only 1,204,224 bytes/token versus a 4,370,558,976-byte non-embedding resident-footprint proxy. Fusion-only is therefore rejected as the primary explanation for the Q3 deficit.
Q3 matched same-hardware evidence gives ArcLLM decode 0.2971-0.3348 tok/s versus llama.cpp 11.3508-18.5754 tok/s, a 35.71-56.79x baseline/Arc factor. This proves substantial software/runtime headroom exists on the exact model/hardware but does not by itself attribute root cause.
Decision: SA-H1a decode-specialized batch-1 packed Q4_K/Q6_K GEMM/dataflow is causally plausible and qualified for one bounded successor study. QKV/gate+up fusion is secondary-only. Optional subgroup/cooperative paths require exact-device zero-science capability evidence. Event Ledger direct transfer remains rejected.
SA0 status: SA0_SPECIFICATION_QUALIFIED_CAPABILITY_PREFLIGHT_REQUIRED. Current architecture remains CLOSED; implementation_permitted=false; target_run_permitted=false. Next: SA0-CAP zero-science exact-device Vulkan capability preflight only.


## 2026-09-21 — SA0-CAP zero-science capability probe implementation
Implemented SA0-CAP only in ArcLLM. Added a standalone Vulkan physical-device query executable that deliberately stops before logical-device creation: no model load, no shader module, no compute pipeline, no dispatch and no Q2/Q3 workload.
The probe inventories exact device/driver/API identity, compute queue/timestamp support, subgroup size/stages/operations, subgroup-size-control, extended 8/16-bit scalar/storage features, compute workgroup/shared-memory limits, memory heaps/types, and KHR/NV cooperative-matrix properties if exposed.
The local wrapper binds the Core Ultra 7 258V + Arc 140V machine and Q3 reference Windows driver 32.0.101.8860, rebuilds only the query executable, verifies zero-science invariants, hashes the implementation/evidence chain and emits results/sa0_capability_return_to_chatgpt.zip.
Optional subgroup/cooperative features are descriptive: absence does not falsify SA-H1a. Current ArcLLM architecture remains CLOSED; successor implementation=false; target model execution=false; SA1-P remains CLOSED pending independent adjudication of returned SA0-CAP evidence.


## 2026-09-21 — SA0-CAP independent static-equivalent audit PASS; target-local execution next
Audited exact implementation commit `1a530292566896e83431b9e910249af19a562110`. All zero-science boundaries PASS: no `vkCreateDevice`, shader-module/pipeline creation, dispatch/queue submit, GGUF/model access or Q2/Q3 runner invocation exists in the probe path. Physical-device properties/features, queue/memory, subgroup and cooperative-matrix property queries are present; exact CPU/GPU/driver gates and evidence packaging are bound.
GitHub Actions runs `35527283508` (SA0 static) and `35527283630` (Q2 audit triggered by manifest) both ended with `steps=null` and unavailable logs. Classified `F0_GITHUB_ACTIONS_BEFORE_STEPS`; no source repair or scientific inference is authorized from those runs.
Decision: `PASS_CONNECTOR_EQUIVALENT_STATIC_AUDIT`. SA0-CAP is READY for target-local Windows BuildOnly + exact-device zero-science query. Current ArcLLM architecture remains CLOSED; successor implementation and target model execution remain forbidden; SA1-P remains blocked until returned capability evidence is independently adjudicated.


## 2026-09-21 — SA0-CAP local static-test governance-state drift repaired
Target-local invocation at HEAD `fd7bde8b63ac65abe2c3193c52eb1ded398d454c` stopped in `tests/test_sa0_capability_package.py` before build or Vulkan query. The test still required the pre-audit manifest literal `SA0_CAP_PROBE_IMPLEMENTATION_STATIC_LOCKED`, while the preceding governance-only static-audit commit had legitimately advanced manifest/SA0/probe state to `SA0_CAP_READY_FOR_TARGET_LOCAL_ZERO_SCIENCE` / `STATIC_AUDIT_PASS_READY_FOR_TARGET_LOCAL`.
Classification: `F0_META_TEST_GOVERNANCE_STATE_DRIFT`. No BuildOnly, Vulkan capability query, model load, shader execution, target benchmark or successor implementation occurred.
Repair scope: test-only governance-state assertion plus this append-only lineage entry. Probe source, build runner, capability runner, SA0-CAP contract and all scientific/runtime files remain byte-identical.
Next: rerun target-local static QA on the repair HEAD. Only PASS permits the already-frozen zero-science capability preflight.


## 2026-09-21 — SA0-CAP exact-device adjudication PASS
Returned bundle SHA256 `C272F50AA5730D417BC0234127F72E345A4168F922794783FE6743CDD0C91570`, 7,820 bytes, five artifacts. Raw capability hash, BuildOnly manifest hash, implementation commit, frozen contract and critical Git-blob chain all cross-validate; repository verification matches 6/6 critical blobs at `e32ed1ccfe0d6e675e3186af5ca7db606692bd98`.
Zero-science boundary holds: no model load, no shader compile/module/pipeline, no dispatch, no Q2/Q3 execution and no successor kernel implementation.
Exact Arc 140V capability PASS: Vulkan device API 1.4.348, compute queue, 64 timestamp-valid bits, subgroup size 32, subgroup extended types, subgroup-size-control 16-32, computeFullSubgroups, 8/16-bit storage, FP16/INT8, 49,152-byte compute shared memory and 1,024 max workgroup invocations. VK_KHR_cooperative_matrix is exposed with four subgroup-scope combinations including FP16->FP32 and signed/unsigned INT8->INT32 forms.
Decision: `SA0_CAP_PASS`; SA0 COMPLETE. These capabilities establish implementation options but no performance advantage. Cooperative matrix remains optional because compatibility/benefit for packed Q4_K/Q6_K dequant dataflow is not yet established.
SA1-P specification-only preregistration is now permitted. Successor kernel implementation=false; target model execution=false; Q3 reopen forbidden.


## 2026-09-21 — SA1-P preregistration candidate opened specification-only
Opened SA1-P from SA0-CAP PASS. No shader, harness or target measurement was added.
Frozen one primary mechanism: subgroup-32 split-K per output row. One 128-thread workgroup contains four full 32-lane subgroups, each subgroup owns one row, lanes split K, directly dequant packed Q4_K/Q6_K into FP32 partial sums, subgroup-reduce, and lane 0 stores.
Frozen exact batch-1 7B shape universe: Q4 (3584x3584 bias/no-bias, 3584x512 bias, 3584x18944 no-bias, 18944x3584 no-bias) and Q6 (3584x512 bias, 18944x3584 no-bias), with exact row/weight bytes.
Frozen baseline Q4/Q6 P7 shader Git blobs and Q2 SPIR-V hashes. Cooperative matrix, subgroup-16, fusion, pre-dequantization, tile/local-size search and multiple candidate variants are explicitly forbidden in SA1.
Frozen timing: exact Arc 140V/Q3 driver, GPU timestamps, two processes A/B, forward/reverse cell order, 10 warmups/arm/cell, 30 measured pairs/cell/process, alternating within-pair order, no outlier deletion or pooling.
Frozen correctness: finite, max_abs<=0.02, RMSE<=0.005 against CPU and baseline.
Frozen materiality gate: Q4 five-cell geometric-mean speedup>=1.50x independently in A/B and every cell>=1.10x; valid Q4 FAIL stops SA1 and blocks Q6. Q6 future extension uses the same 1.50x aggregate / 1.10x floor and same mechanism.
Status: SA1P_PREREGISTRATION_CANDIDATE_AWAITING_INDEPENDENT_QA. Candidate code and measurement remain forbidden.


## 2026-09-21 — SA1-P independent QA PASS / implementation lock
Independent QA of preregistration commit `e98650ec3b2e0f0c8fa3168f3d97aed8ea599896` PASS. Recomputed 7/7 exact shape cells, row bytes and packed weight bytes; verified Q4/Q6 baseline Git blobs and Q2 SPIR-V hashes; verified specification-only delta and all execution gates closed.
Worst-case four-bank Q6-down weight allocation is 222,781,440 bytes (~212.46 MiB), within the frozen 256 MiB per-cell process cap. One mechanism remains frozen: subgroup-32 split-K per output row. Cooperative matrix/fusion/staging/geometry search remain forbidden.
Implementation lock now authorizes only SA1-K1 engineering: exactly one Q4 candidate shader at `shaders/sa1_q4k_subgroup_splitk.comp`, one component harness and bounded SA1 build/preflight/adjudication tooling. Q6 implementation remains blocked until independent valid Q4 PASS.
Important: the lock does NOT authorize component measurement, target-model inference or Q3. SA1-K1 must first undergo static QA and zero-measurement preflight, followed by separate execution authorization.


## 2026-09-21 — SA1-K1 exact locked implementation
Implemented the first successor kernel under SA1-P lock, Q4 only. Exactly one new shader was added: `shaders/sa1_q4k_subgroup_splitk.comp`.
Mechanism matches the frozen lock: local_size 128, required subgroup size 32 at Vulkan pipeline creation, full-subgroup flag, four subgroups/workgroup, one subgroup/output row, K stride 32, direct packed Q4_K dequant, FP32 partial accumulation, subgroupAdd reduction, lane-0 store. No shared memory, scratch, staging, fusion, cooperative matrix or variant search.
Added a standalone synthetic component harness. Preflight mode executes correctness only on banks 0/3 across all five Q4 shape cells with no timestamp query, no performance sample and no model load. Measurement mode contains the frozen future A/B protocol but requires a separate authorization file that does not exist at implementation time.
Added pinned compile/build tooling, fail-closed local preflight, future measurement runner and adjudicator. Existing P7/Q2/Q3 production files are untouched.
Status: SA1K1_IMPLEMENTED_AWAITING_STATIC_QA. Component measurement, Q6 implementation, target-model execution and Q3 remain forbidden.


## 2026-09-21 — SA1-K1 pre-static implementation QA repair
Before any compile, GPU dispatch, correctness preflight or performance measurement, connector audit of implementation commit `e5c761eff4811c41cee8b3c803d560600baac656` found three engineering-only defects: MSVC-risky multiword functional casts in RMSE code; an incorrect descriptive future-timing census field (400 rather than 300 timed dispatches / 600 timestamp values); and retention of all four host fixture weight copies after upload, which could violate the 256 MiB component-process allocation budget.
Repaired only the SA1 component harness/static test. Candidate shader, frozen mechanism, geometry, fixtures, correctness thresholds, performance gates, baseline identity and all P7/Q2/Q3 files are unchanged. No scientific attempt was consumed.


## 2026-09-21 — SA1-K1 independent static-equivalent audit PASS
Audited exact repaired implementation commit `2f163873723b6d5bdf70cc7f8385d2bfd370ab27`. Verified one and only one SA1 shader; exact subgroup32 split-K geometry; required subgroup size 32 and full-subgroup pipeline controls; no shared memory/cooperative matrix/staging/fusion; immutable P7 Q4/Q6 baselines and Q2/Q3 runtime; corrected resource handling; and separate execution-authorization gate on future measured mode.
The Vulkan required-subgroup-size structure and full-subgroup flag are valid for compute, and frozen local_size_x=128 is a multiple of required subgroup size 32. This is API/structural validation only.
Decision: `PASS_CONNECTOR_EQUIVALENT_STATIC_AUDIT`. Native shader compile, C++ BuildOnly and exact-device correctness preflight remain NOT RUN and cannot be inferred from static review.
Status: READY_FOR_ZERO_MEASUREMENT_PREFLIGHT. Measurement, Q6, target-model execution and Q3 remain forbidden.


## 2026-09-21 — SA1-K1 preflight packaging F0 after compile/build/correctness PASS
Target-local invocation at `b45c2cee60e99e4b7700ff032489e77df9fbd3c3` reported static QA PASS, candidate/baseline shader compile PASS and native BuildOnly PASS. It then reached post-preflight packaging and failed while evaluating `H $Raw`: Windows PowerShell resolved the one-letter helper name `H` through the built-in alias `h -> Get-History`, attempting to parse the JSON path as a history Id.
Because the failure occurs after the script has invoked the correctness-only component executable and passed its status/zero-measurement guards, this is classified `F0_POWERSHELL_HASH_HELPER_ALIAS_COLLISION_AFTER_VALID_ZERO_MEASUREMENT_PREFLIGHT`, not a shader/build/correctness scientific failure. No timestamp query, measured pair, performance gate, model load, Q6 implementation or Q3 execution occurred.
Repair: rename SHA helper to `Get-Sha256` in preflight and future measured runner; add static guard against one-letter `H`; add `-PackageExisting` recovery mode that verifies the existing native-build execution commit and unchanged candidate/harness blobs, then packages existing raw/build/SPIR-V/executable evidence without recompiling, rebuilding or redispatching GPU correctness.


## 2026-09-21 — SA1-K1 recovery packaging tokenization F0 repaired
Packaging-only recovery at `7a24368569da4f4edaac2a8cc728130d6c0d0b8a` successfully entered recovery mode and therefore did not recompile, rebuild or redispatch GPU work. It then failed before ZIP creation because `Test-Path$Zip` was tokenized as an unknown command rather than `Test-Path $Zip`.
Classified `F0_POWERSHELL_CMDLET_VARIABLE_TOKENIZATION_DURING_PACKAGING`. Exact scan of the SA1 preflight, measured runner, compiler and builder found no other command-variable concatenation in the checked cmdlet set.
Repair is packaging/test/governance only: insert the missing whitespace and add a static regression guard. Candidate shader, component harness, raw correctness evidence, compiled SPIR-V and native executable are unchanged; no scientific attempt is consumed.


## 2026-09-21 — SA1-K1 zero-measurement preflight adjudication PASS / evidence lock
Returned bundle SHA256 `37366F785BB391E38E04A8FD0D631850C0DE62F13E2BACEA7FEA2A8363E14D02`, 10 entries. Independent rehash verified all preflight-lock bindings: raw correctness, shader/native build manifests, executable, baseline SPIR-V and candidate SPIR-V. Frozen baseline SPIR-V exactly reproduces the Q2 hash.
Scientific execution commit remains `b45c2cee60e99e4b7700ff032489e77df9fbd3c3`; final packaging commit is `e2d0b025418d43ad634f3323d873f7da5527d495`. Candidate shader/harness/baseline/contract/parent-lock blobs are identical across both. Packaging recovery had two F0 defects; the returned lock field names only the first, while lineage binds both. Neither recovery reran compile/build/GPU correctness.
Correctness PASS across 5 Q4 cells × banks 0/3. Worst candidate-vs-CPU was max_abs 0.013916015625, RMSE 0.00269372814522; worst candidate-vs-baseline max_abs 0.014404296875, RMSE 0.00270290781691, all below frozen 0.02/0.005 gates.
Zero-measurement invariants PASS: model_loaded=false, performance_measurement=false, timestamp_queries=0, measured_pairs=0, performance_gate_evaluated=false.
Decision: `SA1_K1_ZERO_MEASUREMENT_PREFLIGHT_PASS`; implementation evidence locked. Q4 execution authorization may now be created only as a separate commit. Q6/model/Q3 remain blocked.


## 2026-09-21 — SA1-K1 Q4 measured execution authorized
Following independent preflight PASS and evidence-lock commit `d207089a87cd9938b977a29689d4a2e934d2fdac`, authorized exactly one Process A and one Process B Q4 component measurement under the frozen executable/SPIR-V hashes.
Frozen tooling reference is `e2d0b025418d43ad634f3323d873f7da5527d495`; scientific preflight execution remains bound to `b45c2cee60e99e4b7700ff032489e77df9fbd3c3`. Authorization binds executable `AC993642...`, baseline SPIR-V `2EFD94AC...`, candidate SPIR-V `B16868A8...`, five Q4 cells, 10 warmups/arm/cell, 30 measured pairs/cell/process, A forward/B reverse order, no outlier deletion and no process pooling.
Decision: `SA1_Q4_EXECUTION_AUTHORIZED`. Measurement is not yet consumed. Run A exactly once and B exactly once. Any infrastructure invalidation must return for adjudication before retry; no self-authorized rerun. Q6/model/Q3 remain blocked.


## 2026-09-21 — SA1-K1 Q4 measured adjudication PASS
Authorized Process A and B returned complete measured JSONs. A SHA256 `1EB9F0907E542B51B8195BFA7F26B46BC729FC86C563EE6501A3738A606BB625`; B SHA256 `ACF7CE9AB6B28841A5E3516933DB792C6CD7CE0A5647FC9D4BCE060A3F9A2421`. Each contains exactly five Q4 cells and 30 baseline + 30 candidate samples per cell; A uses forward cell order and B reverse order; no target model was loaded.
Independent recomputation with the preregistered statistic gives process A geometric mean 3.1371499895x, minimum cell 1.6594191177x; process B geometric mean 3.1475012006x, minimum cell 1.6714738763x. Both exceed frozen geomean >=1.50x and every-cell >=1.10x gates. No samples or visible timing spikes were removed.
Decision: `Q4_STAGE_PASS`. Q4 measurement is consumed and must not be rerun. This supports the bounded Q4 component mechanism only; no end-to-end model claim follows. The preregistered Q6 extension may now receive a same-mechanism implementation lock. Q6 measurement/model/Q3 remain blocked.


## 2026-09-21 — SA1-K2 Q6 implementation lock opened after Q4 PASS
Prerequisite Q4 adjudication at `accd26471e4abd5bbe9981d49121068e116cc62c` is `Q4_STAGE_PASS`. Opened the preregistered Q6 extension only; no new scientific mechanism is introduced.
Frozen two Q6 cells: 3584→512 bias and 18944→3584 no-bias. Frozen baseline Q6 blob `a0de99f972db6cd202606aad95fb9eab223639e6` and SPIR-V SHA256 `F2267838D099128F233EF30817464658AAD71AAFA3933461FB315FAD10ED3F67`.
Implementation lock permits exactly one `shaders/sa1_q6k_subgroup_splitk.comp` using the same local_size=128, required subgroup=32, four rows/workgroup, direct packed Q6_K dequant, FP32 partial accumulation and subgroup reduction. Q4 implementation/results are immutable. Cooperative matrix/fusion/staging/geometry search remain forbidden.
Q6 performance measurement remains forbidden pending static QA, correctness-only zero-measurement preflight, independent evidence adjudication and separate execution authorization.


## 2026-09-21 — SA1-K2 exact same-mechanism Q6 implementation
Implemented exactly one new Q6 successor shader under lock `2a7a7a2ecec38aac0853f112e7bea01bbad7514a`: `shaders/sa1_q6k_subgroup_splitk.comp`.
Mechanism/geometry are unchanged from Q4: local_size=128, required subgroup=32 at pipeline creation, full subgroups, four rows/workgroup, one subgroup/output row, K-stride 32, direct packed Q6_K dequant, FP32 partial accumulation, subgroupAdd reduction and lane-0 store. No shared memory, scratch, staging, fusion, cooperative matrix or variant search.
Extended the existing component harness minimally with a Q6 mode containing only the two preregistered cells and CPU/reference fixture support for the frozen 210-byte Q6_K block layout. Q4 mode remains the default and the Q4 candidate shader is immutable.
Compile/native-build artifacts for Q6 are isolated under `artifacts/SA1_K2/build`. Q6 correctness preflight is banks 0/3 only and zero-measurement. Future Q6 timing is fail-closed behind a separate authorization file that does not exist.
Status: SA1_K2_Q6_IMPLEMENTED_AWAITING_STATIC_QA. No Q6 measurement, model execution or Q3 reopen.


## 2026-09-21 — SA1-K2 pre-static test assertion repair
Before shader compile/native build/GPU execution, connector static audit of implementation commit `ad3fc9ace60cc7882a6d737efe39726d46e7f725` found one regression-test-only defect: the Q6 build-stage isolation assertion searched for a concatenated runtime path string rather than the literal PowerShell construction expression.
Repaired only `tests/test_sa1_component_package.py` to assert the actual `"artifacts\\SA1_"+$Stage+"\\build"` construction. Q6 shader, harness, fixture/reference semantics, compile/build scripts, preflight gates and all frozen Q4/Q6 scientific contracts are unchanged. No execution attempt consumed.


## 2026-09-21 — SA1-K2 independent static-equivalent audit PASS
Audited exact repaired Q6 implementation commit `53195e7951fd740da32add7182657c27153a23ab`. Verified exactly one Q6 candidate; immutable Q4 candidate and frozen P7 Q6 baseline; exact same subgroup32 split-K mechanism/geometry; direct packed 210-byte Q6_K layout mirrored in shader and CPU reference; exact two-cell Q6 census; isolated `artifacts/SA1_K2/build`; Q6 preflight banks 0/3 only; and fail-closed future measurement authorization.
No Q6 execution-authorization file exists. Q6 measurement/model/Q3 remain closed.
No GitHub Actions workflow run is associated with this commit, so shader compile, native Windows BuildOnly and exact-device GPU correctness preflight are explicitly NOT RUN and cannot be inferred from static review.
Decision: `PASS_CONNECTOR_EQUIVALENT_STATIC_AUDIT`; ready for exact-target Q6 compile/BuildOnly + zero-measurement correctness preflight.


## 2026-09-21 — SA1-K2 Q6 correctness preflight FAIL observed; stop rule engaged
Exact target-local run at `b87f3bccee3809cedb2d88ab77c9885406348a87` reported `SA1_K2_STATIC_QA_PASS`, frozen Q6 baseline/candidate shader compile PASS and native BuildOnly PASS, then stopped at `correctness gate failed: candidate_cpu`.
The harness evaluates `baseline_cpu` before `candidate_cpu`; reaching the candidate failure therefore means the baseline-vs-CPU gate had already passed for the active case while the same-mechanism Q6 candidate violated at least one frozen correctness condition (finite, max_abs <= 0.02, RMSE <= 0.005).
Static post-failure audit found no byte-layout/indexing implementation defect: P7 Q6 baseline, candidate Q6 shader and CPU reference share the same 210-byte block semantics; the intended algorithmic difference is split-K/subgroup FP32 accumulation order. Under the preregistered rule `Any correctness gate failure => scientific FAIL for that stage; no performance rescue`, the Q6 extension is stop-closed. No threshold mutation, alternate geometry, reduction-order rescue, Q6 timing or target-model run is authorized.
Because the error path wrote a compact ERROR JSON rather than the full metric record, added packaging-only `-PackageFailedExisting` recovery. It binds the already-produced raw error/build/SPIR-V/executable artifacts to execution commit `b87f3bc...` and performs no compile/build/GPU rerun.


## 2026-09-21 — SA1-K2 post-failure static-state assertion repair
After engaging the Q6 correctness stop rule, the static package test still expected the pre-failure manifest state `q6_implementation_permitted=true`. Repaired only that governance assertion to require `q6_implementation_permitted=false`, component measurement false and rerun_authorized=false. No Q6 shader/harness/build artifact or scientific evidence changed.


## 2026-09-21 — SA1-K2 Q6 independent evidence adjudication: correctness FAIL
Returned bundle `sa1_k2_q6_failure_return_to_chatgpt.zip` independently rehashed to SHA-256 `B4D6A6F7CF4ADE32EFFD9408754216CF3ED8FCABC2065F1A94D253CBA0933E95`. All ten entries were rehashed; frozen baseline SPIR-V matched `F2267838D099128F233EF30817464658AAD71AAFA3933461FB315FAD10ED3F67`; candidate source Git blob matched `0fdc0c8f195872396a653b38ee2283156fbaeaa0`; implementation lock/contract blobs matched the scientific execution state. Native build provenance binds execution to `b87f3bccee3809cedb2d88ab77c9885406348a87`; packaging binds to `8e783119ff6268d26d9c2e8405e0cb5f2faec182`.
The harness order is `baseline_cpu -> candidate_cpu -> candidate_baseline`. Raw result is exactly `ERROR / correctness gate failed: candidate_cpu`; therefore the active-case baseline gate had already passed and the candidate breached at least one frozen numerical condition. Fail-fast did not retain exact magnitude or violated dimension, and no rerun was performed. No F0 invalidation was found.
Formal classification: `Q6_STAGE_FAIL_CORRECTNESS` / `SA1_K2_Q6_CORRECTNESS_FAIL`. Q6 timing, rerun, rescue, target-model execution and Q3 reopen remain forbidden. Machine-readable adjudication commit: `984c4052d523f005ffbe43499c5e564af66232fa`.

## 2026-09-21 — SA1 formal closeout
SA1 closed asymmetrically: Q4_K is `Q4_STAGE_PASS` with reproducible component uplift under the frozen contract; Q6_K, using the same mechanism/geometry, is `Q6_STAGE_FAIL_CORRECTNESS` before performance measurement. Thus SA1 supports mechanism efficacy for the frozen Q4_K component family while falsifying cross-quant generality of the unchanged mechanism to Q6_K under the frozen numerical contract. End-to-end applicability remains untested.
Formal closeout artifact commit: `b70b1e0ea48c63b9cf1562e56fb12727b05d2ed9`. SA1 is immutable/closed. The only scientifically valid continuation is a new specification-only, independently preregistered research program addressing the Q6 correctness boundary; no new execution is opened by this closeout.


## 2026-09-21 — Q6CB-1 opened specification-only after SA1 closeout
Opened independent branch `research/q6-correctness-boundary` from exact SA1-closeout HEAD `3352dbc841a06f1cab8e70f9d8353d683c411ccc`.
Program: `Q6CB-1 — Q6 Correctness-Boundary Mechanism Study`.
This is not an SA1 rescue: SA1 remains immutable and closed as `SA1_CLOSED_ASYMMETRIC_Q4_PASS_Q6_CORRECTNESS_FAIL`; no Q6CB outcome can reclassify SA1, mutate its thresholds, rerun its Q6 fixtures as fresh evidence, authorize Q6 performance timing, or load the target model.
Competing hypotheses are preregistered rather than assuming reduction topology is causal: reduction-topology × conditioning interaction (H-RTCI), packed-dequant access interaction (H-PDI), device/subgroup arithmetic contribution (H-DSA), hidden semantic/reference defect (H-SEM), and no stable fresh boundary (H-NSB).
The causal-identification design requires serial/high-precision references, deterministic CPU fixed-tree emulation, GPU direct-packed split-32, GPU pre-expanded split-32 diagnostic control, conditioning-strata contrasts, and an independent Q6 semantic invariant.
Exact SA1 Q6 cells, banks 0/3, inputs/weights/outputs and random seeds are excluded from future Q6CB primary/confirmatory evidence. Identification and confirmatory partitions must both be frozen before the first scientific execution.
Current state: specification files only. Causal harness implementation, scientific CPU/GPU execution, fresh fixture execution, timing, target-model load and SA1 rerun are all forbidden pending zero-science QA and later explicit locks/authorization.


## 2026-09-21 — Q6CB-0 zero-science specification QA PASS
Audited specification HEAD `af36dbc5382d3d0b1ec009f1562230ffb68456a3` against exact SA1-closeout base `3352dbc841a06f1cab8e70f9d8353d683c411ccc`.
Delta contained only the Q6CB specification contract/document plus manifest/lineage governance updates; no shader, native source, PowerShell runner, scientific harness or execution-authorization file was changed or created.
SA1 Q6 candidate shader blob remained `0fdc0c8f195872396a653b38ee2283156fbaeaa0`; SA1 closeout blob remained `83b30549410ce89e8d5047685c8bcbddc87983ee`; Q6 adjudication blob remained `a2419af5a98ec931424bda46de840d3495e0bc38`.
Scientific design QA passed: competing hypotheses are explicit, reduction topology is not assumed causal, fresh-data exclusions are frozen, identification/confirmatory partitions must be frozen before first execution, negative/multifactor/unresolved outcomes are admissible, and the roadmap is finite.
Decision: `PASS_ZERO_SCIENCE_SPECIFICATION_QA`; Q6CB-0 complete. No implementation or scientific execution is automatically authorized. Next gate is explicit authorization for Q6CB-1 causal-harness implementation lock only.


## 2026-09-21 — Q6CB termination and goal-alignment governance locked
Before authorizing Q6CB-1 implementation, a hard-stop governance was added and independently zero-science audited. The research line now defines scientific success as a trustworthy adjudication rather than a positive outcome.
Q6CB terminates at `Q6CB-4 FINAL ADJUDICATION`; Q6CB-5 is forbidden. At most one causal successor intervention (`SI-1`) may follow a supported causal result, with exactly one candidate and one valid frozen correctness attempt. SI-2 is forbidden. If SI-1 shows practical component value, micro-research stops and the program returns to real-model end-to-end validation.
Negative outcomes including non-reproduction, unresolved mechanism, falsified hypotheses, non-actionability, correctness failure and lack of practical value are retained as terminal scientific evidence rather than rescued.
Every next action is subject to a mandatory goal-alignment test. If it no longer directly reduces uncertainty required for the frozen causal question or the one permitted intervention, or cannot alter a declared terminal decision, the binding decision is `STOP_DRIFT`.
F0 repair is also finite: at most one bounded tooling/measurement repair per stage; a second F0 at that stage yields `STOP_INFRASTRUCTURE_UNSTABLE`.
Zero-science QA confirmed no scientific code or authorization changed. Q6CB-1 implementation and all scientific execution remain closed pending the existing explicit implementation-authorization gate.


## 2026-09-21 — Q6CB-1 implementation source audit PASS; BuildOnly pending
After explicit implementation-only authorization and exact allowlist freeze, implemented the isolated five-arm Q6CB causal harness, independent canonical Q6 decoder/reference, deterministic conditioning-strata generator, two diagnostic GPU shaders, BuildOnly tooling and static/unit invariant test.
Allowlist audit from implementation-scope commit `42277644207197934be2ca57b26a59fde510fd40` found only permitted Q6CB files plus manifest/documentation changes; SA1 source/evidence was not changed.
A pre-BuildOnly connector-equivalent source audit verified: R64/S32/T32-CPU arms; packed and expanded GPU split-32 arms with identical geometry; generator rejection of the two exact SA1 Q6 cells and all four SA1 seeds; absence of timing/model paths; and fail-closed future execution authorization before fixture generation or Vulkan initialization.
One compile-portability defect was repaired before any BuildOnly or science: `src/q6cb_causal_harness.cpp` now includes `<iterator>` explicitly for `std::istreambuf_iterator`. No scientific contract, causal arm, fixture semantics or threshold changed.
Current exact source blobs are recorded in manifest. Scientific CPU/GPU execution remains forbidden. Next permitted action is static/unit test plus target-local shader compile and native BuildOnly only; the produced harness must not be launched.


## 2026-09-21 — Q6CB-1 prelock semantic-invariant repair
Target-local static/unit, shader BuildOnly and native BuildOnly at commit `93c0980eab34bcf747cd685f7d9db34bb06697ff` all passed with no executable launch, GPU dispatch, timing or model load. During independent adjudication before final implementation lock, one scientific-identifiability gap was found: the runtime observation did not expose an independent semantic reconstruction invariant capable of distinguishing H-SEM from the packed-path contrast H-PDI.
Because no fresh Q6CB science had executed and the implementation lock was not yet finalized, a bounded prelock implementation repair was applied without changing hypotheses, topology, shaders, conditioning strata, thresholds, SA1 evidence or execution permissions. The deterministic generator now stores independently generated intended q/scale/d-bit values; the canonical decoder reconstructs them from packed bytes; the harness emits exact q/scale/d mismatch counts as `semantic_invariant`.
Changed files are limited to `src/q6cb_fixture_generator.hpp`, `src/q6cb_causal_harness.cpp`, and `tests/test_q6cb1_package.py`. Both GPU shader blobs remain unchanged. Prior BuildOnly evidence remains useful provenance for the unchanged shaders but cannot serve as the final exact-head native lock. Static/unit plus target-local BuildOnly must be rerun on the repaired exact HEAD before Q6CB-1 can close.
Scientific execution remains forbidden.


## 2026-09-21 — Q6CB-1 BuildOnly PASS; static-test false negative repaired
Target-local validation at `6ba51a9d968cd0ed46f808b2f241cd8a50b7e118` produced shader BuildOnly PASS and native BuildOnly PASS. The two SPIR-V SHA-256 values are `CE326D6CFB6474D3B255D0530FE3BE789BACC8B1540A709223EF7F06E1C49B61` and `73F35157417A2BB28E0BBB97D1F9CFE4AD7C54C0DFED599DEA0E4EE53F5AA6D2`; the native executable SHA-256 is `83E2A04C83418258EB2BB77E4350D3661766EFDBB140E754AE704805D62A1AA9`. Evidence explicitly records executable_launched=false, scientific_execution=false, gpu_dispatch=false, performance_timing=false, and model_loaded=false.
The static test failed only because its source-text assertion searched for an unescaped `"semantic_invariant"` token while the C++ source necessarily contains escaped quotes inside the emitted JSON string literal. Commit `54d1f9d4f0dfc0994e4d072366a1101c3a5ef753` corrects only that assertion.
A commit comparison confirms this repair changes only `tests/test_q6cb1_package.py`; all harness, generator, reference, shader and BuildOnly-tool blobs remain identical to those used by the successful target-local build. Therefore no native/shader rebuild is scientifically or technically required. Only the repaired static test must be rerun before final static-equivalent QA and implementation/evidence lock.
Scientific execution remains forbidden.


## 2026-09-21 — Q6CB-1 implementation locked; goal alignment PASS
Final static/unit QA at `627bcc52759b25649b49fd494ba6c802d7bf3d50` returned `Q6CB1_STATIC_UNIT_QA_PASS`. The target-local BuildOnly evidence from `6ba51a9d968cd0ed46f808b2f241cd8a50b7e118` remains authoritative because the only subsequent code change before the static PASS was the test-only escaped-literal assertion repair; build-critical source/shader/tool blobs did not change.
Static-equivalent QA is `PASS_STATIC_EQUIVALENT_QA`. The exact implementation/evidence lock was frozen at commit `237a4174b409bf2e2200fa96fe1a7dbccfa76bab`, binding the canonical reference, fixture generator, causal harness, both GPU shaders, BuildOnly tools, SPIR-V hashes and native executable hash. Implementation mutation is now closed.
The mandatory five-question goal-alignment check passed: the next stage directly serves the frozen causal question, is required to distinguish declared terminal outcomes, remains inside the hard budget, preserves prior evidence, and remains scientifically justified under a negative result. No drift was detected.
Decision: `CONTINUE_WITHIN_LOCK_TO_EXECUTION_CONTRACT_DESIGN_ONLY`.
Fresh fixture execution, CPU/GPU scientific execution, timing and target-model loading remain forbidden. No execution authorization exists. The only next permitted stage is specification-only design and freeze of the execution contract.


## 2026-09-21 — Q6CB execution contract locked; Q6CB-2 closed
Execution-contract specification completed under the locked Q6CB-1 implementation. The frozen design binds two fresh K-length shapes, five conditioning strata, three independent seeds per shape/stratum, and separate 30-fixture identification and 30-fixture confirmatory partitions.
Zero-science QA result: `PASS_ZERO_SCIENCE_EXECUTION_CONTRACT_QA`.
The exact execution contract is bound by `config/q6cb1_execution_contract_lock_v0.1.json`; fixture/seed identities, numerical tolerances, F0/F1 and H-RTCI/H-PDI/H-DSA/H-SEM rules, evidence schema and target runtime identities are now immutable.
No fresh fixture was generated or executed, no CPU/GPU scientific execution occurred, no timing or target-model load occurred, and no execution authorization exists.
Q6CB-2 remains closed. The only next admissible gate is explicit authorization for the single frozen Q6CB-2 identification collection.


## 2026-09-21 — Q6CB-2 frozen identification authorized
The explicit authorization gate was passed only after the execution contract and zero-science QA were locked.
Authorization file: `config/q6cb2_execution_authorization.json`, commit `a20bd784413f40b8d3e590f70c358a9936318590`.
Scope is exactly one frozen Q6CB-2 identification partition: 30 fixtures in canonical order, using the locked executable and two locked SPIR-V artifacts. Fixture/seed substitution, selective valid-fixture replay, threshold/rule changes, implementation changes, timing and target-model loading remain forbidden.
Q6CB-3 remains unauthorized and ineligible until independent Q6CB-2 adjudication determines otherwise.
No Q6CB-2 fixture has yet been executed at this authorization record point.


## 2026-09-21 — Q6CB-2 one-shot PS1 runner locked
A single operator entrypoint, `run_q6cb2_identification.ps1`, was added and zero-science static QA passed. The Q6CB-2 authorization was reissued as revision 2 before any Q6CB-2 execution and now binds the exact runner Git blob `fa82d7023d0f94de2572df852f0d3a2f9dab6c91`.
The runner fail-closes on contract/source/runtime provenance, validates the exact 30-fixture identification schedule and target identity, prevents overwrite/selective replay, executes only the frozen Q6CB-2 partition, and packages provenance without adjudicating scientific outcomes.
Q6CB-3 remains unauthorized. No Q6CB-2 fixture has been executed at this lineage point.


## 2026-09-21 — Q6CB-2 pre-science F0 repaired within bounded allowance
The first Q6CB-2 runner invocation failed closed during environment preflight because the portable Vulkan SDK did not contain `vulkaninfo.exe`. No scientific fixture was generated, the Q6CB causal harness was not launched, and no outcome was observed.
One infrastructure-only repair was applied: the runner now uses the already-frozen zero-science SA0 Vulkan capability probe, with exact source/build-tool blobs bound by authorization revision 3. Q6CB fixtures, seeds, thresholds, mechanism rules, causal harness and shaders are unchanged.
The single Q6CB-2 infrastructure repair allowance is now consumed (1/1). Any subsequent F0 in this stage requires `STOP_INFRASTRUCTURE_UNSTABLE`; no second repair is permitted.
Q6CB-3 remains unauthorized.


## 2026-09-21 — Q6CB-2 terminal: STOP_INFRASTRUCTURE_UNSTABLE
The repaired Q6CB-2 runner failed a second time during pre-science environment validation. SA0 capability BuildOnly passed, but the zero-science capability probe exited with `requested Vulkan device substring not found` before any Q6CB fixture was generated or the Q6CB causal harness was launched.
The stage had already consumed its single permitted infrastructure repair after the prior `vulkaninfo.exe unavailable` F0. Per `config/q6cb_termination_goal_alignment_v0.1.json`, a second F0 in the same stage mandates `STOP_INFRASTRUCTURE_UNSTABLE`.
Static diagnosis indicates the repaired runner supplied `Arc 140V` to a contiguous substring matcher while the previously qualified Vulkan device name is `Intel(R) Arc(TM) 140V GPU`. This explanation is recorded but intentionally not repaired because the repair budget is exhausted.
No scientific Q6CB-2 observation exists; F1-F6 are not adjudicated. Q6CB-2 authorization is revoked, Q6CB-3 remains closed, and no further Q6CB execution or renamed rescue continuation is permitted.


## 2026-09-21 — ArcLLM program-level decision: STOP_CURRENT_SUCCESSOR_LINE
The program-level review considered the full ArcLLM chain rather than treating Q6CB infrastructure termination as a causal result.
The legacy architecture remains closed at `FEASIBLE_NO_DEMONSTRATED_ADVANTAGE`. SA-H1 remained the only bounded successor candidate. SA0 established target capability, SA1 established a strong Q4 component PASS, but the unchanged Q6 mechanism failed correctness and SA1 explicitly denied target-model integration or an end-to-end claim.
Q6CB then terminated `STOP_INFRASTRUCTURE_UNSTABLE` before any valid scientific fixture, so it produced no causal mechanism result and cannot unlock the governance-required successor intervention gate.
Because the preregistered Q4/Q6 successor scope is incomplete and no supported causal result / SI-1 correctness / component-value chain exists, `RETURN_TO_END_TO_END_VALIDATION` is not admissible. The current SA-H1 successor line is therefore closed.
The Q4 component PASS, SA0 capability PASS and original ArcLLM E2E feasibility remain valid bounded evidence; they are not promoted into a complete successor architecture claim.
No current successor implementation, Q6CB execution, Q3 reopen or successor E2E validation is authorized.

## 2026-10-01 — CORE-0E one-shot collection: STOP_CORE0E_COLLECTION_INCOMPLETE
The authorized one-shot CORE-0E collection executed all 24 frozen requests across 12 matched rows, but only 8 rows satisfied collection validity. Four llama.cpp `COMBINED_PHASE_TRACE` rows failed the preregistered strict marker-order requirement; in each failed row `prefill_wall_end_ns == decode_wall_start_ns`, while the two llama combined rows that passed strict order had a 100 ns observed boundary gap.

Because strict marker ordering was frozen before measured execution, equality cannot be reclassified post hoc. The collection is therefore formally adjudicated `STOP_CORE0E_COLLECTION_INCOMPLETE`. E1–E5 phase attribution and the phase Amdahl priority gate are not opened, no phase winner is selected, and measured timing magnitudes from this failed collection are not admissible for recovery design. The 24-request authorization is consumed; rerun or selective replay under the same authorization is forbidden. Any continuation requires a separately preregistered recovery/successor study with fresh evidence.
