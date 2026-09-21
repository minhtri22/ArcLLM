# SA1-K1 Zero-Measurement Preflight Adjudication

**Date:** 2026-09-21  
**Decision:** **PASS**

The returned `sa1_k1_preflight_return_to_chatgpt.zip` has SHA256:

```text
37366F785BB391E38E04A8FD0D631850C0DE62F13E2BACEA7FEA2A8363E14D02
```

All ten packaged entries were independently rehashed. Every hash bound by the preflight lock matches, including the raw correctness JSON, shader-build manifest, native-build manifest, executable, frozen baseline SPIR-V and candidate SPIR-V.

The frozen P7 baseline SPIR-V reproduces the Q2 SHA exactly:
`2EFD94ACDA45555AF1C082C916AF7C3F1AA1BE46AD7868B7C4BE868816CAEE4A`.

The candidate shader Git blob and component harness Git blob are identical between the scientific execution commit `b45c2cee...` and final packaging commit `e2d0b025...`.

## Correctness

All five frozen Q4 shapes were executed on banks 0 and 3: 10 cases total.

Every baseline-vs-CPU, candidate-vs-CPU and candidate-vs-baseline comparison is finite and below the frozen gates:

```text
max_abs <= 0.02
RMSE    <= 0.005
```

Worst candidate-vs-CPU case:

```text
Q4_H18944_R3584_NOBIAS, bank 3
max_abs = 0.013916015625
RMSE    = 0.00269372814522
```

Worst candidate-vs-baseline case:

```text
Q4_H18944_R3584_NOBIAS, bank 3
max_abs = 0.014404296875
RMSE    = 0.00270290781691
```

Therefore the change in accumulation order remains inside the frozen numerical contract.

## Zero-measurement boundary

The raw evidence states:

```text
model_loaded               false
performance_measurement    false
timestamp_queries          0
measured_pairs             0
performance_gate_evaluated false
```

No performance outcome has been observed.

## Packaging F0 provenance

Two packaging-only defects occurred after the valid correctness/build artifacts existed:

1. PowerShell `h/Get-History` alias collision;
2. `Test-Path$Zip` tokenization.

The returned preflight lock's single `packaging_recovery_reason` field records only the first defect. This is retained as a metadata limitation rather than rewritten. The final packaging commit and append-only lineage record both defects.

Both repairs left the candidate shader, component harness, baseline, compiled evidence and raw correctness evidence unchanged, and recovery explicitly performed no compile/build/GPU redispatch.

## Decision

```text
SA1-K1 zero-measurement preflight    PASS
implementation evidence lock         PASS
Q4 measured execution authorization PERMITTED AS A SEPARATE COMMIT

Q6 implementation                    BLOCKED
target-model execution               BLOCKED
Q3 reopen                            BLOCKED
```
