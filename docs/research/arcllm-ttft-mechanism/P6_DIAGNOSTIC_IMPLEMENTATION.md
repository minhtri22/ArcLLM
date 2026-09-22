# ARCLLM_TTFT_M1 — P6 Diagnostic Implementation

**Date:** 2026-09-22  
**State:** IMPLEMENTED / STATIC QA PASS / BUILDONLY PENDING

P6 introduces an isolated diagnostic harness only:

```text
src/ttft_m1_diagnostic.cpp
```

Production ANL64, safe Q2 and all existing shader sources remain unchanged.

## Factorial controls

The harness exposes exactly:

```text
SP = SAFE   + PREFILL_ONLY
SF = SAFE   + FULL_INFERENCE
QP = Q4FAST + PREFILL_ONLY
QF = Q4FAST + FULL_INFERENCE
```

For every measured attempt:

```text
reset
→ conditioning
→ reset
→ t0
→ exact common 441-dispatch prefill
→ q2_top2
→ t1
```

There is no separate warmup. Conditioning is the preregistered factor.

## Common prefill invariant

The complete `build_prefill` source region in the diagnostic harness is byte-for-byte identical to frozen `src/q2_benchmark.cpp`.

Q4FAST references in measured prefill:

```text
0
```

Q4FAST references in decode:

```text
5
```

and only Q/K/O/gate/up are factor-controlled. V/down remain on the safe Q4/Q6 paths.

## Diagnostic endpoints

Primary:

```text
ttft_ms
```

Secondary localization only:

```text
prefill_execute_wall_ms
submit_wait_ms
host_record_and_lifecycle_ms
top2_ms
```

The harness does not read parent P6 timing artifacts.

## H-ART BuildOnly gate

P6 builds two independent shader artifact directories from the frozen sources with pinned glslang 16.5.0 / Vulkan 1.2:

```text
shaders_safe
shaders_q4fast
```

All 16 common shader SPIR-V files must match exactly.

The Q4FAST-only shader must reproduce:

```text
B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569
```

If any common artifact differs:

```text
H_ART_SUPPORTED_STATIC_STOP_TIMING
```

If all match:

```text
H_ART_FALSIFIED_STATIC
```

This BuildOnly stage never loads the target model or executes the diagnostic binary.

## Static QA

Connector-equivalent checks PASS:
- frozen production source identities unchanged;
- exact common prefill source equality;
- factor/arm mapping exact;
- conditioning before measured t0;
- five measured attempts only;
- Q4FAST confined to five decode roles;
- parent P6 timings not read;
- H-ART two-set compilation contract present;
- native BuildOnly cannot launch the executable.

Current result:

```text
PASS_CONNECTOR_EQUIVALENT_P6_STATIC_QA_BUILDONLY_PENDING
```


## Pre-run H-ART implementation correction

Before any P6 BuildOnly runner was executed, static review found that the first H-ART implementation compared two fresh compilations of the same common sources. That was insufficient to establish historical SAFE-vs-candidate artifact identity.

The defect was adjudicated as:

```text
P6_PRERUN_H_ART_IMPLEMENTATION_DEFECT_NO_SCIENCE_CONSUMED
```

No build, model load, GPU dispatch, timing, or P7 observation had occurred.

The corrected H-ART method is:

```text
candidate exact source/toolchain reproduction
        ↓
compare each of 16 reproduced common SPIR-V hashes
against frozen historical SAFE hashes
from config/q2_execution_authorization.json
        ↓
cross-check Q4-safe/Q6-safe reproduced hashes
against ANL64 P4 historical BuildOnly adjudication
        ↓
reproduce historical Q4FAST hash
```

Frozen historical inputs:
- SAFE authorization blob `20f556fb18ee4ffd0ce5e17cf8fab1a2b9a992dc`;
- ANL64 P4 BuildOnly adjudication blob `e9039de1af496cf724a390e8180ae7d97e99f091`.

The hypothesis, support/falsification rule, workloads, arms, 1.10 materiality threshold and future P7 collection are unchanged.

The correction conservatively consumes the single P6 BuildOnly repair allowance. No further P6 BuildOnly repair is permitted after lock v0.2.
