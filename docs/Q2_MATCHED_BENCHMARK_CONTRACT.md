# ArcLLM Q2 Contract — matched performance / resource characterization

Date: 2026-09-20
Status: DESIGN_FROZEN

## Scientific role

Q2 serves only:

> Under frozen matched workloads on the target machine, what performance and resource envelope does ArcLLM exhibit relative to a strong matched baseline?

Q2 is a characterization study, not an advantage claim.

Q2 MUST NOT declare ArcLLM faster, better or more efficient. Any candidate advantage observed here can only be promoted into a separately frozen Q3 claim and then fresh-reproduced.

## Parent

Q1 verdict:
`Q1_FEASIBILITY_ESTABLISHED`

Q1 implementation:
`ec83bf42727f799e31d3900a7545e2642b3b90eb`

Authoritative Q1 evidence archive SHA256:
`DFB3E86C4F51D06290A2AE1ED1479C96F35AE0D329CFA5E2B7D50289DC641B43`

Q1 performance timings are not Q2 measurements and are excluded from Q2 comparison.

## Target hardware

Frozen target machine:
- Windows 11 x64;
- Intel Core Ultra 7 258V, 8C/8T;
- Intel Arc 140V GPU (16GB family);
- 32 GB system RAM;
- Intel GPU driver 32.0.101.8860 as observed at Q1.

If the GPU driver or relevant hardware changes before Q2, record the change and do not silently merge the results with this contract.

## Model

Both systems must load the exact same model bytes:
- SHA256 `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`
- bytes `4,683,074,048`
- Qwen2.5-Coder 7B Q4_K_M Ollama GGUF blob.

No conversion, requantization or replacement model is permitted.

## Systems

### A — ArcLLM

Use the Q1 production architecture:
- P8 19-arena / 341-piece residency;
- segmented embedding/output;
- all 28 layers;
- GPU-resident FP32 KV;
- frozen production kernels;
- full vocabulary logits;
- greedy argmax;
- no CPU model-math fallback;
- no teacher forcing.

Q2 implementation may generalize buffer sizes, sequence length, decode-count and measurement instrumentation only.

It MUST NOT tune or replace production kernels before Q2 outcome.

### B — matched baseline

Baseline:
- repository: `ggml-org/llama.cpp`;
- release: `v0.4.1`;
- source commit: `391fac16460f15233a7740550d858ac96df3419d`;
- backend: Vulkan;
- exact same GGUF bytes;
- one sequence;
- context capacity 4096;
- all model layers requested for GPU offload;
- CPU threads = 8;
- batch = 256;
- ubatch = 256;
- KV cache K = F32;
- KV cache V = F32;
- greedy generation;
- no speculative decoding;
- no prompt cache reuse between measured repetitions;
- no early stop on EOS for the frozen fixed output count.

Use the pinned upstream defaults for other optimizations. Record the resolved build options, Vulkan device, actual layer offload and any fallback reported by llama.cpp.

The baseline must accept the same raw token IDs. Tokenizer or chat-template differences are not permitted to enter the matched benchmark.

If this exact baseline cannot be built or cannot accept the exact GGUF/raw-token workload, adjudicate `Q2_BASELINE_NOT_MATCHED`; do not replace it after seeing ArcLLM performance.

## Workloads

Two workloads are frozen before execution.

### W-S — short/decode-dominant

Prompt length = 4.

Exact token IDs:
`[1,133151,133152,152062]`

Generated token count = exactly 32:
- token #1 from prefill full logits;
- 31 cached autoregressive decode steps.

### W-C — context/prefill-sensitive

Prompt length = 256.

For positions 0..3:
`[1,133151,133152,152062]`

For each position `i=4..255`:
`token[i] = 1 + ((104729 + 7919*i) mod 152063)`

All IDs are interpreted directly as vocabulary IDs.

Generated token count = exactly 32:
- token #1 from prefill;
- 31 cached decode steps.

No EOS early termination.

Both workloads remain far below the frozen KV capacity of 4096.

## Warmup / repetitions

For each system × workload cell:
- 1 complete warmup inference;
- reset KV/state;
- 5 measured inference attempts.

A failed measured attempt is evidence and counts toward stability/error rate; it must not be silently replaced.

Execution order:
- W-S: ArcLLM cell first, baseline cell second;
- W-C: baseline cell first, ArcLLM cell second.

This reverses system order across the two workload regimes.

No performance threshold is changed after outcomes.

## Timing definitions

Model loading, shader compilation and executable startup are excluded from primary inference timing but recorded separately as descriptive setup metrics.

### TTFT

From immediately before submission/execution of the frozen prompt prefill to availability of the full-vocabulary logits and greedy token #1.

Unit: milliseconds.

### Decode throughput

For generated tokens #2..#32 only:

`decode_tps = 31 / decode_elapsed_seconds`

