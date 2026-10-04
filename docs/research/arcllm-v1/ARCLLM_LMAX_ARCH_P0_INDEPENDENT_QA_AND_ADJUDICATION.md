# ARCLLM_LMAX_ARCH_P0 — Independent QA and Adjudication

Gate: `ARCLLM_LMAX_ARCH_P0_INDEPENDENT_QA_AND_ADJUDICATION`

Formal verdict: **UNRESOLVED**

## Independent QA

Frozen campaign evidence contained 52 records: 4 excluded warmups and 48 measured inference runs (24 matched A/B pairs). Pair order was exact and all recorded runs completed with return code 0 and `success=true`.

Semantic gate: **PASS**.

- 24/24 matched pairs checked.
- Generated token IDs matched exactly.
- Generated token counts matched.
- `finite=true` for both arms.
- Prefill/decode dispatch and submission topology matched.
- Route/lifecycle counters matched.
- No asymmetric runtime error.
- Independent ITL recomputation from `token_ready_ns`: exact.
- Independent decode-throughput recomputation: maximum absolute error `5.551115123125783e-16`.

## Performance diagnostics from stored derived metrics

Using linear-interpolated p50/p95 aggregation, several cells showed >=5% candidate improvements before guards:

- W2 TTFT.
- W4 TTFT, E2E latency and decode throughput.
- W5 E2E latency and decode throughput.

However `E2E_SUPPORTED` is not reached because frozen guards fail:

- TTFT p95 regression >3% in W3: ~9.80%.
- ITL p95 regression >3% in W1 (~5.37%), W3 (~7.49%) and W6 (~3.52%).
- CPU utilization regressed >5% in all six workload cells. Median CPU regressions were approximately 35.9% to 61.0%.

`REGRESSION` is not reached. Under linear p95 interpolation, 0/6 cells meet the >10% regression criterion; under nearest-rank p95, only W3 does, still below the frozen minimum of 4/6.

Request-wide and decode-window instrumented allocation counts were identical between arms in every workload cell.

If the stored derived metrics alone were accepted as complete evidence, the diagnostic category would be `NO_SUPPORTED_BENEFIT`.

## Formal QA blockers

Formal QA does **not** PASS for two preregistration-contract reasons:

1. The preregistration required per-request serialization of request start, request completion, process CPU start and process CPU end. The frozen observation schema serializes only derived `ttft_ns`, `e2e_ns` and `cpu_utilization_percent`; therefore independent primitive recomputation of TTFT/E2E/CPU is impossible from the frozen evidence.
2. The preregistered 1,000,000-event zero-model control-path subtest was not executed/present in the frozen evidence. Therefore `CONTROL_PATH_ONLY` cannot be independently evaluated.

No inference rerun, selective rerun, post-hoc control-path run, threshold change or tuning was performed during QA.

## Adjudication

Because semantic evidence is valid but the frozen evidence package is incomplete for the full preregistered decision tree, the formal bounded verdict is:

`UNRESOLVED`

This does not support an ArcLLM LMAX speedup claim, a control-path-only benefit claim, or a regression claim. The diagnostic readout `NO_SUPPORTED_BENEFIT` is not promoted to the formal scientific verdict.

Frozen artifacts:

- Independent checker SHA-256: `5c0c148564bd9b423eea46918a23dabf8f22fd3202e81b2867e0b766f01ad68c`
- Independent QA result SHA-256: `12c79e940b01afae248aa4205425b5557c57833cc5be4fb6d65b1b78e75d6526`
- Raw evidence SHA-256: `ed3ed2659a9ad75a58db2d4f7dae537f7aff5e7bc364a5a7a01f0536b01941f4`
