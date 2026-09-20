# ArcLLM Q2 — Formal Adjudication

Date: 2026-09-20

## Formal classification

**Q2_MATCHED_CHARACTERIZATION_COMPLETE**

This closes Q2 validity only. Q2 remains characterization-only: it does not declare a winner and does not establish a regime advantage.

## Evidence identity

- implementation commit: `43afd71161c4dc8c766c09c3b55d5eca48352bde`
- execution authorization commit: `8dca75dd5e0fba74fa75ccb3330313c9690f7f63`
- returned bundle SHA256: `A802BFA44FE7FEE5723B11E90013B23E1B0F42E51DA877E5889557326726F730`
- returned bundle bytes: 144,915
- evidence manifest SHA256: `559B58ED1E262EBA72D5F162540800D61539B7E98351CB65C22277808265FE95`
- summary SHA256: `719576899733C78A9FA128E8F7E57FCE06104287F983A8CCE94D9B7673854381`
- preflight lock SHA256: `3973BE933EE27E93803171856189B0406D56F35E6745E8520CD3EFE030153BEB`
- execution authorization SHA256: `248886131ACC5AA144329977E45AE91304C8DB408316908AB571165F70586450`

The exact target SHA/size, Q1 archive, baseline release/tag commit, ArcLLM executable, baseline executable, shader provenance, workload hashes and authorization chain remain bound to the preflight lock.

## Validity adjudication

All four frozen cells contain exactly five measured attempts and all 20/20 measured attempts succeeded. Every successful attempt records exactly 32 generated tokens with finite final logits. ArcLLM dispatch census passes on every measured attempt.

The pinned llama.cpp baseline is exact `v0.4.1` at annotated-tag commit `b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`, Vulkan, F32 K/V, exact raw-token prompts, context 4096, batch/ubatch 256, 8 threads, and full offload 29/29.

Mandatory resource traces are complete for every attempt: process working-set peak, private-bytes peak, mean CPU and peak CPU are present, with process exit code 0 for all four cells.

Therefore every preregistered requirement for `Q2_MATCHED_CHARACTERIZATION_COMPLETE` passes.

## Frozen descriptive medians

| Workload | System | TTFT ms | Decode tok/s | E2E ms | Working set peak | Private bytes peak |
|---|---|---:|---:|---:|---:|---:|
| W-S | ArcLLM | 918.265 | 0.3184 | 98,285.231 | 10,017,341,440 | 5,346,062,336 |
| W-S | llama.cpp | 100.318 | 12.7449 | 2,535.868 | 5,467,303,936 | 5,518,123,008 |
| W-C | llama.cpp | 1,423.482 | 13.0354 | 3,803.029 | 5,481,332,736 | 5,529,026,560 |
| W-C | ArcLLM | 14,706.425 | 0.3990 | 91,043.729 | 10,006,102,016 | 5,343,809,536 |

Descriptive ArcLLM/baseline ratios:

- W-S: TTFT 9.154×, decode throughput 0.02499×, E2E 38.758×, working set 1.832×, private bytes 0.9688×.
- W-C: TTFT 10.331×, decode throughput 0.03061×, E2E 23.940×, working set 1.825×, private bytes 0.9665×.

These ratios are descriptive only and are not a Q2 winner/advantage adjudication.

## Evidence limitations that do not invalidate Q2

1. Windows GPU Engine peak counters exceed 100% in some ArcLLM samples. The contract already makes GPU counters conditional. GPU-utilization peak is therefore marked unreliable and excluded from Q2 validity and any future advantage claim unless a new Q3 instrument explicitly fixes/requalifies it.
2. `q2_run_meta.cell_exit_codes` captured child stdout plus the trailing numeric exit code because of PowerShell pipeline semantics. This is a metadata-shape defect, not an execution ambiguity: each resource trace independently records `process_exit_code=0`, all 20 results are PASS, and all cell logs terminate successfully.
3. The environment snapshot lists idle Ollama processes. The Q2 resource sampler is bound to each benchmark child PID; the evidence contains no indication of concurrent Ollama inference.

## Post-Q2 design input

The complete matched table does not expose a plausible practical advantage in either frozen regime on the primary performance metrics or working-set memory. ArcLLM's private-bytes median is only about 3% lower while the primary timing/throughput and working-set metrics move strongly in the opposite direction. Lower CPU utilization is descriptive but cannot by itself establish a practical advantage under the observed throughput gap.

This statement is input to Q3 design only. It is not a Q2 winner declaration.

## Governance decision

Q2 is closed as `Q2_MATCHED_CHARACTERIZATION_COMPLETE`.

No ArcLLM tuning, kernel optimization, workload search or new microbenchmark is authorized between this Q2 closeout and the Q3 design lock.

Q3 execution remains closed. The next scientific step is to freeze a Q3 **no-practical-advantage confirmatory** design with explicit practical-effect thresholds and a fresh-reproduction contract.