where decode_elapsed begins immediately before cached decode for token #2 and ends when token #32 argmax is available.

### End-to-end latency

From the same prefill start used for TTFT through availability of generated token #32.

Unit: milliseconds.

## Resource metrics

Sampling interval target: 100 ms.

Mandatory per measured attempt:
- process working set peak;
- process private bytes peak;
- mean CPU utilization;
- peak CPU utilization;
- success/error status.

Where Windows exposes valid per-process GPU counters for both systems, also record:
- peak GPU dedicated usage;
- peak GPU shared usage;
- mean GPU compute utilization;
- peak GPU compute utilization.

GPU counters are conditionally required: absence/unreliability must be explicitly recorded and cannot be silently replaced with a fabricated estimate.

ArcLLM additionally records descriptive architecture counters:
- requested resident weight bytes;
- requested KV bytes;
- working bytes;
- CPU reads of full logits;
- CPU token-ID writes;
- any explicit host/device transfer introduced by Q2 instrumentation.

These Arc-only counters are not by themselves matched advantage metrics.

## Stability

For each system × workload:
- attempts = 5;
- successful attempts;
- failed attempts;
- error rate.

If a run crashes or returns invalid/non-finite logits, record the failure exactly.

Do not rerun a failed measured repetition unless an independently identified measurement/infrastructure defect made that repetition invalid. A genuine runtime failure remains data.

## Summary statistics

For each timing/resource metric with valid samples:
- median;
- minimum;
- maximum;
- MAD (median absolute deviation).

Do not report p95 from five samples.

Comparison output may report ArcLLM/baseline ratios, but Q2 must label them descriptive.

## Output-token recording

For every attempt record:
- 32 generated token IDs;
- generated-token sequence hash;
- final logits finite status;
- final hidden/state checksum if available.

ArcLLM and baseline are not required to generate identical continuations. Same prompt, exact model bytes, greedy rule and fixed output length are the matched workload; numerical/runtime implementation differences may change tokens.

## Environmental controls

Before each cell:
- no concurrent LLM inference process;
- no unrelated GPU workload intentionally running;
- AC power connected;
- record active Windows power scheme;
- record OS, CPU, GPU, driver;
- record git/source commit and build hashes;
- record baseline build configuration.

Do not change driver, power mode or kernel configuration between systems within the study.

## Evidence

Required:
- frozen Q2 contract;
- ArcLLM implementation SHA;
- baseline repository/release/commit and build fingerprint;
- exact model SHA/size;
- exact workload token materialization/hash;
- warmup records;
- all 20 measured attempt records (2 systems × 2 workloads × 5);
- raw resource samples or compressed trace;
- per-cell summaries;
- matched-comparison summary;
- environment fingerprint;
- logs/failure records;
- evidence manifest.

## Q2 validity classifications

Exactly one:

### Q2_MATCHED_CHARACTERIZATION_COMPLETE

Require:
- exact model/input/runtime invariants preserved;
- both systems actually execute the intended workloads;
- all 5 measured attempts are recorded in every cell;
- at least 3 successful attempts in every cell;
- primary timing metrics valid for successful attempts;
- mandatory RAM/CPU resource measurements valid;
- baseline matching requirements satisfied.

This classification says characterization is complete. It does not say ArcLLM wins.

### Q2_BASELINE_NOT_MATCHED

Use if the pinned baseline cannot execute the exact model/raw-token/matched configuration and the issue is not merely a correctable packaging/build defect.

No weaker replacement baseline may be selected after seeing ArcLLM performance.

### Q2_RUNTIME_INCOMPLETE

Use if a real runtime limitation leaves fewer than 3 successful measured attempts in any cell after the intended frozen implementation is confirmed.

### Q2_MEASUREMENT_INVALID

Use if timing/resource instrumentation or evidence packaging cannot support the frozen measurements.

Infrastructure/packaging defects may be repaired before adjudication when they do not change the performance path or frozen workload.

No other Q2 classification is permitted.

## No advantage adjudication in Q2

Q2 must not define a winner.

After `Q2_MATCHED_CHARACTERIZATION_COMPLETE`:
1. freeze Q2 evidence;
2. identify any candidate regime advantage or lack thereof from the complete matched table;
3. design Q3 with an explicit practical-effect threshold and fresh reproduction;
4. do not tune ArcLLM between Q2 observation and Q3 freeze.

If Q2 shows no plausible practical advantage in either frozen regime, Q3 may directly test a predeclared no-advantage boundary rather than search additional workloads.

## Governance

Q1 is closed and cannot be reopened by Q2 performance results.

Q3 remains blocked until Q2 is adjudicated.

No new subsystem optimization/microbenchmark is authorized unless Q2 itself demonstrates a bottleneck that directly blocks Q2/Q3 and the six architecture-experiment conditions in governance are satisfied.
