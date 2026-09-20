# ArcLLM Q1 Contract — real-model end-to-end 7B feasibility

Date: 2026-09-20

## Governance position

This is the first main-path study after P8-G6 under `docs/ARC_LLM_RESEARCH_GOVERNANCE.md`.

It serves **Q1 — Feasibility** only:

> Can ArcLLM execute the exact frozen 7B model end-to-end on the target hardware?

This is not a microbenchmark, architecture-optimization study, correctness-metric study, Q2 performance benchmark, Q3 regime-advantage claim, tokenizer/API validation, or productization run.

The pre-governance P8-G6 suggestion to open another correctness-metric qualification is superseded for future planning by the active project governance. P8-G6 evidence itself is not modified.

## Parent evidence

P8-G6 is COMPLETE and diagnostic-valid.

Frozen P8-G6 classification:

`H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED`

Authoritative P8-G6 SHA256:
- shader provenance: `1490475D0D7EAA0498FEEA5CD0A37460C4881FFFF676A7C912E0E113E2CAAC84`
- fresh production-semantic result: `0526D3F1400080AF6B68D1D44CBD2AF897671B727EA924EC0D0C953C16FDD259`
- summary: `13C36E5EB14D08F60C3DC9277F7A21EE4033DB50D84806839DE62B3E9F7EE303`

P8-G6 confirmed on fresh IDs {73,89,107,131}:
- C0 PASS 4/4;
- C1 production GPU vs R0 PASS 4/4;
- C2 production-semantic decomposition closure PASS 4/4;
- C3 local/state <=5% PASS 4/4;
- structural_valid=true.

This resolves the bounded L3 FFN-down production-semantic blocker. It does not itself establish whole-model inference.

## Frozen target

Exact model:
- logical target: local Ollama Qwen2.5-Coder 7B Q4_K_M model layer;
- SHA256: `60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463`;
- bytes: `4,683,074,048`;
- architecture: qwen2;
- layers: 28;
- hidden: 3584;
- q_heads: 28;
- kv_heads: 4;
- head_dim: 128;
- ffn: 18944;
- vocab: 152064;
- max resident KV capacity: 4096 positions.

Frozen P8 residency:
- 19 physical weight arenas;
- 341 physical tensor pieces;
- exactly two segmented logical tensors: `token_embd.weight` and `output.weight`;
- total planned resident bytes: 5,347,770,372;
- usable planned Vulkan budget: 16,374,562,816 bytes.

No substitute model is allowed.

## Q1 execution boundary

Q1 model execution begins at **pretokenized token IDs** and ends at **autoregressively generated token IDs**.

Tokenizer/detokenizer, chat templates, HTTP/OpenAI API and user-facing text transport are outside Q1 and remain P9 concerns.

This boundary is deliberate: Q1 tests the complete model compute/runtime path without adding a tokenizer implementation as an unrelated blocker.

## Frozen input

Exactly one prefill sequence of four valid vocabulary IDs:

`[1, 133151, 133152, 152062]`

Reasons for freezing this sequence before execution:
- all IDs are within vocab [0,152063];
- it crosses the exact segmented embedding boundary between rows 133151 and 133152;
- it also exercises a high-vocabulary row;
- semantic language quality is not a Q1 criterion.

Prefill:
- sequence length = 4;
- positions = 0,1,2,3.

Decode:
- greedy argmax only;
- exactly 4 generated tokens;
- decode positions = 4,5,6,7;
- temperature/sampling/randomness = none.

Generated token IDs must come only from the model's actual full-vocabulary logits. No teacher-forced decode token is permitted.

## Frozen production graph

Inherit the P7 production architecture and P8 segmented resolver.

Every run must execute:
1. segmented Q4_K token embedding;
2. all 28 decoder layers in order;
3. GPU-resident K/V writes and cached attention;
4. final RMSNorm;
5. full segmented Q6_K output projection for all 152064 logits;
6. greedy argmax;
7. four sequential cached decode steps feeding each generated token back through the segmented embedding path.

Frozen production operators:
- direct packed Q4_K/Q6_K weights;
- no full-model weight expansion;
- tiled Q/K/V/O projections;
- P7-L fused Q4_K gate+up where tensor type permits;
- separate SwiGLU;
- P7-G FFN-down tile16;
- online prefill attention;
- cached decode attention;
- GPU-resident KV.

CPU is allowed only for:
- orchestration;
- greedy argmax over returned logits;
- invariant checks;
- timestamps/resource logging;
- hashing/evidence serialization.

CPU model-math fallback or CPU teacher forcing is forbidden.

## Frozen dispatch census

Inherited P7 production graph census:
- one full prefill = exactly 441 compute dispatches;
- each cached decode step = exactly 469 compute dispatches.

Q1 therefore requires:
- prefill dispatches = 441;
- decode step 0 = 469;
- decode step 1 = 469;
- decode step 2 = 469;
- decode step 3 = 469.

Any hidden fallback, layer skip, duplicate layer or altered production graph that changes this census invalidates the run unless an implementation defect in the census instrumentation itself is demonstrated.

## Two independent executions

The executable must perform exactly two independent end-to-end executions, A and B, inside one invocation.

Before B:
- reset KV contents/state;
- reset position state;
- reset all transient activations;
- reload/reinitialize input token sequence;
- do not reuse generated token IDs from A as inputs to B.

