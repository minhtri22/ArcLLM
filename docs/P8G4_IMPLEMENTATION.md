# P8-G4 Implementation — fresh-cohort compositional error decomposition

Date: 2026-09-20

## Purpose

P8-G4 prospectively replicates the P8-G1 causal separation on four fresh deterministic inputs after P8-G3 qualified the R1 diagnostic oracle.

It does not redefine correctness and does not revise any historical P8 verdict.

## Fresh cohort

Exactly:
- 17
- 29
- 43
- 61

For flattened index i and seed s:

x_s[i] =
0.13*sin((i+11+37*s)*0.009)
+ 0.04*cos((i+5+19*s)*0.017)
+ 0.02*sin((i+3+23*s)*0.0043)

The original P8-G input is not generated or executed.

## Frozen GPU prefix

One prefix command chain is prepared once and reused across seeds.

For each seed:
- copy x_s into the same host-visible coherent input buffer;
- execute L0-L2 fully and L3 through SwiGLU;
- exact 58 dispatches;
- exact one submit;
- direct GPU handoff;
- no CPU state injection;
- no layer4+.

The CPU reference executes L0-L2 fully and L3 only through SwiGLU. L3 CPU FFN-down is explicitly disabled.

## R1 diagnostic oracle

The exact P8-G3 R1 arithmetic is implemented independently for the focus Q4_K down projection:

- Q4_K decoded weight: float;
- product: float;
- accumulator: double;
- final output: float.

The oracle evaluates X_CPU and X_GPU together while decoding each Q4_K weight once. Independent output coordinates may be distributed over up to eight CPU worker threads. Within each output row, term order remains k=0..18943.

Outputs:
- Y_CC = R1_down(X_CPU)
- Y_CG = R1_down(X_GPU)

The R1 oracle is CPU-only.

## Production local operator

A second command chain contains exactly one P7-G Q4_K tiled16 FFN-down dispatch on the actual X_GPU buffer.

For each seed:
- one dispatch;
- one submit;
- Y_GG = GPU_down(X_GPU).

No CPU teacher-forced vector is uploaded to this production-kernel diagnostic.

## Error decomposition

Per seed:
- E_state = Y_CG - Y_CC
- E_local = Y_GG - Y_CG
- E_total = Y_GG - Y_CC
- E_reconstructed = E_state + E_local

D0 compares E_total with E_reconstructed:
- max_abs <= 1e-5
- RMSE <= 1e-7

D1 compares Y_GG with Y_CG using the historical local thresholds:
- max_abs <= 0.02
- RMSE <= 0.005

D2 requires:
- local_to_state_max <= 0.05
- local_to_state_rms <= 0.05

D3 records whether Y_GG vs Y_CC passes historical 0.02 / 0.005. It is descriptive only and is never used in seed_pass or cohort classification.

A seed passes only D0 && D1 && D2.

## Cohort classification

- 4/4 pass: H-COMPOSITIONAL-SEPARATION-REPLICATED
- 2-3/4 pass with all D0 valid: H-HETEROGENEOUS-SEPARATION
- 0-1/4 pass with all D0 valid: H-LOCAL-ERROR-NONNEGLIGIBLE
- any D0 or structural/execution failure: DECOMPOSITION-INVALID

## Execution lifecycle

The prefix and local-down chains are prepared once, reused across all four fresh seeds, and destroyed once.

Expected aggregate target execution:
- 4 * 58 = 232 prefix dispatches across four submits;
- 4 local-down dispatches across four submits.

The contract is enforced per seed, not via aggregate timing or performance measurement.

## Governance

Always frozen:
- P8-G = FAIL
- P8-G1 = H-AMPLIFICATION
- P8-G2 = H-NONLINEAR/UNEXPLAINED
- P8-G3 = H-FP32-ACCUMULATION
- replacement_gate_defined=false
- D3 decision role = descriptive_only
- P8-H forbidden
- full inference forbidden

No target fresh-cohort experiment is run in the implementation/static-QA lock commit.
