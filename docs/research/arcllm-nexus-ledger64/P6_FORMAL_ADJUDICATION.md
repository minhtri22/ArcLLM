# ANL64 P6 — Formal Matched E2E Adjudication

**Date:** 2026-09-22  
**Verdict:** `P6_NO_MATERIAL_E2E_BENEFIT`  
**Validity:** valid fresh confirmatory negative.

The recovered P6 collection is complete and independently reproducible from the returned bundle.

## F0 — provenance and freshness: PASS

Returned bundle:

```text
SHA256
4C91C295F98E9A5267135F1D4693E147AB7FBDDD54564A8EAC9CFB85F67F4BE1

bytes
24,699

members
19
```

Execution head:

```text
a0d7cc2cc0bc060114274034823cf846418c6525
```

The canonical P6 authorization, lock, specification, specification QA and P5 adjudication embedded in the bundle reproduce their exact Git blobs.

The interrupted first attempt is not used. It was previously classified `SPENT_NON_ADMISSIBLE`. The returned bundle comes from the single authorized full A+B replay after that infrastructure interruption.

Environment:
- exact 4,683,074,048-byte model;
- model SHA256 `60E05F21...BFC2463`;
- Intel Core Ultra 7 258V;
- Intel Arc 140V;
- driver 32.0.101.8860;
- Windows build 26200;
- Balanced power scheme;
- AC online.

Session A PID 51656 and Session B PID 41552 are distinct.

Exactly 40/40 fresh measured attempts are present.

## F1 — semantic and structural guard: PASS

All 40 measured attempts:
- succeeded;
- produced finite logits;
- passed dispatch census.

Every matched candidate/reference attempt produced the exact same 32-token greedy sequence.

Candidate plan identity remained exactly:

```text
nodes                    469
quant-linear nodes       215
Q4_FAST nodes            140
regions                24,104
fixed-Q4 regions      19,936
metadata bytes        593,504
plan hash     04f3f884c0fc4fcc
```

## F2 — session validity: PASS

The two sessions use independent child processes and the exact frozen counterbalanced orders.

No selective cell/session salvage was used in the valid replay.

## F3 — decode material benefit: PASS

Frozen gate:

```text
candidate/reference >= 1.10
```

Observed:

```text
A / W-S   2.5318×
A / W-C   1.2228×
B / W-S   2.1635×
B / W-C   3.2002×
```

All four pass.

## F4 — E2E material benefit: PASS

Frozen gate:

```text
candidate/reference <= 0.90
```

Observed E2E latency ratios:

```text
A / W-S   0.4044
A / W-C   0.8765
B / W-S   0.4709
B / W-C   0.3984
```

All four pass.

This is strong evidence that the admitted ANL64 Q4 decode substitutions propagate to large user-visible E2E reductions against the exact safe ArcLLM reference on these frozen workloads.

## F5 — TTFT blocking harm: FAIL

Frozen blocking gate:

```text
candidate/reference <= 1.10
```

Observed:

```text
A / W-S   1.5738   FAIL
A / W-C   1.3365   FAIL
B / W-S   1.3085   FAIL
B / W-C   1.0677   PASS
```

The guard fails in three of four required comparisons.

The canonical P6 stop rule states that **any** TTFT blocking-guard failure yields `P6_NO_MATERIAL_E2E_BENEFIT`, even if decode and E2E primary gates pass.

## Final classification

```text
F0 provenance/freshness       PASS
F1 semantic/structural        PASS
F2 session validity           PASS
F3 decode material benefit    PASS
F4 E2E material benefit       PASS
F5 TTFT blocking harm         FAIL (3/4)
-----------------------------------------------
P6_NO_MATERIAL_E2E_BENEFIT
```

This is a valid negative for the preregistered overall P6 claim.

It does **not** mean ANL64 failed to accelerate inference: the fresh matched evidence supports large decode and E2E gains relative to the safe ArcLLM reference. The blocked claim is specifically the combined practical-materiality criterion because TTFT regressed beyond the frozen 10% harm ceiling.

No threshold revision, workload search, executor swap, prefill tuning or rescue is admissible inside this program.

Next: P7 final program adjudication.
