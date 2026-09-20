# ArcLLM Q3 — Formal Adjudication

Date: 2026-09-20

## Final verdict

**FEASIBLE_NO_DEMONSTRATED_ADVANTAGE**

Q1 established end-to-end feasibility. Q2 established a valid matched characterization. Q3 completed the preregistered two-session fresh reproduction and did **not** falsify the bounded no-practical-advantage hypothesis.

This is a valid negative scientific result. It is not `UNRESOLVED`.

## Evidence identity

- Q3 implementation commit: `032688ee1b188f5ef23c3b3d02466cbf2391eb96`
- Q3 execution authorization commit: `ae1e5b76d0b09081d514264990a3790c26bcf051`
- returned bundle SHA256: `8E19970779A96A68073F6F274A44FC41E6A0DDDB71156181FB23FB2BFC02DC15`
- bundle bytes: 257,323
- evidence manifest SHA256: `36CC5966E7C7EE5EB15A6FDFDF38FEF7C885B20D27726FA91D42AFCD8D93ED0B`
- candidate adjudication SHA256: `986A6BD43F91289AD4F1B02232FEC77036E647FF89ECFF377062F56CBA826D88`
- design SHA256: `6FA611161AF6D461BA46567AAE624B92764CD5F66848C1A32EAC99A9415B5573`
- preflight lock SHA256: `9105FA31E44029A63A316C13743F2FBB172D20F2C1022C091D70F25CFEDDF76E`
- execution authorization SHA256: `F47481E19F9EDD42544F4C4F2B3DB064708BCC2650833409DC84AA1FEECE129A`

The evidence manifest covers 56 packaged evidence files. Independent recomputation found zero manifest hash mismatches.

## Fresh-session validity

Session A runner PID = `40168`.  
Session B runner PID = `51840`.

The PIDs are distinct and the fixed counterbalanced orders were preserved:

- A: ArcLLM W-S → llama.cpp W-S → llama.cpp W-C → ArcLLM W-C
- B: llama.cpp W-S → ArcLLM W-S → ArcLLM W-C → llama.cpp W-C

Both sessions used the exact frozen model, pinned llama.cpp baseline, same GPU driver `32.0.101.8860`, same Balanced power scheme and AC Online state.

All eight cells completed their warmup. All **40/40 measured attempts succeeded**. Exact prompt identities, finite logits, 32 generated tokens, mandatory timing, working-set/private-bytes/CPU traces, process exit code 0 and ArcLLM dispatch census all validate.

Session-complete file hashes also independently validate.

## Independent threshold recomputation

| Session | Workload | TTFT Arc/Base | Decode Arc/Base | E2E Arc/Base | Working-set Arc/Base | Blocking-harm guard |
|---|---|---:|---:|---:|---:|---|
| A | W-S | 12.422× | 0.02473× | 39.209× | 1.836× | FAIL |
| A | W-C | 9.198× | 0.02172× | 31.355× | 1.829× | FAIL |
| B | W-S | 14.801× | 0.01761× | 55.009× | 1.836× | FAIL |
| B | W-C | 9.605× | 0.02800× | 26.451× | 1.829× | FAIL |

Frozen benefit thresholds were:

- TTFT ≤ 0.90× baseline;
- decode throughput ≥ 1.10× baseline;
- E2E ≤ 0.90× baseline;
- working set ≤ 0.85× baseline.

Frozen blocking-harm limits required all primary metrics to remain within 10% of baseline in the non-benefit directions.

No primary benefit dimension passed in any one of the four session/workload comparisons. Therefore there is no same-workload/same-dimension benefit that could reproduce across A and B. The blocking-harm guard also fails in all four comparisons.

Thus `H_NPA` is **not falsified**.

## Supporting-only observations

ArcLLM private bytes remain approximately 3% below baseline and recorded process CPU mean remains much lower. These observations reproduce, but the Q3 contract explicitly prohibited private bytes, CPU utilization and GPU counters from establishing a regime advantage by themselves.

They therefore cannot change the Q3 verdict.

## Governance decision

The current ArcLLM architecture line is **closed**.

Per the preregistered stop rule:

- no Q3-A/Q3-B extension;
- no iterative tuning of the current architecture;
- no workload search;
- no threshold revision;
- no retroactive use of NEXUS evidence to rescue Q3.

The final validation verdict for the current architecture is:

> **FEASIBLE_NO_DEMONSTRATED_ADVANTAGE**

ArcLLM successfully demonstrated that this custom Vulkan architecture can execute the real 7B model end-to-end on the target hardware. It did not demonstrate a practically meaningful matched regime advantage against the pinned llama.cpp baseline under W-S or W-C.

## Next scientific boundary

Further work on the **current** ArcLLM architecture is not scientifically justified by this validation line.

A future study may be considered only through a separate **Architecture Intervention Review** after this verdict. That review may inspect independently established NEXUS findings solely as hypothesis sources and ask whether any finding has a mechanistic causal mapping to the Q2/Q3 bottleneck.

A NEXUS result cannot be transferred as ArcLLM evidence. Any successor architecture must begin as a new bounded study with a frozen mechanism hypothesis, falsification condition, resource budget and stop condition before implementation.
