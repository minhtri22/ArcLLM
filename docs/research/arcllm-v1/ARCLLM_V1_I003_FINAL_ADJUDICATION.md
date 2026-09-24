# ArcLLM v1 — I003-MEB Final Adjudication

**Date:** 2026-09-24  
**Primary collection:** `20260924T143132554Z_cf9a222c`  
**Primary bundle SHA256:** `2FE89BCCFA9FB1ABE5782D628BF860F0026334F2ADC0BBE69FFF7B8A65253FE6`

## Final classification

`I003_MATCHED_EXTERNAL_CHARACTERIZATION_COMPLETE`

Independent recomputation from the returned raw pair artifacts found:
- 20/20 matched pairs valid;
- 40/40 measured inferences represented;
- zero raw validity issues;
- exact counterbalanced pair order;
- all arm process exit codes zero;
- all measured attempts successful with finite logits and 32 generated tokens;
- candidate dispatch census PASS throughout;
- independent recomputation exactly matches the frozen summary.

Cross-system token identity is not a validity requirement in the frozen I003 specification. Each
system is internally deterministic across all five repetitions of each workload, while candidate
and llama continuations differ across systems as expected to be recorded by the specification.

## Fresh post-I002 practical gap

| Cell | TTFT candidate/llama | Decode latency candidate/llama | Decode throughput candidate/llama | E2E candidate/llama | Working-set candidate/llama | Private-bytes candidate/llama |
|---|---:|---:|---:|---:|---:|---:|
| A/W-S | 7.4993× | 8.8334× | 0.11321× | 8.7291× | 1.83610× | 0.97049× |
| A/W-C | 8.7584× | 9.2050× | 0.10864× | 9.1629× | 1.82875× | 0.96660× |
| B/W-C | 9.7039× | 13.0370× | 0.07671× | 11.5055× | 1.82880× | 0.96663× |
| B/W-S | 6.1645× | 10.9457× | 0.09136× | 10.7462× | 1.83609× | 0.97047× |

Global geometric means of cell medians:
- decode latency candidate/llama = **10.378702×**;
- E2E latency candidate/llama = **9.972199×**;
- decode throughput candidate/llama = **0.096351×**.

I003 is a descriptive matched-gap characterization, not a winner score.

## Scientific decision

The fresh post-I002 external gap remains large. Per the preregistered I003 decision boundary,
do **not** select LM-head, FFN-down, or any other mechanism from historical shares.

The next scientific program is a **fresh post-I002 exact device-work profile**. It must first
localize the current closed-I002 device work under the post-I002 implementation and only then
permit a new mechanism hypothesis.

No historical Q2 ratios are algebraically combined with this fresh collection.

## Closure

I003-MEB is CLOSED with
`I003_MATCHED_EXTERNAL_CHARACTERIZATION_COMPLETE`.

Any later complete I003 run is replication/robustness only and requires a fresh explicit
authorization after this primary authorization is closed.
