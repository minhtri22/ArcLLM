# ANL64 P5 — Component Integration Validation Specification

**Date:** 2026-09-21  
**State:** SPECIFICATION ONLY / EXECUTION BLOCKED

## Scientific question

P5 asks one bounded question:

> Does the locked ANL64 planner + 140-node Q4_FAST decode integration preserve the observable greedy execution semantics of the exact safe ArcLLM reference on the frozen target model, before any performance claim is considered?

P5 is not a benchmark.

## Reference and candidate

Reference:

```text
ARC_SAFE_REFERENCE
src/q2_benchmark.cpp
blob ea1e986e22f6921e7f6c52a4fa5935121cfec663
```

Candidate:

```text
ANL64_P4_LOCKED
src/anl64_runtime.cpp
blob dbcb7afed5a08e7aff3ca02a1bd95bd985076f70

src/anl64_plan.hpp
blob 157be15c63363ba2d55093af829ca68be9107e27
```

The candidate uses Q4_FAST only at the 140 frozen Q/K/O/gate/up decode nodes. Q6 and all other safe paths remain unchanged.

## Exact target

P5 is bound to the same 4,683,074,048-byte Qwen2 GGUF:

```text
SHA256
60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463
```

and the exact target:
- Intel Core Ultra 7 258V;
- Intel Arc 140V;
- driver 32.0.101.8860.

## Workloads

P5 uses exactly the already-frozen Q2 workload pair.

### W-S

```text
[1, 133151, 133152, 152062]
```

4 prompt tokens + 32 generated tokens.

### W-C

256 prompt tokens:

```text
first four:
[1, 133151, 133152, 152062]

for i >= 4:
1 + ((104729 + 7919*i) mod 152063)
```

followed by 32 generated tokens.

No workload search is permitted.

## Repetition contract

The locked production runtimes already enforce:

```text
1 warmup
5 measured attempts
```

per system/workload cell.

P5 reuses this exact repetition contract rather than changing locked production code solely to make the validation cheaper.

There are four cells:

```text
reference / W-S
candidate / W-S
reference / W-C
candidate / W-C
```

for 20 measured attempts total.

Warmups have no scientific decision role.

## Performance quarantine

The production runtime emits timing fields as part of its existing schema. P5 must not use them.

The P5 adjudicator is allowed to read only:
- success;
- final logits finite;
- dispatch census;
- generated token IDs;
- generated-token hash;
- final-logits hash;
- final-hidden hash;
- ANL64 plan metadata.

It must not adjudicate:
- TTFT;
- decode latency;
- decode tok/s;
- E2E latency;
- any performance/resource comparison.

Any P5 timing values are declared **spent/non-admissible for P6**. P6, if reached, requires a fresh separately authorized matched execution.

## F0 — provenance

P5 is invalid unless:
- exact model SHA/size matches;
- exact target GPU/driver matches;
- candidate is the P4-locked executable or an exact-source rebuild bound to the frozen blobs;
- safe reference is bound to the exact Q2 source/shaders;
- W-S/W-C and repetitions are exact;
- no runtime/shader mutation occurred;
- no attempt is selectively substituted.

## F1 — structure

Candidate must report:

```text
PlanNodes                 469
quant-linear PlanNodes    215
Q4_FAST nodes             140
Region64                24,104
fixed-Q4 Region64       19,936
metadata               <=2 MiB
```

The nonzero plan hash must be identical between W-S and W-C candidate processes.

Every measured attempt must:
- succeed;
- pass dispatch census;
- have finite logits.

## F2 — unchanged-prefill control

For every measured candidate/reference pair:

```text
generated_token_ids[0]
candidate == reference
```

This is a control because prefill is intentionally unchanged and uses the safe path.

A mismatch here is not attributed to Q4_FAST numerical tolerance; it indicates an integration/control problem.

## F3 — integrated decode semantics

Primary semantic gate:

> For each workload and measured attempt index, the complete 32-token greedy generated sequence must be exactly identical between ANL64 and the safe reference.

Tolerance:

```text
zero token mismatches
```

P5 does **not** require final-logit bit hashes to equal the safe reference because the admitted Q4_FAST executor changes FP32 reduction order. Closed SA1 evidence already established local numerical correctness for the exact Q4_FAST mechanism.

P5's new question is whether those locally correct substitutions preserve integrated greedy execution semantics.

## F4 — repeatability

Within each system/workload:
- all five measured generated sequences must be identical;
- final-logits hash must be stable across the five measured attempts for both reference and candidate;
- final-hidden hash must be stable across the five measured attempts for both reference and candidate.

Warmup is excluded from this gate.

## Outcomes

```text
P5_INTEGRATION_PASS
P5_SEMANTIC_INTEGRATION_FAIL
P5_INVALID_F0
P5_STOP_INFRASTRUCTURE_UNSTABLE
```

A valid semantic mismatch is terminal for the current ANL64 architecture path before P6. No threshold tuning, Q6 optimization, fusion, repack, prefetch or alternate Q4_FAST variant may be introduced as a P5 rescue.

A P5 PASS does not authorize P6 automatically.

## Current authorization

```text
P5 specification          AUTHORIZED
P5 target execution       BLOCKED
model load                BLOCKED
GPU dispatch              BLOCKED
performance adjudication  BLOCKED
P6                        BLOCKED
```

Next: zero-science QA of this specification, then a separate explicit P5 execution-authorization gate.
