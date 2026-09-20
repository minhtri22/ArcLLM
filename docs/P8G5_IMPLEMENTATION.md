# P8-G5 Implementation — production-semantic local-error attribution

Date: 2026-09-20

## Purpose

P8-G5 replays the already frozen P8-G4 cohort only for retrospective causal attribution.

It asks whether P8-G4 D2 failures came from using the higher-precision R1 oracle as the production comparator, or whether the failures persist against a production-matched R0 oracle.

It is not a fresh confirmation study and cannot establish generalization.

## Cohort and GPU execution

Exactly the frozen P8-G4 cohort is reused:
- 17
- 29
- 43
- 61

The P8-G4 input formula is unchanged.

For every seed:
- CPU reference runs L0-L2 fully and L3 through SwiGLU only;
- GPU prefix runs the same prepared 58-dispatch chain / one submit;
- direct GPU handoff is preserved;
- no CPU teacher forcing;
- production local-down runs exactly one P7-G Q4_K tiled16 dispatch / one submit on X_GPU;
- no layer4+.

## Joint R0/R1 oracle

The CPU oracle decodes each Q4_K weight once per output coordinate and evaluates four outputs together:

- R0_down(X_CPU)
- R0_down(X_GPU)
- R1_down(X_CPU)
- R1_down(X_GPU)

Within each dot the exact term order is k=0..18943.

R0:
- decoded weight: float
- product: float
- accumulator: float
- output: float

R1:
- same decoded float weight
- same float product
- accumulator: double
- final output cast to float

Independent output coordinates may use up to eight CPU worker threads; there is no cross-output reduction.

## P0 — P8-G4 reproduction

For every seed P8-G5 reconstructs the P8-G4 R1 state/local terms and compares against frozen P8-G4 constants.

Required tolerances:
- max_abs metric difference <= 1e-6
- RMSE metric difference <= 1e-7
- local/state ratio relative difference <= 0.1%

The R1 5% D2 result must reproduce as FAIL for all four seeds.

Any P0 failure invalidates attribution.

## P1 — local-term split

Per seed:

E_prod = GPU - R0_G
E_oracle = R0_G - R1_G
E_local_R1 = GPU - R1_G

The implementation verifies:

E_prod + E_oracle = E_local_R1

with:
- max_abs <= 1e-5
- RMSE <= 1e-7
- finite values

It records absolute max/RMS magnitudes and descriptive contribution ratios.

## P2 — production-matched equivalence

Compare production GPU down(X_GPU) with R0_down(X_GPU).

Frozen diagnostic gate:
- max_abs <= 1e-4
- RMSE <= 1e-6
- finite values

This is not a replacement production correctness gate.

## P3 — production-semantic decomposition

Per seed:

E_state_R0 = R0_G - R0_C
E_local_R0 = GPU - R0_G
E_total_R0 = GPU - R0_C

Verify decomposition closure:
- max_abs <= 1e-5
- RMSE <= 1e-7

Then apply the unchanged P8-G4 mechanistic threshold:
- local/state max <= 0.05
- local/state RMS <= 0.05

No relaxation is permitted.

## Classification order

Classification is intentionally ordered:

1. ATTRIBUTION-INVALID if structural/execution invariants, P0, P1, or P3 closure fail.
2. H-PRODUCTION-LOCAL-RESIDUAL if any P2 seed fails.
3. H-R1-ORACLE-SEMANTIC-MISMATCH if all P3 dominance tests pass.
4. H-RATIO-SENSITIVITY otherwise.

This prevents an apparent R0 dominance result from hiding a production-matched local residual failure.

## Governance

Always frozen:
- P8-G = FAIL
- P8-G1 = H-AMPLIFICATION
- P8-G2 = H-NONLINEAR/UNEXPLAINED
- P8-G3 = H-FP32-ACCUMULATION
- P8-G4 = H-LOCAL-ERROR-NONNEGLIGIBLE
- replacement_gate_defined=false
- P8-H blocked
- full inference forbidden

No P8-G5 target attribution experiment is run in the implementation/static-QA lock commit.
