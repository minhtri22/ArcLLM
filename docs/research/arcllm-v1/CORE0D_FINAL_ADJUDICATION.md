# CORE-0D — Final Scientific Adjudication

Date: 2026-09-29

## Verdict

**STOP_CORE0D_GPU_MEASUREMENT_NOT_QUALIFIED**

The frozen 36-request primary collection completed operationally, but the preregistered GPU measurement gate did not qualify. This is a scientific STOP for the current CORE-0D study, not a runtime crash.

## Frozen primary collection

Dataset:

`results/core0d_measured_20260929T095154508631Z`

Frozen commit:

`bce0d1c84afd953264d1efb35ee93d575e34dd49`

Exactly 36 measured requests were executed. There were no process-level result failures.

The CONTROL and PHASE_ONLY matched pairs completed validly. All six GPU_TRACE matched pairs failed the same llama-side parser qualification:

```text
required Vulkan timing groups = 32
executed parser observed       = 31
```

No selective rerun occurred.

## Gate consequence

The study stops at **G3 — GPU_MEASUREMENT_QUALIFICATION**.

The following remain deliberately unopened or unadjudicated:

```text
G0 common trajectory       not formally adjudicated
G1 control transfer        not formally adjudicated
G2 trace transfer          not adjudicated
G3 GPU measurement         FAIL
G4 residual closure        NOT OPEN
Amdahl gate                NOT OPEN
```

No G/H attribution is computed from this failed primary collection.

## Outcome-blind structural audit

After freezing the incomplete collection, the six raw llama Vulkan perf logs were audited using only source structure, group cardinality, operation shape and numeric lexical class. Timing magnitudes were not used.

Both the primary audit and an independent checker returned PASS with zero findings.

The exact raw logger surface contains **32 groups on every one of the six requests**:

```text
group 0      = prefill
groups 1–31  = cached decode steps 0–30
```

Structural evidence is exact:

- W-S group 0 carries query sequence length 4;
- W-C group 0 carries query sequence length 256;
- groups 1–31 carry query sequence length 1;
- all six logs contain 32 `Vulkan Timings:` sections;
- all six logs contain 32 `Total time` records under a numeric grammar that accepts exponent notation.

Therefore **no Vulkan timing group is missing**.

## Root cause

The pinned llama logger prints:

`Total time: <double> us.`

through default C++ stream formatting.

For all six raw logs, the first/prefill total is rendered in scientific notation. The remaining 31 totals are rendered as ordinary decimal/integer forms.

The executed CORE-0D parser was frozen to a decimal-only grammar:

```text
[0-9]+(?:\.[0-9]+)?
```

and therefore ignored the scientific-notation prefill total. That deterministically produced the apparent cardinality of 31.

The failure is consequently a **parser numeric-grammar defect**, not a missing llama GPU graph.

## Why the current study still STOPs

The defect was discovered only after the measured primary collection had completed and outcome had opened.

CORE-0D therefore may not:

- repair the parser and reparse the same dataset into PASS;
- rerun only the six GPU_TRACE requests;
- change G3 cardinality to 31;
- change G0–G4 thresholds;
- compute G/H or Amdahl results from this collection.

Doing any of those would be post-hoc rescue.

## Scientific consequence

Current CORE-0D is formally closed with:

**STOP_CORE0D_GPU_MEASUREMENT_NOT_QUALIFIED**

The outcome-blind audit does, however, justify one narrow prospective successor: repair only the numeric grammar used to parse llama `Total time` records, while preserving the exact 32-group requirement, 36-request design, teacher-forced trajectories, G0–G4 gates and Amdahl thresholds unchanged.

That successor must pass a fresh implementation/static preflight before any new measured request.
