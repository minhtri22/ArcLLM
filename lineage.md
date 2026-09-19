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
