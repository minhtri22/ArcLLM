# CORE-0D-R1 — llama Vulkan Perf Numeric-Grammar Repair Preregistration

Date: 2026-09-29

Status: **PREREGISTERED_NOT_AUTHORIZED_FOR_MEASURED_EXECUTION**

## Parent result

CORE-0D is closed:

**STOP_CORE0D_GPU_MEASUREMENT_NOT_QUALIFIED**

The frozen parent dataset is permanently retained as failed primary evidence and is not reparsed into PASS.

Outcome-blind audit established that the raw pinned llama Vulkan logger actually produced the required structure on all six TRACE requests:

```text
32 raw groups
= 1 prefill
+ 31 cached-decode groups
```

The parent parser reported 31 only because it accepted decimal notation but rejected the scientific notation used by the first/prefill `Total time` line.

## Only authorized change

CORE-0D-R1 may change exactly one implementation contract: the numeric grammar used to parse llama `Total time` records.

Old grammar:

```text
[0-9]+(?:\.[0-9]+)?
```

New frozen grammar:

```text
[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?
```

The parser must additionally require finite values and exactly 32 structurally qualified records.

The new grammar accepts the default C++ numeric forms required by the pinned logger, including scientific notation. It does not change units or timing semantics.

## Structural qualification remains mandatory

A 32-record parse is insufficient by itself.

Before accepting the totals:

```text
record 0      must be prefill-shaped
records 1–31 must be cached-decode-shaped
```

Expected query sequence geometry remains:

```text
W-S prefill: q sequence length 4
W-C prefill: q sequence length 256
decode:      q sequence length 1
```

## Everything else is unchanged

The following are copied without modification from CORE-0D:

- exact model;
- pinned llama.cpp commit;
- Token-XRay pin;
- W-S and W-C prompts;
- both 31-token teacher-forced continuations;
- 3 blocks per workload;
- CONTROL / PHASE_ONLY / GPU_TRACE modes;
- system ordering;
- mode ordering;
- 36 total measured requests;
- G0 common-trajectory gate;
- G1 control-transfer thresholds;
- G2 trace-transfer thresholds;
- G3 exact 1+31 GPU-group requirement;
- G4 1-microsecond arithmetic closure;
- CORE-0B performance authority;
- all Amdahl formulas, eligibility thresholds and winner margins.

No threshold may be changed after this preregistration.

## Parent dataset boundary

The failed CORE-0D dataset may be used only as a **structure-only regression fixture** during zero-science parser qualification.

It may not be used to:

- calculate repaired G/H outcomes;
- tune scientific thresholds;
- choose a region;
- create an Amdahl result;
- serve as CORE-0D-R1 primary evidence.

No six-arm selective rerun is allowed.

CORE-0D-R1 requires a fresh complete prospective collection if preflight passes.

## Zero-science parser fixtures

PASS must cover:

```text
12345
12345.5
1.2345e+05
1.2345E+05
1.2345e-05
```

and reject non-finite textual forms such as `nan` and `inf`.

A structural fixture must contain exactly one prefill-shaped block followed by exactly 31 decode-shaped blocks.

## Current authorization

Authorized:

- numeric-grammar parser repair;
- static preflight;
- zero-science fixtures;
- independent preflight;
- execution-lock construction.

Not authorized:

- measured inference;
- any new 36-request collection;
- canonical runtime/kernel/shader changes;
- llama source changes;
- NPU, counters or profiler;
- any mechanism selection.

Next:

**CORE0D_R1_PARSER_REPAIR_IMPLEMENTATION_STATIC_PREFLIGHT_AND_EXECUTION_LOCK**
