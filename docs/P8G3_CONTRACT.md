# P8-G3 Contract — arithmetic-precision attribution

## Parent evidence

P8-G2 is COMPLETE and diagnostic-valid on the exact frozen 7B target.

Authoritative P8-G2 SHA256:
- shader provenance: 43A8DEA2AD14FF516B7FDF3EB69EC3D807C455F84D53E67C7BBF00A20C4993F4
- amplification geometry: 2D183D80DD4A63CC75E10D2DC42BE08D7606B147A39FC15021E1D52E569CA168
- summary: B48FC0B48CC94363C24C0FD772058AC46C8BD3ED32203039F7FDEE230B0CB8A6

Frozen P8-G2 observations:
- A0 parent reproduction PASS.
- A1 directional gain: max=3.37789904502, RMS=5.13510196006.
- A2 linear closure FAIL: max_abs=0.000383861362934, RMSE=5.5805988593e-06.
- A3 kernel residual PASS: max ratio=0.00107700592353, RMS ratio=0.00166563879756.
- A4 linearity FAIL under the frozen 1% ratio tolerance.

A4 deviations from alpha=1 normalized values:
- alpha=0.25: max +1.45396%, RMS +0.94511%;
- alpha=0.50: max +0.96931%, RMS +0.80096%;
- alpha=0.75: max -0.16155%, RMS approximately -0.00172%.

Only alpha=0.25 max_abs/alpha exceeds the frozen 1% tolerance.

P8-G2 is frozen H-NONLINEAR/UNEXPLAINED under its preregistered protocol. This label means the FP32 closure/linearity criteria were not met; it does not establish mathematical nonlinearity in FFN-down.

## Source-level motivation

The frozen CPU Q4_K row dot decodes weights to float, multiplies in float, and sequentially accumulates 18,944 terms into a float sum. Finite-precision non-distributivity is therefore a prospective explanation for:
- W(X_GPU)-W(X_CPU) != W(X_GPU-X_CPU);
- imperfect alpha proportionality.

P8-G3 tests this explanation without changing model weights, GPU shaders, historical gates or prior verdicts.

## Frozen target and focus

Target SHA256:
60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463

Focus:
- blk.3.ffn_down.weight
- Q4_K
- n=18944
- rows=3584
- batch=4
- exact original GGUF byte span
- one physical binding slice

Reconstruct the same direct prefix through L3 SwiGLU:
- 58 GPU dispatches;
- one submit;
- direct L0->L1->L2->L3 handoff;
- no CPU teacher forcing.

Retain the exact same float X_CPU and X_GPU vectors used by P8-G2.

## R0 — frozen FP32 replay

Use the existing CPU path unchanged:
- float decoded weight;
- float product;
- float sequential accumulation;
- float output;
- float dX = X_GPU-X_CPU;
- float X_alpha = X_CPU + alpha*dX.

R0 must reproduce P8-G2 A2 and A4 within:
- metric max_abs tolerance <=1e-6;
- metric RMSE tolerance <=1e-7;
- normalized alpha-ratio relative tolerance <=0.1%.

If R0 does not reproduce, P8-G3 is INVALID.

## R1 — accumulator-only control

Change exactly one arithmetic stage:
- decoded weight remains float;
- product is still evaluated and rounded as float;
- each float product is promoted to double;
- accumulation uses double;
- final output is cast to float;
- dX and X_alpha remain the exact R0 float vectors.

This isolates sequential FP32 accumulation while preserving product precision, input representation and output representation.

Evaluate the unchanged P8-G2 criteria:
- closure max_abs<=1e-4 and RMSE<=1e-6;
- alpha normalized-ratio deviation <=1%.

## R2 — full dot-precision control

Starting from R1:
- float decoded weight is promoted to double;
- float input is promoted to double;
- product and accumulation use double;
- output remains double;
- dX and X_alpha remain the exact R0 float vectors.

This removes float product and final-output rounding while preserving the original float input algebra.

Evaluate the same closure and alpha criteria.

## R3 — input-algebra precision control

Starting from R2:
- dX_double = double(X_GPU)-double(X_CPU);
- X_alpha_double = double(X_CPU)+alpha*dX_double;
- decoded weights remain the exact same float values promoted to double;
- product, accumulation and output remain double.

Evaluate the same closure and alpha criteria.

R3 is a diagnostic high-precision control, not production execution.

## Additional diagnostics

For every regime record:
- closure max_abs / RMSE;
- alpha max_abs / RMSE;
- alpha max_abs/alpha / RMSE/alpha;
- argmax output index at every alpha;
- whether the argmax coordinate is stable across alpha.

Argmax tracking is descriptive and has no independent gate.

## Attribution

For closure and alpha separately record the earliest regime that passes:
R1, R2, R3, or NONE.

Overall classification:

H-FP32-ACCUMULATION:
- R0 reproduces;
- closure and alpha both first pass at R1.

H-DOT-ROUNDING:
- R0 reproduces;
- closure and alpha both pass by R2;
- at least one first requires R2.

H-INPUT-ROUNDING:
- R0 reproduces;
- closure and alpha both pass by R3;
- at least one first requires R3.

H-MIXED-FINITE-PRECISION:
- closure and alpha both pass by R3 but their earliest passing regimes differ.

H-HIGH-PRECISION-UNEXPLAINED:
- R0 reproduces but closure or alpha still fails at R3.

PARENT-REPRODUCTION-FAILED:
- R0 does not reproduce.

No other classification is permitted.

## Governance

P8-G3 must not:
- edit GPU shaders or model weights;
- change sequence length;
- change the historical P8-G 0.02 / 0.005 gate;
- change P8-G2 closure or alpha criteria;
- define a replacement production gate;
- revise P8-G FAIL;
- revise P8-G1 H-AMPLIFICATION;
- revise P8-G2 H-NONLINEAR/UNEXPLAINED;
- execute layer 4+;
- run decode/generation/performance;
- permit P8-H or full inference.

All higher-precision paths are CPU diagnostic controls only.

## Next

If P8-G3 attributes A2/A4 to finite precision, the next scientific step may design a separately frozen correctness-metric study. It must remain prospective and cannot rewrite historical P8-G/P8-G2 outcomes.

If R3 still fails, investigate the unexplained arithmetic discrepancy before any correctness-gate study.

P8-H remains blocked.
