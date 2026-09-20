# ArcLLM Successor Evidence Register

**Date:** 2026-09-20  
**Role:** immutable source register for post-verdict architecture review.

## ArcLLM sources

- `inputs/q3_formal_result.authoritative.json` — final 40-attempt Q3 evidence.
- `docs/ARC_LLM_FINAL_VERDICT.md` — current architecture terminal verdict.
- `docs/P7_CLOSEOUT.md` — production-path kernel composition and P7-M dispatch/profile evidence.
- `src/q2_benchmark.cpp` — frozen real-model Q2/Q3 graph and dispatch census.

## NEXUS sources

Canonical main snapshot reviewed: `6ee72552159113e3103a84106d83e701a0b48eac`.

- `artifacts/R4B1/R4B1_RESULT_COMPACT.json`
- `artifacts/R4B2/R4B2_RESULT_COMPACT.json`
- `artifacts/R4B3/R4B3_RESULT_COMPACT.json`
- `artifacts/R4B4/R4B4_RESULT_COMPACT.json`
- `artifacts/R4B5/R4B5_RESULT_COMPACT.json`
- `artifacts/R4C/R4C_RESULT_COMPACT.json`
- `experiments/R5/R5_CALIBRATION_STOP_REVIEW.md`
- `experiments/R5/R5_1_STAGE_A_INDEPENDENT_FINAL_AUDIT_v0.3.md`

Event-ledger evidence reviewed from `research/h2-event-ledger-d64-layout`:

- `artifacts/H2_EVENT_LEDGER/L4/EL_L4_FORMAL_ADJUDICATION.json`
- `docs/research/h2-event-ledger/CROSS_BRANCH_CANONICAL_INTEGRATION_REVIEW.md`
- `artifacts/H2_EVENT_LEDGER/CROSS_BRANCH_CANONICAL_INTEGRATION_REVIEW.json`
- `docs/research/h2-event-ledger-d64-layout/CHARTER.md`
- `docs/research/h2-event-ledger-d64-layout/LD64_0_REVIEW.md`
- `artifacts/H2_EVENT_LEDGER_D64/LD64_1_FORMAL_ADJUDICATION.json`
- `docs/research/h2-event-ledger-d64-layout/LD64_2_MEASUREMENT_SANITY_POSTMORTEM.md`
- `artifacts/H2_EVENT_LEDGER_D64/LD64_2_TARGET_20260920_221424_ONE_SHOT_ADJUDICATION.json`

Important evidence-state distinction:
- EL-L4: confirmed positive under tested regimes.
- LD64-0/1: structural/semantic PASS only.
- LD64-2 performance: UNRESOLVED; must not be used as positive performance evidence.

## External publications/public papers

1. Hong et al., **FlashDecoding++: Faster Large Language Model Inference on GPUs**, arXiv:2311.01282.
2. Frantar et al., **MARLIN: Mixed-Precision Auto-Regressive Parallel Inference on Large Language Models**, arXiv:2408.11743.
3. Dao et al., **FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness**, arXiv:2205.14135.
4. Dao, **FlashAttention-2: Faster Attention with Better Parallelism and Work Partitioning**, arXiv:2307.08691.
5. Aminabadi et al., **DeepSpeed Inference: Enabling Efficient Inference of Transformer Models at Unprecedented Scale**, arXiv:2207.00032.
6. Kwon et al., **Efficient Memory Management for Large Language Model Serving with PagedAttention**, arXiv:2309.06180 / SOSP 2023.
7. Lin et al., **QServe: W4A8KV4 Quantization and System Co-design for Efficient LLM Serving**, arXiv:2405.04532.
8. Gale et al., **MegaBlocks: Efficient Sparse Training with Mixture-of-Experts**, MLSys 2023.
9. Fedus et al., **Switch Transformers: Scaling to Trillion Parameter Models with Simple and Efficient Sparsity**, JMLR 2022.
10. Song et al., **PowerInfer: Fast Large Language Model Serving with a Consumer-grade GPU**, arXiv:2312.12456.
11. Federici et al., **Efficient LLM Inference using Dynamic Input Pruning and Cache-Aware Masking**, MLSys 2025.
12. Ye et al., **FlashInfer: Efficient and Customizable Attention Engine for LLM Inference Serving**, MLSys 2025.

## Portability rule

Published speedup numbers are **not** expected ArcLLM speedups. Most external systems use NVIDIA/CUDA or different models, batches and serving regimes. They are cited only to support or falsify mechanism plausibility.
