# P8-G6 Contract — fresh production-semantic separation confirmation

## Parent evidence

P8-G5 is COMPLETE and diagnostic-valid on the exact frozen 7B target.

Authoritative P8-G5 SHA256:
- shader provenance: 6483D82540EC31F3CE058B51FC48C9BABFED9983F7F59A090D9AE14917176F9A
- production-semantic attribution: 64565C94AD2D9EF85D1DFF8CE972FD263C2C1B4A28A28B5F6830F6EB9AD6E096
- summary: 1732663E0A9CF90848D54487E2C2D031EB1A5C2A90808A34ABB1C06D57995751

Frozen P8-G5 observations:
- P0 reproduces P8-G4 on all 4 retrospective seeds;
- P1 oracle-shift reconstruction passes all 4;
- P2 production GPU vs R0 passes all 4;
- P3 R0 decomposition closure passes all 4;
- P3 R0 state-dominance <=5% passes all 4;
- structural_valid=true;
- classification=H-R1-ORACLE-SEMANTIC-MISMATCH.

P8-G5 is retrospective attribution only and does not establish generalization.

## Scientific question

On a genuinely fresh input cohort, does the production-semantic R0 decomposition reproduce the same causal separation:
1. production GPU down remains locally equivalent to R0;
2. composed total error closes as upstream state drift + local production residual;
3. local production residual remains <=5% of propagated state drift on both max and RMS?

R1 is retained only as a side diagnostic and has no role in P8-G6 adjudication.

## Frozen target and scope

Target SHA256:
60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463

Model scope:
- layers exactly [0,1,2,3];
- seq=4;
- pos=0;
- same frozen shaders, resolver and arena plan;
- GPU executes L0-L2 fully and L3 through SwiGLU;
- 58 prefix dispatches / one submit per seed;
- one production P7-G Q4_K tiled16 down dispatch / one separate submit per seed;
- no CPU teacher forcing in GPU prefix;
- no layer4+, embedding, output norm, LM-head, decode, sampling, generation or performance study.

Focus:
- blk.3.ffn_down.weight;
- Q4_K;
- n=18944;
- rows=3584;
- batch=4;
- exact GGUF source span and single-piece binding.

## Fresh cohort

Exactly four new deterministic IDs:

S={73,89,107,131}

These IDs are disjoint from P8-G4/P8-G5 cohort {17,29,43,61}.

For flattened input index i and seed s:

x_s[i] =
0.13*sin((i + 11 + 37*s)*0.009)
+ 0.04*cos((i + 5 + 19*s)*0.017)
+ 0.02*sin((i + 3 + 23*s)*0.0043)

The cohort is frozen before any P8-G6 target evidence is observed.

## Primary R0 oracle

For X_CPU and X_GPU at L3 SwiGLU compute R0_down:

- exact Q4_K weight decode to float;
- float input;
- float product;
- sequential float accumulator in k=0..18943 order;
- float output.

R0 is the production-semantic comparator.

## Side R1 diagnostic

Also compute R1_down for X_CPU and X_GPU:
- same decoded float weights;
- same float products;
- double sequential accumulator;
- final output cast to float.

Record R0->R1 shifts, but R1 metrics MUST NOT participate in P8-G6 PASS/FAIL or classification.

## C0 — structural/execution validity

For every seed require:
- exact target / binding invariants;
- direct GPU prefix handoff;
- prefix dispatches=58;
- prefix submits=1;
- local-down dispatches=1;
- local-down submits=1;
- finite R0, R1 and GPU outputs;
- no layer4+.

Any C0 failure => CONFIRMATION-INVALID.

## C1 — production-matched local equivalence

For every seed:

E_local_R0 = Y_GPU - R0_down(X_GPU)

Require:
- max_abs <=1e-4;
- RMSE <=1e-6;
- finite values.

This is the same preregistered P8-G5 P2 diagnostic envelope.
It is not a replacement production correctness gate.

## C2 — production-semantic decomposition closure

Define:

E_state_R0 = R0_down(X_GPU) - R0_down(X_CPU)
E_total_R0 = Y_GPU - R0_down(X_CPU)
E_reconstructed_R0 = E_state_R0 + E_local_R0

Compare E_reconstructed_R0 with E_total_R0.

Require:
- max_abs <=1e-5;
- RMSE <=1e-7;
- finite values.

Any C2 failure => CONFIRMATION-INVALID.

## C3 — fresh state-dominance confirmation

For every seed with valid C0/C2:

local_to_state_max =
max_abs(E_local_R0) / max_abs(E_state_R0)

local_to_state_rms =
RMS(E_local_R0) / RMS(E_state_R0)

Require unchanged:
- local_to_state_max <=0.05
- local_to_state_rms <=0.05

No relaxation, denominator flooring or post-hoc normalization is permitted.

## Descriptive-only diagnostics

Record but exclude from adjudication:
- R1 state/local terms;
- R0->R1 oracle shift max/RMS;
- historical total-error pass/fail under 0.02 / 0.005;
- signal-normalized state/local errors;
- coordinates of max local and max state errors.

## Cohort adjudication

H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED:
- C0 valid all 4;
- C1 PASS all 4;
- C2 PASS all 4;
- C3 PASS all 4.

H-FRESH-RATIO-SENSITIVITY:
- C0 valid all 4;
- C1 PASS all 4;
- C2 PASS all 4;
- C3 FAIL at least one seed.

H-FRESH-PRODUCTION-LOCAL-RESIDUAL:
- C0 valid all 4;
- C2 PASS all 4;
- C1 FAIL at least one seed.

CONFIRMATION-INVALID:
- any C0 failure;
- any C2 failure;
- exact target/binding/execution invariant failure.

No other classification is permitted.

## Governance

P8-G6 must not:
- reuse IDs 17,29,43,61 as confirmatory observations;
- change shaders, model weights or target;
- change 1e-4 / 1e-6 local-equivalence gate;
- change 1e-5 / 1e-7 closure gate;
- change 5% dominance threshold;
- let R1 diagnostics influence adjudication;
- define a replacement correctness gate;
- revise P8-G FAIL;
- revise P8-G1 H-AMPLIFICATION;
- revise P8-G2 H-NONLINEAR/UNEXPLAINED;
- revise P8-G3 H-FP32-ACCUMULATION;
- revise P8-G4 H-LOCAL-ERROR-NONNEGLIGIBLE;
- revise P8-G5 H-R1-ORACLE-SEMANTIC-MISMATCH;
- execute layer4+;
- permit P8-H or full inference.

## Next

If H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED is supported, design a separate prospective correctness-metric qualification that explicitly distinguishes local operator correctness from composed-state drift. Do not rewrite historical P8-G and do not open P8-H in the same step.

If H-FRESH-RATIO-SENSITIVITY is supported, investigate prospective absolute/signal-normalized causal metrics without relaxing 5% retrospectively.

If H-FRESH-PRODUCTION-LOCAL-RESIDUAL is supported, investigate the local production kernel residual before any metric qualification.

P8-H remains blocked.
