# P8-G2 Implementation — amplification geometry qualification

Date: 2026-09-19

## Purpose

P8-G2 measures the geometry of the already-observed P8-G/P8-G1 error amplification. It does not modify the historical correctness gate and cannot change P8-G from FAIL.

## Prefix reconstruction

The implementation reuses the P8-G1 direct GPU prefix:

- L0-L2 execute the complete frozen 15-operation layer chain.
- L3 executes through SwiGLU only.
- exactly 58 dispatches;
- exactly one submit;
- direct GPU hidden-state handoff L0 -> L1 -> L2 -> L3;
- no CPU teacher forcing enters this prefix.

The independent CPU chain produces X_CPU = L3 CPU SwiGLU. The GPU prefix produces X_GPU = L3 GPU SwiGLU.

A0 requires both the parent X metric and the P8-G1 CPU-only C3 down-projection metric to reproduce within the frozen deterministic tolerances.

## Exact focus

The only investigated operator is blk.3.ffn_down.weight:
- Q4_K;
- n=18944;
- rows=3584;
- batch=4;
- no bias;
- one physical binding slice;
- exact original GGUF byte span.

## A1 — observed directional gain

The implementation forms:
- dX = X_GPU - X_CPU;
- Y_CC = CPU_down(X_CPU);
- Y_CG = CPU_down(X_GPU);
- dY_prop = Y_CG - Y_CC.

It records max-absolute and RMS norms of dX and dY_prop, their empirical directional gains, reference signal scales for X_CPU and Y_CC, and normalized perturbation/error ratios.

These are descriptive measurements only.

## A2 — linear propagation closure

The implementation independently computes:

dY_direct = CPU_down(dX)

and compares dY_direct against dY_prop using the frozen P8-G2 closure gate:
- max_abs <= 1e-4;
- RMSE <= 1e-6;
- finite values required.

## A3 — kernel residual budget

One additional GPU dispatch executes the frozen production P7-G Q4_K tiled16 down kernel on X_GPU:
- Y_GG = P7-G_down(X_GPU).

The implementation forms:
- kernel residual = Y_GG - Y_CG;
- total actual error = Y_GG - Y_CC.

Kernel max/RMS residuals are divided by propagated max/RMS errors. Propagation dominance requires both ratios <= 0.01.

This diagnostic dispatch executes in one separate submit after the 58-dispatch prefix. No P7-C comparison is repeated because P8-G1 already established exact P7-G/P7-C identity for this boundary.

## A4 — frozen perturbation scaling series

For alpha in {0.25, 0.50, 0.75, 1.00}:

X_alpha = X_CPU + alpha * dX

CPU_down(X_alpha) is compared with Y_CC.

The implementation records:
- max_abs;
- RMSE;
- max_abs / alpha;
- RMSE / alpha;
- whether the historical P8-G 0.02 / 0.005 gate would pass.

Linearity requires both normalized quantities at every alpha to stay within 1% of the alpha=1.00 values.

## Classification

Only these outcomes are emitted:
- H-GEOMETRIC-AMPLIFICATION;
- H-NONLINEAR/UNEXPLAINED;
- H-KERNEL-CONTRIBUTION;
- PARENT-REPRODUCTION-FAILED.

P8-G remains frozen FAIL in every outcome. P8-G2 never defines a replacement correctness gate and never permits P8-H or full inference.
