# P8-G2 Contract — amplification geometry qualification

## Parent evidence

P8-G1 is COMPLETE and diagnostic-valid on the exact frozen 7B target.

Authoritative P8-G1 SHA256:
- shader provenance: A4B095E1F2E7CD4AD78EE74C06A96323DF258C2ADB2CCA7DE827816191019740
- causal decomposition: 416A97CD13A585BD4CAB397A3B3503BA8A87BD94F13BB1F5664F384603359499
- summary: 1E04C9BF94DA57258D9DEA36FAE3F71B896D9F0CA773BF360305742397C27B1C

P8-G1 reproduced both parent signals:
- X_GPU vs X_CPU at L3 SwiGLU: max_abs=0.00838851928711, RMSE=7.10637175468e-05, frozen gate PASS;
- C0 actual-path down failure: max_abs=0.0283279418945, RMSE=0.000364843778882, frozen max_abs gate FAIL.

Kernel controls:
- P7-G tiled16 on X_CPU: max_abs=6.103515625e-05, RMSE=7.36516371841e-07, PASS;
- P7-C tiled8 on X_CPU: identical metrics;
- P7-G tiled16 on X_GPU: max_abs=3.0517578125e-05, RMSE=6.0782396936e-07, PASS;
- P7-C tiled8 on X_GPU: identical metrics;
- P7-G vs P7-C direct difference is exactly 0 on both inputs.

CPU-only input amplification C3:
- max_abs=0.0283355712891;
- RMSE=0.000364919435264;
- frozen max_abs gate FAIL.

P8-G1 therefore supports H-AMPLIFICATION and rejects the tested tiled16-specific and common-Q4 explanations for this boundary.

P8-G remains historically frozen FAIL. P8-G2 may not revise that verdict.

## Scientific question

Is the P8-G L3 FFN-down gate crossing quantitatively explained by linear propagation of the already-observed upstream perturbation through the exact Q4_K down operator, with GPU kernel residual negligible relative to propagated error?

P8-G2 is an error-geometry study. It does not define a replacement correctness gate.

## Frozen target and focus

Exact model SHA256:
60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463

Focus:
- tensor: blk.3.ffn_down.weight
- format: Q4_K
- n=18944
- rows=3584
- batch=4
- no bias
- exact original GGUF byte span
- one physical P8 binding slice

Reconstruct the same direct prefix through L3 SwiGLU as P8-G1:
- 58 GPU dispatches;
- one submit;
- direct L0->L1->L2->L3 handoff;
- no CPU teacher forcing in the prefix.

Retain:
- X_CPU
- X_GPU
- ΔX = X_GPU - X_CPU
- Y_CC = CPU_down(X_CPU)
- Y_CG = CPU_down(X_GPU)
- Y_GG = P7-G_down(X_GPU)

## Required analyses

### A0 — parent reproduction

Reproduce P8-G1 X_GPU vs X_CPU and C3 within deterministic tolerance:
- max_abs tolerance <= 1e-6;
- RMSE tolerance <= 1e-7.

If parent reproduction fails, P8-G2 is INVALID.

### A1 — directional amplification

Compute:
- ΔY_prop = Y_CG - Y_CC
- observed max-direction gain = max_abs(ΔY_prop) / max_abs(ΔX)
- observed RMS-direction gain = RMS(ΔY_prop) / RMS(ΔX)

These are empirical gains along the observed perturbation direction, not global operator norms.

Record signal scales:
- RMS(X_CPU), max_abs(X_CPU)
- RMS(Y_CC), max_abs(Y_CC)

Record normalized perturbation/error:
- RMS(ΔX) / RMS(X_CPU)
- max_abs(ΔX) / max_abs(X_CPU)
- RMS(ΔY_prop) / RMS(Y_CC)
- max_abs(ΔY_prop) / max_abs(Y_CC)

No pass/fail threshold is assigned to these normalized values in P8-G2.

### A2 — linear propagation closure

Compute ΔY_direct = CPU_down(ΔX).

Compare ΔY_direct against ΔY_prop.

Pre-registered closure criterion:
- finite;
- max_abs <= 1e-4;
- RMSE <= 1e-6.

PASS supports that the observed downstream error is explained by the linear down operator acting on the upstream perturbation, within ordinary floating-point evaluation differences.

FAIL means the simple linear-propagation explanation is incomplete and requires another decomposition.

### A3 — kernel-residual budget

Compute:
- ε_kernel = Y_GG - Y_CG
- total_actual = Y_GG - Y_CC

Record max_abs and RMSE for both.

Compute contribution ratios:
- kernel_max_ratio = max_abs(ε_kernel) / max_abs(ΔY_prop)
- kernel_rms_ratio = RMS(ε_kernel) / RMS(ΔY_prop)

Pre-registered propagation-dominance criterion:
- kernel_max_ratio <= 0.01
- kernel_rms_ratio <= 0.01

This criterion does not make P8-G PASS. It only tests whether kernel residual contributes less than 1% of the propagated error on both recorded norms.

### A4 — perturbation scaling series

For α in the frozen set:
{0.25, 0.50, 0.75, 1.00}

Construct:
X_α = X_CPU + α * ΔX

Compute with CPU only:
Y_α = CPU_down(X_α)

Compare each Y_α with Y_CC.

Record:
- max_abs
- RMSE
- max_abs / α
- RMSE / α
- whether the historical frozen P8-G gate 0.02 / 0.005 would pass at that α.

Linearity qualification:
for α > 0, max_abs/α and RMSE/α must remain within 1% of the α=1.00 values.

This series is diagnostic. It cannot redefine or relax the historical P8-G threshold.

## Frozen hypotheses

H-GEOMETRIC-AMPLIFICATION:
- A0 PASS;
- A2 PASS;
- A3 propagation-dominance PASS;
- A4 linearity qualification PASS.

This means the L3 gate crossing is quantitatively explained by ordinary linear amplification of the upstream perturbation under the exact down operator, with negligible GPU-kernel contribution.

H-NONLINEAR/UNEXPLAINED:
- A0 PASS but A2 or A4 FAIL.

H-KERNEL-CONTRIBUTION:
- A0 PASS, A2 PASS, but A3 propagation-dominance FAIL.

PARENT-REPRODUCTION-FAILED:
- A0 FAIL.

No other classification is permitted.

## Governance

P8-G2 must not:
- change any shader;
- change weights;
- change sequence length;
- change P8-G or P8-G1 evidence;
- change the historical max_abs=0.02 / RMSE=0.005 gates;
- declare a new production correctness gate;
- execute layer 4+;
- perform decode/generation/performance trials;
- permit P8-H or full inference.

## Next

If H-GEOMETRIC-AMPLIFICATION is supported, the next scientific step is a separately frozen correctness-gate study for compositional/scale-aware validation. That future study may define a new protocol, but it may not rewrite P8-G's historical FAIL.

If H-NONLINEAR/UNEXPLAINED or H-KERNEL-CONTRIBUTION is supported, investigate that obstruction before any gate study.

P8-H remains blocked.
