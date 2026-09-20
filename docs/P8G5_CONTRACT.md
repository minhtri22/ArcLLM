# P8-G5 Contract — production-semantic local-error attribution

## Parent evidence

P8-G4 is COMPLETE and diagnostic-valid on the exact frozen 7B target.

Authoritative P8-G4 SHA256:
- shader provenance: 1C146FCDD60D14A782512687865AC403D6DEE5C72FC125F8A74C5D4A920D5C7C
- fresh compositional result: 9255B70BA50DF316B7D8BA04CB6696DA9FB2CB97D82F7A559DCF166F7E76E757
- summary: 6347545710333A3CF9856399DF040E57A6C140E3463C1E50E5A8A432AF82FFD0

Frozen P8-G4 observations:
- all 4/4 D0 decomposition closures PASS;
- all 4/4 D1 same-input local historical gates PASS;
- all 4/4 D2 5% state-dominance tests FAIL;
- seed pass count = 0/4;
- classification = H-LOCAL-ERROR-NONNEGLIGIBLE.

Frozen D2 ratios:
- seed 17: local/state max=0.0467289719626168, RMS=0.0539959165053624;
- seed 29: max=0.377049180327869, RMS=0.175281903700474;
- seed 43: max=0.0329218106995885, RMS=0.0640244987353183;
- seed 61: max=0.0617977528089888, RMS=0.0523069533336445.

P8-G4 remains frozen under its preregistered protocol.

## Source-level motivation

The production P7-G Q4_K tiled16 shader uses:
- float dequantized weights;
- float input values;
- float products;
- float accumulators `acc_a` / `acc_b`;
- sequential k traversal: k0 advances by 32 and kk advances 0..31.

Therefore production arithmetic semantics match the R0 FP32 oracle much more closely than the P8-G3 R1 oracle.

P8-G4 defined local error against R1:

E_local_R1 = GPU_down(X_GPU) - R1_down(X_GPU)

R1 intentionally changes accumulation from float to double. Therefore E_local_R1 can contain both:
1. actual production-kernel residual;
2. the R0->R1 arithmetic-oracle shift.

P8-G5 separates these terms explicitly.

## Scientific question

Are the P8-G4 D2 failures primarily an artifact of comparing the production FP32 kernel with the higher-precision R1 oracle, or do they persist when local error is measured against a production-matched R0 oracle?

P8-G5 is retrospective causal attribution on the already frozen P8-G4 cohort. It is not a fresh confirmation and cannot establish generalization.

## Frozen target and cohort

Target SHA256:
60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463

Reuse exactly P8-G4 cohort:
S={17,29,43,61}

Reuse exactly the P8-G4 input formula and GPU prefix:
- layers [0,1,2,3];
- L3 GPU execution through SwiGLU;
- 58 prefix dispatches / one submit per seed;
- direct GPU handoff;
- no CPU teacher forcing in prefix;
- no layer4+.

## Three local outputs on X_GPU

For every seed retain X_CPU and X_GPU at L3 SwiGLU.

Compute:

Y_R0_G = R0_down(X_GPU)

where R0 is production-matched CPU arithmetic:
- exact Q4_K weight decode to float;
- float product;
- sequential float accumulator in k=0..18943 order;
- float output.

Compute:

Y_R1_G = R1_down(X_GPU)

using the exact P8-G3/P8-G4 R1 oracle:
- exact Q4_K weight decode to float;
- float product;
- double sequential accumulator;
- float output.

Compute:

Y_GPU = production P7-G Q4_K tiled16 down(X_GPU)

with exactly one local GPU dispatch / one separate submit.

Also compute R0_down(X_CPU) and R1_down(X_CPU) for state terms.

## P0 — exact P8-G4 reproduction

Reproduce the P8-G4 R1 decomposition for all four seeds.

For E_state_R1 and E_local_R1:
- max_abs metric absolute tolerance <=1e-6;
- RMSE metric absolute tolerance <=1e-7;
- local/state ratio relative tolerance <=0.1%.

The four D2_R1 outcomes must reproduce as FAIL.

If P0 fails, P8-G5 is ATTRIBUTION-INVALID.

## P1 — oracle-shift decomposition

Define:

