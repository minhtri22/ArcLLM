# ANL64 P5 — Formal Integration Adjudication

**Date:** 2026-09-21  
**Verdict:** `P5_INTEGRATION_PASS`

P5 tested the locked ANL64 planner + 140 Q4_FAST decode substitutions against the exact frozen safe ArcLLM reference. It did not adjudicate performance.

## F0 — provenance: PASS

Returned bundle:

```text
SHA256
44A3DE02CBFC25F9548E56D3C7A9F85E1247AD948C77029185E06C9EA9E0B73D

bytes
17,516

members
11
```

Exact execution head:

```text
d0e2e562f7c56d2d7e7af49358fc58d3c751e1da
```

The bundled authorization, lock, P5 specification, P5 QA and P4 adjudication independently reproduce their committed Git blobs.

Exact model and target:
- model SHA256 `60E05F21...BFC2463`;
- model bytes 4,683,074,048;
- Intel Core Ultra 7 258V;
- Intel Arc 140V;
- driver 32.0.101.8860.

The candidate executable SHA256 is exactly the P4-locked value:

```text
1F45DA9D8CACE3CF78FE31B7B7041B6027CB41E180F7FC99E50E1127EA4F5451
```

All four raw cell files are present and their SHA256 values reproduce the semantic evidence manifest.

A minor runner telemetry defect is recorded: the environment JSON's `cell_exit_codes` fields contain child stdout plus terminal exit code `0`, rather than scalar integers. This does not invalidate the collection because each raw result file independently exists, contains exactly five measured attempts, reports success/finite/dispatch PASS, and is SHA-bound. No selective rerun or substitution occurred.

## F1 — structure: PASS

All 20/20 measured attempts:
- succeeded;
- produced finite final logits;
- passed dispatch census.

ANL64 reports the exact frozen plan:

```text
PlanNodes                 469
quant-linear nodes        215
Q4_FAST nodes             140
Region64                24,104
fixed-Q4 Region64       19,936
metadata bytes           593,504
metadata hard budget   2,097,152
plan hash        04f3f884c0fc4fcc
```

The plan hash is identical between W-S and W-C.

## F2 — unchanged-prefill control: PASS

There are ten measured candidate/reference attempt pairs.

```text
first generated token equal
10 / 10

mismatches
0
```

The unchanged prefill control therefore passes.

## F3 — integrated decode semantics: PASS

The preregistered primary rule was exact equality of the complete 32-token greedy sequence.

```text
W-S pairs exact
5 / 5

W-C pairs exact
5 / 5

total exact pairs
10 / 10

token-mismatch pairs
0
```

Generated sequence hashes:

```text
W-S  f31d4bb9fe5eb9c3
W-C  471519ddc45b232e
```

Final-logit and final-hidden hashes differ between the safe reference and ANL64. This is not a failure under the frozen P5 contract: the inherited Q4_FAST mechanism changes FP32 reduction order, and cross-system bitwise numeric hash equality was explicitly not required. The observable greedy sequences are exactly identical.

## F4 — repeatability: PASS

For all four system/workload cells, all five measured attempts have:
- identical 32-token sequence;
- stable final-logits hash;
- stable final-hidden hash.

No within-cell nondeterminism was observed.

## Performance quarantine

P5 timing values are not used.

The semantic extractor reports:

```text
performance_fields_extracted = false
raw_timing_decision_role = NONE_SPENT_FOR_P6
```

Therefore P5 makes no performance claim. Any timing contained in raw runtime JSON is permanently non-admissible for P6.

## Final decision

```text
F0 provenance          PASS
F1 structure           PASS
F2 prefill control     PASS
F3 decode semantics    PASS
F4 repeatability       PASS
--------------------------------
P5_INTEGRATION_PASS
```

The current ANL64 architecture has now demonstrated semantic integration of the frozen 140-node Q4_FAST substitution on both frozen workloads.

P6 is eligible for **specification only**. P6 performance execution is not authorized by this result and must use a fresh separately frozen collection.
