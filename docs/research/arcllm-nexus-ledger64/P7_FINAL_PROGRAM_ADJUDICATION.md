# ANL64 P7 — Final Program Adjudication

**Date:** 2026-09-22  
**Final status:** `ANL64_PROGRAM_CLOSED_VALID_NEGATIVE_TTFT_BLOCKED`

## Research question

The ANL64 program asked whether an exact-model runtime combining ArcLLM quant-specific execution with transfer-admissible NEXUS / Event-Ledger / Ledger64 planning could produce a material end-to-end improvement without changing model semantics.

Under the frozen preregistered contract, the answer is:

```text
NOT ESTABLISHED
```

The reason is narrow and explicit: semantic preservation and large decode/E2E gains were demonstrated, but the TTFT blocking guard failed in three of four fresh matched comparisons.

## Stage synthesis

```text
P0  origin/governance                       COMPLETE
P1  prior-art + transfer review             PASS
P2  architecture + E2E upper bound          PASS
P3  zero-science local compatibility        PASS
P4  bounded implementation/build            PASS
P5  semantic integration                    PASS
P6  fresh matched E2E confirmation          VALID NEGATIVE
P7  final adjudication                      TERMINAL
```

## What was demonstrated

### Exact-model semantics

P5 and P6 support semantic preservation on the frozen W-S and W-C workloads:
- exact 32-token greedy sequence equality;
- all attempts successful;
- finite logits;
- dispatch census PASS.

### Ledger64-style planning integration

The frozen successor maintained:

```text
469 PlanNodes
215 quant-linear nodes
140 fixed Q4_FAST nodes
24,104 Region64 descriptors
19,936 fixed-Q4 Region64 descriptors
593,504 metadata bytes
plan hash 04f3f884c0fc4fcc
```

This supports local compatibility and stable plan authority.

### Decode effect

Fresh P6 candidate/reference decode ratios:

```text
A / W-S   2.5318×
A / W-C   1.2228×
B / W-S   2.1635×
B / W-C   3.2002×
```

All four clear the frozen 1.10 materiality gate.

### End-to-end effect

Fresh P6 E2E latency ratios:

```text
A / W-S   0.4044
A / W-C   0.8765
B / W-S   0.4709
B / W-C   0.3984
```

All four clear the frozen 0.90 gate.

The successor therefore demonstrated a real decode effect that propagated to large E2E latency reductions against the exact safe ArcLLM reference.

## Why the overall program still closes as a valid negative

The preregistered TTFT blocking guard was:

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

Three of four required comparisons fail.

The P6 contract explicitly states that any TTFT blocking-guard failure yields:

```text
P6_NO_MATERIAL_E2E_BENEFIT
```

even when decode and E2E primary gates pass.

That rule was frozen before the outcome and cannot be weakened after seeing the result.

## Claim boundary

Supported:
- semantic preservation on the frozen tested workloads;
- stable ANL64/Ledger64 plan integration;
- material decode gain against the exact safe ArcLLM reference;
- material E2E latency reduction against the exact safe ArcLLM reference.

Not supported:
- the preregistered overall practical-materiality claim;
- absence of TTFT harm;
- competitive advantage against llama.cpp or another external runtime;
- generalization beyond the exact model / Intel Arc 140V / frozen workloads;
- an independent causal performance benefit of NEXUS/Ledger64 control-plane logic itself.

The last point matters: P6 compares the complete frozen successor with the safe reference. It does not contain an ablation where executor substitutions are held fixed and Ledger64 planning is removed. Therefore the control plane's independent performance contribution is not identified.

## Program closure

No further rescue is admissible inside ANL64:
- no threshold relaxation;
- no workload search;
- no executor retuning to make P6 pass;
- no prefill/TTFT optimization under the same program;
- no P6 rerun;
- no renamed continuation.

The program is terminal.

```text
ANL64_PROGRAM_CLOSED_VALID_NEGATIVE_TTFT_BLOCKED
```

## Scientifically valid future direction

If TTFT is worth pursuing, it must be a **new independent research program**, not ANL64 rescue.

A valid new question would be:

> What mechanism causes the reproducible TTFT regression in the frozen ANL64 successor, and can a separately preregistered architecture remove it without sacrificing the already-demonstrated decode/E2E gains?

That new program would require:
- a fresh branch;
- a fresh `lineage.md`;
- ORIGIN binding this terminal P7 evidence;
- independent prior-art/source review;
- a new mechanism hypothesis and falsification gates;
- fresh evidence.

No such successor program is authorized by this P7 closure.

Current next experiment:

```text
NONE
```
