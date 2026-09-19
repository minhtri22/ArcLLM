# P8-G1 Implementation — L3 FFN-down causal decomposition

Date: 2026-09-19

## Purpose

P8-G1 diagnoses the frozen P8-G failure at L3.ffn_down without changing any model weight, shader, correctness threshold, or P8-G verdict.

## Stage 1 — reproduce the upstream state

The implementation reuses the P8-G execution path but stops layer 3 immediately after SwiGLU.

- L0, L1 and L2 execute the full frozen 15-operation chain.
- L3 executes the first 13 operations through SwiGLU.
- total: 58 dispatches;
- one submit;
- direct GPU handoff L0 -> L1 -> L2 -> L3 remains intact.

After the submit, the implementation reads the actual GPU L3 SwiGLU vector as X_GPU.

The independent CPU reference computes the same four-layer prefix and retains CPU L3 SwiGLU as X_CPU.

X_GPU vs X_CPU must reproduce the parent P8-G L3.swiglu metric within the frozen reproduction tolerance.

## Stage 2 — causal down-projection A/B

The focus tensor is exactly blk.3.ffn_down.weight:
- Q4_K;
- n=18944;
- rows=3584;
- batch=4;
- no bias;
- one physical binding slice;
- exact original GGUF byte span.

CPU outputs:
- Y_CC = CPU_down(X_CPU)
- Y_CG = CPU_down(X_GPU)

GPU outputs:
- Y_GC_P7G = P7-G Q4_K tiled16(X_CPU)
- Y_GG_P7G = P7-G Q4_K tiled16(X_GPU)
- Y_GC_P7C = P7-C Q4_K tiled8(X_CPU)
- Y_GG_P7C = P7-C Q4_K tiled8(X_GPU)

These four GPU diagnostics execute as exactly four dispatches in one separate submit.

The teacher-forced X_CPU buffer is used only in this diagnostic stage. It is never injected into the reconstructed P8-G prefix.

## Comparisons

The executable records C0..C6 exactly as frozen in docs/P8G1_CONTRACT.md. CPU-vs-GPU correctness comparisons use the unchanged max_abs<=0.02 and RMSE<=0.005 gates.

P8-G1 reports one classification from:
- H-KERNEL
- H-COMMON-Q4
- H-AMPLIFICATION
- H-INTERACTION
- INPUT_DEPENDENT_TILED_KERNEL_SENSITIVITY
- INCONCLUSIVE
- PARENT_REPRODUCTION_FAILED

No classification changes P8-G from FAIL.

## Reproduction guard

The diagnostic is valid only if:
- exact P8-G authoritative parents are supplied;
- X_GPU vs X_CPU reproduces the frozen P8-G L3.swiglu metric;
- C0 reproduces the frozen P8-G L3.ffn_down failure;
- prefix execution is exactly 58 dispatches / one submit;
- diagnostic execution is exactly four dispatches / one submit;
- graph binding and focus tensor source span are exact.

## Exclusions

No layer 4+, embedding, output norm, LM-head, decode, generation, performance trial, shader edit or numerical-gate edit is introduced.
