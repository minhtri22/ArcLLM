# P8-G4 Contract — fresh-cohort compositional error decomposition

## Parent evidence

P8-G3 is COMPLETE and diagnostic-valid on the exact frozen 7B target.

Authoritative P8-G3 SHA256:
- shader provenance: C10D0DB9444E584CDC76D939F5D134EC1229BD600CF3EED181C9D08505E955E8
- arithmetic-precision result: B0ADAAF9790018467C721599C2147AF7A6C0F69F0967B5C25F77631095571835
- summary: EF494284E7BFBED541380267EDB197CF53F9EBB7E86040A83546735F84209C4D

P8-G3 frozen observations:
- R0 reproduced P8-G2.
- R0 closure FAIL and alpha-linearity FAIL.
- R1 closure PASS: max_abs=1.32623827084899e-05, RMSE=2.76852946947452e-07.
- R1 alpha-linearity PASS.
- R2 and R3 also PASS, with closure near floating-point noise.
- closure earliest pass = R1.
- alpha earliest pass = R1.
- classification = H-FP32-ACCUMULATION.

Therefore sequential FP32 accumulation is sufficient to explain the P8-G2 A2/A4 misses. This does not revise P8-G's historical FAIL.

## Scientific question

Does the causal separation observed in P8-G1 replicate prospectively on fresh inputs when the FFN-down CPU diagnostic oracle uses the P8-G3-qualified R1 arithmetic?

Specifically: across a fresh input cohort, is downstream total error still dominated by propagated upstream state drift while same-input local GPU FFN-down error remains small?

P8-G4 is a replication/decomposition study. It does not define a replacement correctness gate.

## Frozen target and scope

Target SHA256:
60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463

Model scope:
- layers exactly [0,1,2,3];
- seq=4;
- pos=0;
- same frozen kernels and arena/binding plan;
- no layer4+;
- no embedding, output norm, LM-head, decode, sampling, generation or performance study.

Focus:
- L3 SwiGLU -> blk.3.ffn_down.weight;
- Q4_K;
- n=18944;
- rows=3584;
- batch=4;
- exact original GGUF byte span;
- one physical binding slice.

## Fresh cohort

Exactly four fresh deterministic input IDs:
S={17,29,43,61}

For each s in S and flattened input index i:

x_s[i] =
0.13*sin((i + 11 + 37*s)*0.009)
+ 0.04*cos((i + 5 + 19*s)*0.017)
+ 0.02*sin((i + 3 + 23*s)*0.0043)

These inputs are frozen before any P8-G4 target result is observed.

No P8-G/P8-G1/P8-G2/P8-G3 original input is part of this cohort.

## Per-input execution

For each fresh input independently:

1. Independent CPU reference executes L0->L1->L2->L3 through SwiGLU.
2. GPU executes the exact direct prefix through L3 SwiGLU:
   - 58 dispatches;
   - one submit;
   - direct GPU handoff;
   - no CPU teacher forcing.
3. Retain:
   - X_CPU = CPU L3 SwiGLU;
   - X_GPU = GPU L3 SwiGLU.
4. Use the exact P8-G3 R1 diagnostic oracle for L3 FFN-down:
   - exact Q4_K decoded weight remains float;
   - product remains float;
   - accumulator is double;
   - final output is cast to float.
5. Compute:
   - Y_CC = R1_down(X_CPU);
   - Y_CG = R1_down(X_GPU).
6. Execute the production P7-G Q4_K tiled16 down kernel on X_GPU:
   - Y_GG = GPU_down(X_GPU);
   - exactly one diagnostic down dispatch;
   - one separate submit.

Teacher forcing is used only for the R1 CPU diagnostic oracle. No CPU state is injected into the direct GPU prefix.

## Decomposition

For every fresh input define:

E_state = Y_CG - Y_CC

E_local = Y_GG - Y_CG

E_total = Y_GG - Y_CC

By construction, total error should decompose into propagated state drift plus local operator error.

Record max_abs and RMS for all three.

Record:
- local_to_state_max = max_abs(E_local)/max_abs(E_state)
- local_to_state_rms = RMS(E_local)/RMS(E_state)
- state_to_signal_max = max_abs(E_state)/max_abs(Y_CC)
- state_to_signal_rms = RMS(E_state)/RMS(Y_CC)
- local_to_signal_max = max_abs(E_local)/max_abs(Y_CG)
- local_to_signal_rms = RMS(E_local)/RMS(Y_CG)

Signal-normalized values are descriptive and do not define a production gate.

## D0 — decomposition closure

Compute:

E_reconstructed = E_state + E_local

Compare E_reconstructed with E_total.

Frozen diagnostic closure criterion:
- finite values;
- max_abs <= 1e-5;
- RMSE <= 1e-7.

A seed with closure failure is invalid for causal dominance interpretation.

## D1 — same-input local operator control

For every fresh seed, E_local must satisfy the historical P8-G numerical thresholds:
- max_abs <= 0.02;
- RMSE <= 0.005;
- finite values.

This does not make the historical P8-G PASS. It only checks that the local same-input operator is not itself showing a large correctness failure.

## D2 — prospective state-drift dominance

For every fresh seed with valid D0:

- local_to_state_max <= 0.05
- local_to_state_rms <= 0.05

This preregisters the replication claim that local operator error contributes at most 5% of propagated state-drift error on both norms.

The 5% criterion is a mechanistic replication threshold, not a production correctness gate.

## D3 — historical total-error observation

For every seed record whether E_total would pass the historical P8-G 0.02 / 0.005 gate.

This is descriptive only and is excluded from the P8-G4 PASS/FAIL decision. P8-G4 must not be tuned according to how many seeds cross the historical total-error threshold.

## Cohort adjudication

A seed is COMPOSITIONAL_SEPARATION_PASS when D0, D1 and D2 all pass.

Overall classification:

H-COMPOSITIONAL-SEPARATION-REPLICATED:
- all 4/4 fresh seeds are COMPOSITIONAL_SEPARATION_PASS.

H-HETEROGENEOUS-SEPARATION:
- D0 is valid for all seeds;
- at least 2/4 but fewer than 4/4 seeds pass D1+D2.

H-LOCAL-ERROR-NONNEGLIGIBLE:
- D0 is valid for all seeds;
- fewer than 2/4 seeds pass D1+D2.

DECOMPOSITION-INVALID:
- D0 fails for any seed, or exact execution/binding invariants fail.

No other classification is permitted.

## Governance

P8-G4 must not:
- change any shader or model weight;
- change historical P8-G 0.02 / 0.005 thresholds;
- change P8-G2/P8-G3 diagnostic thresholds;
- define a replacement correctness gate;
- revise P8-G FAIL;
- revise P8-G1 H-AMPLIFICATION;
- revise P8-G2 H-NONLINEAR/UNEXPLAINED;
- revise P8-G3 H-FP32-ACCUMULATION;
- execute layer4+;
- run decode/generation/performance;
- permit P8-H or full inference.

## Next

If H-COMPOSITIONAL-SEPARATION-REPLICATED is supported on 4/4 fresh inputs, the next scientific step may design a separate correctness-metric qualification study that explicitly separates local operator correctness from composed-state drift.

If separation is heterogeneous or local error is non-negligible, investigate the failing fresh seeds before any metric/gate design.

P8-H remains blocked.
