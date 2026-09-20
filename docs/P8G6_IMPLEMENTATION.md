# P8-G6 Implementation — fresh production-semantic separation confirmation

Date: 2026-09-20

## Purpose

P8-G6 is the fresh prospective confirmation required after P8-G5 retrospective attribution.

The only scientific variable changed from the production-semantic decomposition established in P8-G5 is the input cohort.

## Fresh cohort

Confirmatory IDs are exactly:

- 73
- 89
- 107
- 131

The previous P8-G4/P8-G5 IDs {17,29,43,61} are retained only in metadata and are never executed as confirmatory observations.

The deterministic input family is unchanged:

x_s[i] =
0.13*sin((i+11+37*s)*0.009)
+ 0.04*cos((i+5+19*s)*0.017)
+ 0.02*sin((i+3+23*s)*0.0043)

## GPU execution

The GPU graph is unchanged.

Per fresh seed:
- L0-L2 execute fully;
- L3 executes through SwiGLU;
- exact 58 prefix dispatches;
- exact one prefix submit;
- direct GPU handoff;
- one production P7-G Q4_K tiled16 down dispatch on X_GPU;
- exact one local-down submit;
- no CPU teacher forcing;
- no layer4+.

The prefix and local-down prepared chains are created once and reused across all four seeds.

## Joint R0/R1 oracle

The P8-G5 joint oracle implementation is reused.

For every output coordinate a Q4_K weight is decoded once and feeds both R0 and R1 accumulators in the exact k=0..18943 order.

R0:
- weight: float
- product: float
- accumulator: float
- output: float

R1:
- same weight and product
- accumulator: double
- output cast to float

R0 is primary.

R1 is descriptive-only. R1 values are recorded for audit and interpretation but are excluded from C1, C2, C3 and the classification branch. The only R1-related validity condition is finite output under C0, exactly as frozen in the P8-G6 contract.

## C0

Per seed:
- R0/R1/GPU outputs finite;
- prefix execution = 58/1;
- local-down execution = 1/1.

Global structural validity additionally requires the frozen binding/focus/direct-handoff invariants.

## C1

Production GPU local output is compared with R0(X_GPU):

- max_abs <= 1e-4
- RMSE <= 1e-6

## C2

R0 decomposition:

E_state_R0 = R0(X_GPU) - R0(X_CPU)
E_local_R0 = GPU(X_GPU) - R0(X_GPU)
E_total_R0 = GPU(X_GPU) - R0(X_CPU)

E_state_R0 + E_local_R0 must reconstruct E_total_R0 under:

- max_abs <= 1e-5
- RMSE <= 1e-7

## C3

Unchanged prospective dominance gate:

- max_abs(E_local_R0)/max_abs(E_state_R0) <= 0.05
- RMS(E_local_R0)/RMS(E_state_R0) <= 0.05

No denominator floor or post-hoc normalization exists.

## Descriptive-only outputs

The result also records:
- R1 state/local error;
- R0->R1 CPU/GPU shift;
- state/signal and local/signal ratios;
- max-error coordinates;
- historical total 0.02/0.005 pass/fail.

None appears in the classification branch.

## Classification priority

1. CONFIRMATION-INVALID if structural validity, C0 or C2 fails.
2. H-FRESH-PRODUCTION-LOCAL-RESIDUAL if any C1 fails.
3. H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED if all C3 pass.
4. H-FRESH-RATIO-SENSITIVITY otherwise.

## Governance

Historical P8-G through P8-G5 outcomes are immutable.
No replacement correctness gate is defined.
P8-H and full inference remain forbidden.

No fresh P8-G6 target experiment is run as part of the implementation/static-QA lock commit.
