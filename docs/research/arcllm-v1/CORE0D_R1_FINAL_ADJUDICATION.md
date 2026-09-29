# CORE-0D-R1 — Final Scientific Adjudication

Date: 2026-09-29

## Verdict

**PASS_CORE0D_R1_EXCESS_COST_ATTRIBUTION_COMPLETE**

Open findings: **0**.

The fresh prospective 36-request successor collection completed under the frozen common teacher-forced design. Independent adjudication recomputed G0 through G4 from the immutable dataset and all five gates PASS.

Dataset:

`results/core0d_measured_20260929T121751651960Z`

Frozen dataset commit:

`630406103254b4358ab32a758b73c0fb4d4742f9`

Independent adjudication commit:

`7e1e264c1de2eb52713d49c2ec54d1d9d4b26d0e`

## Gate results

```text
G0 common trajectory validity       PASS
G1 control transfer to CORE-0B      PASS
G2 trace transfer                   PASS
G3 GPU measurement qualification    PASS
G4 residual closure                 PASS
```

The repaired parser qualified the pinned llama Vulkan surface as exactly one prefill group plus thirty-one cached-decode groups. ArcLLM preserved one 441-dispatch prefill trace plus thirty-one 469-dispatch decode traces with the frozen Token-XRay contract.

## G1 — uninstrumented transfer

W-S CONTROL ArcLLM/llama ratios had median **6.4683×**, compared with the frozen CORE-0B reference **6.9139×**. Transfer factor was **1.0689** and max/min spread **1.2332**.

W-C CONTROL median was **8.3783×**, versus CORE-0B **8.6323×**. Transfer factor was **1.0303** and max/min spread **1.3829**.

Both workloads therefore remain in the preregistered CORE-0B performance regime.

## G2 — trace transfer

W-S TRACE median ArcLLM/llama ratio was **6.7712×**, within factor **1.0468** of its CONTROL median.

W-C TRACE median was **7.5549×**, within factor **1.1090** of its CONTROL median.

All six TRACE pairs retained positive ArcLLM total excess.

## G4 — two-region excess decomposition

The primary decomposition is:

```text
DeltaE = total ArcLLM - llama request excess
DeltaG = token-GPU execution excess
DeltaH = outside-token-GPU residual excess

DeltaE = DeltaG + DeltaH
```

Arithmetic closure error was zero at the stored precision for all six TRACE pairs.

### W-S

Median total excess:

`60,682.39 ms`

Median token-GPU excess:

`23,722.48 ms`

Median outside-token-GPU excess:

`36,959.91 ms`

Median excess shares:

```text
f_G = 0.39093
f_H = 0.60907
```

### W-C

Median total excess:

`77,240.38 ms`

Median token-GPU excess:

`48,219.44 ms`

Median outside-token-GPU excess:

`36,827.23 ms`

Median excess shares:

```text
f_G = 0.53889
f_H = 0.46111
```

Thus the workload dependence is real: W-S has more excess outside token-GPU execution, while W-C has more excess inside token-GPU execution.

## Preregistered Amdahl gate

Both regions satisfy eligibility.

### G — token GPU

```text
q_W-S = 0.33439
q_W-C = 0.47646

robust score = 0.33439
eligible = true
```

Idealized full-elimination upper-bound speedups:

```text
W-S = 1.502×
W-C = 1.910×
```

### H — outside token GPU

```text
q_W-S = 0.52098
q_W-C = 0.40769

robust score = 0.40769
eligible = true
```

Idealized full-elimination upper-bound speedups:

```text
W-S = 2.088×
W-C = 1.688×
```

The frozen single-region priority rule requires the winner to exceed the other region by at least **0.05** absolute current-wall fraction.

Observed robust margin:

```text
H - G = 0.07331
```

Therefore the preregistered decision is:

**H_OUTSIDE_TOKEN_GPU**

## Important interpretation boundary

This does **not** mean that the excess is already proven to be model loading, Vulkan setup, CPU orchestration, allocation, file I/O, buffer preparation, or any other particular host mechanism.

H is still the arithmetic residual:

`request wall - observed token-GPU execution`.

PHASE_ONLY measurements provide descriptive candidates, but CORE-0D-R1 did not preregister a causal phase-selection rule inside H. Those phase measurements therefore cannot be used post hoc to declare the winning submechanism.

Likewise, G remains materially important. The decision is priority under the frozen robust Amdahl rule, not a claim that GPU kernels no longer matter.

## Scientific consequence

CORE-0D-R1 is formally closed PASS.

The only scientifically authorized successor is:

**CORE-0E_OUTSIDE_GPU_PHASE_ATTRIBUTION_PREREGISTRATION**

That successor should prospectively freeze a partition of H into observable outside-token-GPU phases and corresponding attribution gates before inspecting fresh outcomes.

No runtime optimization, kernel change, NPU integration, or specific outside-GPU mechanism is authorized by CORE-0D-R1 alone.
