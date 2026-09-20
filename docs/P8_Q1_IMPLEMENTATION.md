# ArcLLM Q1 Implementation — full 7B end-to-end integration

Date: 2026-09-20

## Scope

Implements the frozen Q1 feasibility contract only.

The implementation composes already-qualified components rather than introducing a new model architecture:
- P8 19-arena / 341-piece graph resolver;
- P8-C segmented Q4_K embedding;
- P7/P8 production prefill operators;
- P7 cached decode operators and GPU-resident KV;
- one integration shader for full segmented Q6_K LM-head chunks.

Tokenizer, API, matched baseline, performance tuning and Q2/Q3 remain out of scope.

## Pre-run clarification

Before target execution, the design text was clarified:
- prefill logits predict position 4;
- four cached decode steps at input positions 4..7 predict positions 5..8;
- total generated token IDs = 5, not 4.

This is a semantic correction made before evidence, not an outcome-dependent gate change.

The residency wording was also aligned to P8-B evidence: Q1 records requested bytes and requires them to remain inside the already-proven P8-B envelope; it does not fabricate an actual-allocation metric unavailable from the inherited Vulkan wrapper.

## Files

- `src/q1_end_to_end.cpp`
- `shaders/p8q1_lmhead_q6k_segmented_chunk.comp`
- `tools/compile_q1_shaders.ps1`
- `tools/build_q1.ps1`
- `run_q1.ps1`
- `tests/test_q1_package.py`
- `docs/P8_Q1_END_TO_END_CONTRACT.md`

## Full execution

Frozen input IDs:
`[1,133151,133152,152062]`

Each independent execution:
1. reset K/V and all transient buffers;
2. segmented embedding;
3. all 28 decoder layers prefill;
4. output norm;
5. full 152064-row segmented LM head;
6. greedy argmax -> generated token #1;
7. four cached decode steps at positions 4..7, each feeding the previous actual argmax token;
8. each decode step executes all 28 layers, output norm and full segmented LM head.

Two executions A/B are performed in one invocation.

## Dispatch invariants

Prefill:
`1 embedding + 28*15 layer ops + 1 output norm + 19 LM-head chunks = 441`.

Cached decode:
`1 embedding + 28*16 layer ops + 1 output norm + 19 LM-head chunks = 469`.

The executable rejects any plan with a different census.

## Segmented endpoints

Embedding reuses `p8c_embedding_q4k_segmented_probe.comp`, which already accepts arbitrary valid token IDs and maps the exact boundary at row 133152.

The new `p8q1_lmhead_q6k_segmented_chunk.comp`:
- binds both exact output.weight segments;
- computes rows by global vocabulary row;
- maps rows across boundary 91304;
- writes each result to its global logits index;
- supports chunked full-vocabulary execution;
- uses the production-style sequential FP32 accumulation semantics.

No output-weight re-encoding or merged >256 MiB allocation is introduced.

## Evidence contract

`run_q1.ps1`:
- verifies the exact model SHA/size;
- verifies byte-exact P8-G6 authoritative parent artifacts;
- verifies critical Q1 files match HEAD while allowing unrelated local modifications;
- runs static package QA;
- compiles pinned glslang 16.5.0 shaders;
- builds the native executable;
- records OS/CPU/GPU/driver/Vulkan environment;
- executes A/B;
- creates raw result, summary, console log, environment JSON, shader provenance and evidence manifest;
- packages return artifacts as `results/q1_return_to_chatgpt.zip`.

The executable adjudicates F0/F1/F2 only. The runner completes F3 and only then emits a final Q1 classification.

## Fail-closed rules

- Any exact-target or parent-evidence mismatch stops before model execution.
- Any critical Q1 worktree/index drift stops before execution.
- No CPU model-math fallback exists in the Q1 execution path.
- No teacher-forced decode token is accepted.
- Q1 performance timings are descriptive only.
- Runtime/package failure does not automatically become `Q1_EXECUTION_NOT_ESTABLISHED`; that scientific label requires post-evidence adjudication that the intended known-good execution reproducibly hit a real runtime/resource obstruction.

## Static implementation review

PASS criteria checked before target authorization:
- exact target architecture/constants present;
- exact input IDs and 5-token semantics present;
- two independent reset executions present;
- full 28-layer prefill + decode loops present;
- segmented embedding and full segmented LM head present;
- 441/469 census frozen in code and test;
- all required production shaders pinned;
- CPU model fallback and teacher forcing forbidden by execution design;
- P8-G6 parent hashes frozen in runner;
- F3 evidence package generated before final PASS classification;
- Q2/Q3 remain closed.

Status: **IMPLEMENTATION_LOCKED / TARGET RUN AUTHORIZED**.
