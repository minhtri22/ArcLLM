# ArcLLM Program-Level Decision

**Date:** 2026-09-21  
**Decision:** `STOP_CURRENT_SUCCESSOR_LINE`  
**Return to end-to-end validation:** **NO**

## Decision question

After Q6CB terminated as `STOP_INFRASTRUCTURE_UNSTABLE` with no causal result, the program-level choice was:

```text
RETURN_TO_END_TO_END_VALIDATION
or
STOP_CURRENT_SUCCESSOR_LINE
```

The decision is based on the complete ArcLLM evidence chain, not on a Q6CB causal interpretation.

## 1. Legacy architecture evidence

The validated original ArcLLM architecture already completed real-model end-to-end evaluation.

Its final verdict is:

```text
FEASIBLE_NO_DEMONSTRATED_ADVANTAGE
```

The legacy architecture is feasible and reproducible, but its validated line is already closed because it did not demonstrate the preregistered practical regime advantage against the matched llama.cpp baseline.

Therefore “return to E2E” cannot mean rerunning or reopening the legacy architecture.

## 2. Why SA-H1 was opened

The post-verdict review selected one bounded successor candidate:

```text
SA-H1
Decode-Specialized Packed-Quant Executor
```

It was a research-reopen candidate, not an implementation or target-execution authorization.

The successor scope explicitly covered exact batch-1 packed `Q4_K/Q6_K` decode dataflow. The exact 7B shape universe included Q4 projection families plus legal Q6 V/down paths, and both quant families were preregistered specifically so the study could not keep only the favorable quant type after observing results.

## 3. SA0 and SA1 evidence

SA0 established that the exact Arc 140V target exposes the required Vulkan/subgroup capabilities. That establishes implementability options; it is not evidence of successor performance or E2E value.

SA1 then produced asymmetric evidence.

### Q4

`Q4_STAGE_PASS` is valid closed component evidence.

The fixed subgroup-32 split-K mechanism produced approximately:

```text
Process A geomean speedup: 3.13715x
Process B geomean speedup: 3.14750x
```

with every frozen Q4 cell passing the component gate.

This remains a component-level result. SA1 explicitly states `end_to_end_claim=false`.

### Q6

The unchanged mechanism and geometry failed the frozen Q6 correctness preflight:

```text
Q6_STAGE_FAIL_CORRECTNESS
```

No Q6 performance measurement ran and the target model was never loaded.

The valid SA1 synthesis is therefore:

```text
Q4 mechanism efficacy                    SUPPORTED in frozen component scope
unchanged Q4→Q6 mechanism generality     FALSIFIED within SA1
end-to-end applicability                  NOT TESTED
```

SA1 explicitly forbids target-model integration from that stage.

## 4. Q6CB supplies no causal promotion evidence

Q6CB was opened only to identify the Q6 correctness boundary without rescuing SA1.

It terminated as:

```text
STOP_INFRASTRUCTURE_UNSTABLE
```

before any valid identification fixture.

Therefore Q6CB establishes none of the following:

- boundary reproduced;
- boundary not reproduced;
- H-RTCI supported;
- H-PDI supported;
- H-DSA supported;
- H-SEM supported;
- unresolved scientific mechanism.

Its causal result is simply:

```text
NONE
```

The Q6CB termination must not be converted into either positive or negative mechanism evidence.

## 5. Why RETURN_TO_END_TO_END_VALIDATION is not admissible

The finite governance requires a supported causal result before the one permitted successor intervention can open.

The path to E2E was:

```text
supported causal result
        ↓
one concrete SI-1 intervention
        ↓
one valid successor correctness attempt
        ↓
correctness PASS
        ↓
one frozen component-value program
        ↓
practical value
        ↓
RETURN_TO_END_TO_END_VALIDATION
```

The current program has not passed the first step.

There is:

```text
no supported Q6CB causal result
no authorized SI-1
no successor correctness PASS
no successor component-value result
```

Promoting Q4 alone would also violate the preregistered SA1 scope. Q4 and Q6 were intentionally included together to prevent favorable-type selection after measurement.

Therefore a Q4-only model integration is not a scientifically valid shortcut to E2E.

## 6. Program-level decision

```text
LEGACY ARCLLM LINE
FEASIBLE_NO_DEMONSTRATED_ADVANTAGE
        ↓
CLOSED

SA-H1 SUCCESSOR
        ↓
SA0 capability PASS
        ↓
SA1 Q4 component PASS
SA1 Q6 correctness FAIL
        ↓
Q6CB causal study
STOP_INFRASTRUCTURE_UNSTABLE
NO CAUSAL RESULT
        ↓
successor intervention gate NOT SATISFIED
        ↓
RETURN_TO_E2E NOT ADMISSIBLE
        ↓
STOP_CURRENT_SUCCESSOR_LINE
```

## 7. What is preserved

Stopping the successor line does **not** erase valid evidence.

The following remain closed positive findings:

- original ArcLLM can execute the exact model end-to-end;
- exact Arc 140V Vulkan capabilities were established;
- subgroup-32 split-K materially improved the frozen Q4 component family.

What is not supported is promotion of those findings into a complete Q4/Q6 successor architecture or a real-model advantage claim.

## 8. Closure boundary

The following are closed:

- further Q6CB work;
- a renamed Q6CB rescue;
- Q6 rescue inside SA1;
- SA1 target-model integration;
- Q3 reopening;
- SA-H1 E2E validation;
- SI-1 derived from Q6CB.

No additional execution is authorized on the current successor line.

A future architecture idea, if ever considered, would require a genuinely new program-level justification and governance boundary; it cannot be represented as continuation or rescue of SA-H1/Q6CB.

## Final state

```text
LEGACY LINE             CLOSED — FEASIBLE_NO_DEMONSTRATED_ADVANTAGE
SA-H1 SUCCESSOR LINE    CLOSED — NO END-TO-END ADMISSION
Q6CB                     CLOSED — STOP_INFRASTRUCTURE_UNSTABLE
CURRENT E2E AUTH         FALSE
CURRENT NEXT EXPERIMENT  NONE
```