Both executions use the same frozen model, graph and input.

This is a deterministic feasibility repeat, not the later Q3 fresh reproduction.

## F0 — structural/runtime validity

Each execution must satisfy:
- exact target size/SHA;
- exact 28-layer metadata;
- exact 19-arena / 341-piece resolver;
- exact two segmented logical tensors;
- full resolved-span coverage;
- selected Vulkan memory type retains DEVICE_LOCAL | HOST_VISIBLE | HOST_COHERENT;
- all required weight, KV and working buffers coexist;
- actual Vulkan allocations remain <= 16,374,562,816 bytes;
- all 28 decoder layers execute exactly once per prefill and exactly once per decode step;
- dispatch census matches 441 / 469;
- no CPU model-math fallback;
- no layer skipped;
- no non-finite activation required by downstream execution;
- no out-of-bounds token, tensor, KV or position access.

F0 failure makes Q1 invalid or not established according to failure type below.

## F1 — full-logit / autoregressive validity

For prefill and every decode step:
- full logits count = 152064;
- all logits finite;
- selected greedy token ID is within [0,152063];
- selected token is the actual argmax of that full logit vector;
- selected token is fed into the next decode step;
- KV position advances monotonically from prefill positions 0..3 through decode positions 4..7.

No manual token substitution is permitted.

## F2 — deterministic repeat

Executions A and B must produce exactly the same four generated token IDs.

Also record per step, descriptively:
- top-1 logit;
- top-2 logit;
- top1-top2 margin;
- logits checksum/hash;
- final hidden-state checksum/hash.

Raw floating-point checksum equality is recorded but is not a separate PASS requirement; exact generated token sequence equality is the frozen deterministic gate.

## F3 — evidence completeness

Required evidence must contain:
- implementation commit SHA;
- exact model SHA/size;
- environment/OS/Vulkan/device fingerprint;
- driver information when available;
- exact input token IDs;
- generated token IDs for A and B;
- per-step dispatch census;
- per-step full-logit finiteness and argmax validation;
- KV positions and capacity;
- resident/allocation bytes;
- failure record if any;
- descriptive wall-clock timestamps;
- descriptive CPU/GPU utilization if available without adding a new instrumentation dependency;
- raw result JSON;
- summary JSON;
- shader provenance;
- evidence manifest.

Q1 performance numbers are descriptive only and MUST NOT be used for Q2/Q3 claims.

## Q1 adjudication

### Q1_FEASIBILITY_ESTABLISHED

Require all:
- F0 PASS for A and B;
- F1 PASS for A and B;
- F2 exact generated-token repeat PASS;
- F3 evidence complete.

Interpretation:
The exact 7B model executes end-to-end through prefill + autoregressive decode on the target Arc runtime under the frozen P8 memory architecture.

This does not establish performance advantage.

### Q1_REPRODUCIBILITY_FAILURE

Use only if:
- both A and B complete valid full-model execution;
- F0/F1 pass;
- generated token sequences differ.

Do not tune after observing the mismatch. Investigate only if the mismatch directly blocks Q1 validity.

### Q1_EXECUTION_NOT_ESTABLISHED

Use when:
- package/build/environment is known-good enough to start the frozen execution;
- the intended full-model path reproducibly cannot complete because of a runtime/resource limitation;
- the failure is not merely a packaging or measurement defect.

A single bounded repair is permitted only when a concrete blocker is identified, consistent with governance STOP-A.

### Q1_INVALID

Use when:
- exact target/evidence invariants are not satisfied;
- intended execution path was not actually run;
- measurement/packaging defect prevents scientific adjudication.

Infrastructure failure must not be mislabeled as scientific Q1 failure.

No other classification is permitted.

## Explicit non-goals

Q1 does not:
- compare performance with Ollama/llama.cpp/another baseline;
- claim faster/slower/better;
- tune kernels;
- introduce new architecture variants;
- optimize attention/decode;
- change quantization;
- implement tokenizer/API;
- redefine historical P8-G gates;
- use synthetic kernel evidence as substitute for this run.

## Governance carry-forward

Historical outcomes remain frozen:
- P8-G = FAIL;
- P8-G1 = H-AMPLIFICATION;
- P8-G2 = H-NONLINEAR/UNEXPLAINED;
- P8-G3 = H-FP32-ACCUMULATION;
- P8-G4 = H-LOCAL-ERROR-NONNEGLIGIBLE;
- P8-G5 = H-R1-ORACLE-SEMANTIC-MISMATCH;
- P8-G6 = H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED.

No historical result is rewritten.

## Stop / next rule

If Q1_FEASIBILITY_ESTABLISHED:
- stop subsystem correctness expansion;
- next = Q2 frozen matched performance/resource benchmark.

If Q1_EXECUTION_NOT_ESTABLISHED:
- identify whether one concrete implementation/runtime blocker exists;
- permit at most one bounded blocker repair before STOP-A adjudication.

If Q1_REPRODUCIBILITY_FAILURE:
- treat determinism as a Q1 blocker; do not proceed to Q2.

No additional microbenchmark or correctness-metric phase may be opened unless the Q1 end-to-end run itself identifies a direct blocker and the governance promotion conditions are met.
