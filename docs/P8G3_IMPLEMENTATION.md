# P8-G3 Implementation — arithmetic-precision attribution

Date: 2026-09-20

## Purpose

P8-G3 attributes the frozen P8-G2 A2/A4 misses to specific finite-precision stages, if possible.

It preserves all historical evidence and thresholds:
- P8-G = FAIL;
- P8-G1 = H-AMPLIFICATION;
- P8-G2 = H-NONLINEAR/UNEXPLAINED;
- P8-G2 closure gate = max_abs <= 1e-4 and RMSE <= 1e-6;
- P8-G2 alpha normalized-ratio tolerance = 1%.

No replacement production gate is defined.

## Prefix reproduction

The implementation reuses the P8-G2 prefix exactly:
- L0-L2 full;
- L3 through SwiGLU;
- 58 GPU dispatches;
- one submit;
- direct GPU handoff;
- no CPU teacher forcing;
- no layer 4+.

The only GPU work in P8-G3 is this frozen prefix. All arithmetic-precision controls are CPU-only.

## Exact observed inputs

After prefix completion:
- X_CPU is the independent CPU L3 SwiGLU vector;
- X_GPU is the actual GPU L3 SwiGLU vector.

R0 forms:
- dX_float = X_GPU - X_CPU in float;
- X_alpha_float = float(double(X_CPU) + alpha*double(dX_float)).

R3 separately forms:
- dX_double = double(X_GPU) - double(X_CPU);
- X_alpha_double = double(X_CPU) + alpha*dX_double.

Frozen alpha set:
{0.25, 0.50, 0.75, 1.00}.

## Fused Q4_K evaluator

To avoid repeatedly decoding the same 18,944-term Q4_K row, the CPU diagnostic evaluator decodes each weight once and updates the R0/R1/R2/R3 accumulators for that output coordinate in the same k=0..18943 order.

Output coordinates are independent and may be evaluated across up to eight CPU worker threads. Threading never changes the within-dot term order and therefore cannot change a regime's arithmetic definition.

For every decoded float weight wf and float input xv:

R0:
- float prod = wf*xv;
- float accumulator;
- float output.

R1:
- the same float prod is promoted to double;
- double accumulator;
- cast final sum back to float.

R2:
- double(wf)*double(xv);
- double accumulator;
- double output;
- still uses R0 float input algebra.

R3:
- same double dot arithmetic as R2;
- uses dX_double / X_alpha_double.

R2 and R3 share the same double-dot X_CPU/X_GPU results because those observed vectors remain the original float values promoted to double.

## R0 reproduction lock

R0 must reproduce the authoritative P8-G2 A2 closure and all four A4 alpha metrics.

Frozen reproduction tolerances:
- max_abs metric absolute difference <= 1e-6;
- RMSE metric absolute difference <= 1e-7;
- normalized max_abs/alpha and RMSE/alpha relative difference <= 0.1%.

If reproduction fails, the run is INVALID and no precision attribution is accepted.

## Per-regime outputs

For R0-R3 the executable records:
- closure max_abs / RMSE / finite / pass;
- four alpha max_abs / RMSE values;
- max_abs/alpha;
- RMSE/alpha;
- argmax absolute-error output index;
- alpha-linearity pass;
- whether the argmax index is stable across alpha.

Argmax stability is descriptive only.

## Attribution

Closure and alpha separately receive the earliest passing regime:
- R1;
- R2;
- R3;
- NONE.

Overall classification:
- H-FP32-ACCUMULATION
- H-DOT-ROUNDING
- H-INPUT-ROUNDING
- H-MIXED-FINITE-PRECISION
- H-HIGH-PRECISION-UNEXPLAINED
- PARENT-REPRODUCTION-FAILED

When closure and alpha first pass at different finite-precision regimes, the more specific H-MIXED-FINITE-PRECISION classification is used.

## Governance

The result always records:
- replacement_gate_defined=false;
- p8g_verdict_changed=false;
- p8g1_verdict_changed=false;
- p8g2_verdict_changed=false;
- p8h_permitted=false;
- full_inference_permitted=false.

No target experiment is run as part of the implementation/lock commit.