E_prod = Y_GPU - Y_R0_G

E_oracle = Y_R0_G - Y_R1_G

E_local_R1 = Y_GPU - Y_R1_G

Then:

E_reconstructed_local = E_prod + E_oracle

Compare E_reconstructed_local with E_local_R1.

Frozen closure:
- finite values;
- max_abs <=1e-5;
- RMSE <=1e-7.

Record max_abs/RMS of E_prod, E_oracle and E_local_R1.

Record descriptively:
- prod_to_r1local_max/RMS;
- oracle_to_r1local_max/RMS.

These contribution ratios are descriptive because max coordinates and vector directions can differ.

## P2 — production-matched local equivalence

For every seed compare Y_GPU with Y_R0_G.

Prospective diagnostic gate:
- max_abs <=1e-4;
- RMSE <=1e-6;
- finite values.

This gate is chosen before P8-G5 target data and is anchored to the pre-existing P8-G1 same-input production control, where the GPU-vs-R0 residual was on the order of 3.05e-5 max_abs and 6.08e-7 RMSE.

P2 is a diagnostic equivalence gate, not a production correctness gate.

## P3 — production-semantic compositional decomposition

Define:

E_state_R0 = R0_down(X_GPU) - R0_down(X_CPU)

E_local_R0 = Y_GPU - R0_down(X_GPU)

E_total_R0 = Y_GPU - R0_down(X_CPU)

Require decomposition closure:
E_state_R0 + E_local_R0 == E_total_R0

under:
- max_abs <=1e-5;
- RMSE <=1e-7.

Apply the unchanged P8-G4 mechanistic dominance threshold:
- max_abs(E_local_R0)/max_abs(E_state_R0) <=0.05
- RMS(E_local_R0)/RMS(E_state_R0) <=0.05

No relaxation of the 5% threshold is permitted.

## Attribution

H-R1-ORACLE-SEMANTIC-MISMATCH:
- P0 reproduces;
- P1 closure passes all 4 seeds;
- P2 passes all 4 seeds;
- P3 decomposition closure passes all 4 seeds;
- P3 state-dominance passes all 4 seeds.

Interpretation: P8-G4 D2 failures were caused by using R1 as the local production comparator, while the production-matched R0 residual is sufficiently small to restore the frozen 5% dominance criterion on this cohort.

H-RATIO-SENSITIVITY:
- P0/P1 valid;
- P2 passes all 4 seeds;
- P3 decomposition closures all pass;
- P3 state-dominance fails at least one seed.

Interpretation: production kernel residual remains absolutely tiny, but the 5% local/state ratio is unstable when E_state is small. Do not relax the threshold retrospectively.

H-PRODUCTION-LOCAL-RESIDUAL:
- P0/P1 valid;
- P2 fails any seed.

Interpretation: the production-matched local residual itself exceeds the preregistered diagnostic envelope and requires kernel-level investigation.

ATTRIBUTION-INVALID:
- P0 fails;
- any P1/P3 decomposition closure fails;
- structural or exact execution invariants fail.

No other classification is permitted.

## Governance

P8-G5 must not:
- alter shaders or model weights;
- alter P8-G4 cohort IDs or formula;
- change P8-G4 5% D2 threshold;
- change historical 0.02/0.005 thresholds;
- define a replacement correctness gate;
- revise P8-G FAIL;
- revise P8-G1 H-AMPLIFICATION;
- revise P8-G2 H-NONLINEAR/UNEXPLAINED;
- revise P8-G3 H-FP32-ACCUMULATION;
- revise P8-G4 H-LOCAL-ERROR-NONNEGLIGIBLE;
- execute layer4+;
- run decode/generation/performance;
- permit P8-H or full inference.

## Next

If H-R1-ORACLE-SEMANTIC-MISMATCH is supported, the next step is a new fresh confirmatory cohort using the production-matched R0 decomposition while retaining R1 only as a side diagnostic.

If H-RATIO-SENSITIVITY is supported, do not alter 5% post hoc; design a separate prospective qualification of absolute/signal-normalized causal metrics.

If H-PRODUCTION-LOCAL-RESIDUAL is supported, investigate the production kernel residual before any metric study.

P8-H remains blocked.
