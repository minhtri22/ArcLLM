# ArcLLM v1 — I003-MEB Bounded Matched-External Package

**Date:** 2026-09-24  
**Program:** `I003-MEB`  
**Status:** STATIC IMPLEMENTATION COMPLETE / LOCAL ZERO-SCIENCE PREFLIGHT REQUIRED / FRESH COMPARISON LOCKED

## 1. Purpose

This package compares the closed I002 ArcLLM candidate against the exact pinned llama.cpp v0.4.1 Vulkan baseline on the same model/workloads.

It does not introduce a new kernel or optimize the candidate.

## 2. Candidate benchmark

`src/arcllm_v1_i003_candidate_benchmark.cpp`

Derived from the exact Q2 production benchmark with only:
- closed I002 56 gate/up decode substitutions;
- one warmup + one measured inference per process;
- I003 evidence schema/identity fields.

Prefill remains byte-identical to Q2.

## 3. Baseline benchmark

`baseline/i003_llama_adapter.cpp`

Derived from the exact Q2 llama adapter.

Runtime contract remains:
- llama.cpp v0.4.1;
- commit `b29c606...`;
- Vulkan;
- F32 K/V;
- context 4096;
- batch/ubatch 256;
- 8 CPU threads;
- full GPU offload requested;
- raw token IDs;
- greedy;
- fixed 32 outputs.

Only the repetition wrapper changes to one warmup + one measured inference per process.

## 4. Paired execution

Four cells:
- A/W-S
- A/W-C
- B/W-C
- B/W-S

Five adjacent pairs per cell.

A alternates candidate-first beginning with candidate.
B alternates beginning with llama.

Total: 20 matched pairs / 40 measured inferences.

Each process performs its own warmup before its single measured attempt.

## 5. DEV_HOST

Before each arm, runner records:
- CPU load;
- free/total memory;
- power scheme;
- top processes.

Ambient load never vetoes an otherwise valid run.

Full collection rerun is permitted under identical frozen payload; selective pair/cell rerun is not.

## 6. Resource measurement

The exact Q2 Windows sampler is reused:
- 100 ms process sampling;
- working set/private bytes;
- CPU utilization;
- conditional GPU counters.

Only the measured attempt marker window contributes to primary resource statistics.

## 7. Zero-science preflight

`run_arcllm_v1_i003_preflight.ps1` performs:
- critical-blob verification;
- Python syntax/static QA;
- exact Q2 + SA1 candidate shader compile;
- candidate native build;
- exact pinned llama.cpp build/API qualification;
- exact model SHA/size verification;
- llama runtime `--qualify-only` for W-S/W-C;
- environment identity check;
- executable/SPIR-V fingerprinting;
- preflight evidence bundle.

Baseline qualification may load the model but executes no measured inference and no decode collection.

## 8. Fresh science gate

No I003 science authorization is included in this package.

`run_arcllm_v1_i003.ps1` fails closed unless a later committed:

`config/arcllm_v1_i003_science_authorization.json`

exists and binds the independently adjudicated preflight executable/provenance hashes.

## 9. Next

Run the zero-science preflight locally and return:

`arcllm_v1_i003_preflight_return_to_chatgpt.zip`

Only after independent PASS adjudication may the 40-inference matched collection be authorized.
