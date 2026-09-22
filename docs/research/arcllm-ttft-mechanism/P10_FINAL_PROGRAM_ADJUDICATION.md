# ARCLLM_TTFT_M1 — P10 Final Program Adjudication

**Date:** 2026-09-22  
**Final status:** `TTFT_M1_PROGRAM_CLOSED_INFRASTRUCTURE_STOP_NO_MECHANISM_RESULT`

## Final answer

The program's research question is **not adjudicated**.

TTFT_M1 produced a rigorous causal design, but it did not reach a valid BuildOnly H-ART result or any fresh timing collection.

## Stage record

```text
P0  origin/governance                     COMPLETE
P1  prior-art + source review             COMPLETE
P2  TTFT path + hypotheses                COMPLETE
P3  causal decomposition                  COMPLETE
P4  falsification/STOP contract           COMPLETE
P5  zero-science QA                       PASS
P6  diagnostic implementation             STOP_INFRASTRUCTURE_UNSTABLE
P7  fresh identification                  NOT OPENED
P8  intervention specification            NOT OPENED
P9  intervention confirmation             NOT OPENED
P10 final adjudication                    TERMINAL
```

## Why there is no mechanism result

The first P6 pre-run defect concerned H-ART operationalization. The single P6 BuildOnly repair allowance was consumed to correct it.

After lock v0.2 froze that correction with no further repair permitted, audit found a second runner provenance defect: the runner preflight used lock v0.2, but its result/package section still labeled or resolved the implementation lock as v0.1.

No runner had been executed yet.

Repairing that second defect would violate the frozen no-further-repair rule. Therefore P6 stopped before science.

## Scientific non-results

The following remain **not adjudicated**:

```text
H-ART
H-DPIPE
H-PRECOND
H-STATE-INTERACTION
H-NULL
TTFT-removal intervention
```

Execution accounting:

```text
BuildOnly runs                  0
diagnostic executable launches  0
target model loads              0
GPU dispatches                  0
fresh TTFT observations         0
parent P6 timings reused        0
```

## What remains useful

The program preserves methodological evidence:
- independent provenance from terminal ANL64;
- prior-art review before implementation;
- exact TTFT timer/source mapping;
- common measured-prefill source identity;
- finite hypothesis set;
- preregistered 2×2 factorial causal design;
- frozen falsification and stop rules;
- zero parent-timing reuse.

Those are design assets, not mechanism results.

## Parent ANL64 remains unchanged

```text
ANL64_PROGRAM_CLOSED_VALID_NEGATIVE_TTFT_BLOCKED
```

TTFT_M1 does not retroactively alter that conclusion.

## Closure

The following are forbidden inside this program:
- patching the runner and continuing;
- creating implementation lock v0.3;
- opening P7;
- renaming a continuation as TTFT_M1;
- treating unexecuted H-ART logic as empirical evidence.

Any future attempt must be a new independent program with a fresh branch and fresh lineage.

```text
NEXT = NONE_PROGRAM_TERMINAL
LINEAGE = CLOSE AFTER FINAL APPEND
```
