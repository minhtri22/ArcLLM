# Q6CB — Termination & Goal-Alignment Governance

**Status:** BINDING BEFORE Q6CB-1 IMPLEMENTATION  
**Date:** 2026-09-21  
**Scope:** Q6 correctness-boundary research line and at most one causal successor intervention  
**Parent program:** Q6CB-1  
**SA1 status:** CLOSED and immutable

## 1. Purpose

This governance prevents the Q6 correctness-boundary work from becoming an open-ended research chain.

A negative result is a valid scientific result. Failure, falsification, non-reproduction, lack of actionability, and lack of practical value are all admissible terminal outcomes.

The program is not required to find a successful kernel, a mechanism, or an advantage.

## 2. Original goals that must remain fixed

### Local goal — Q6CB

Identify, within a finite preregistered causal program, whether the Q6_K correctness boundary has a reproducible causal mechanism among the frozen competing mechanism families.

Q6CB is complete when this question is adjudicated. It is not an optimization program.

### Global ArcLLM goal

ArcLLM ultimately seeks a useful, reproducible execution regime on the target hardware relative to a matched baseline.

Q6CB is justified only because the Q6 correctness boundary blocks deciding whether the successful Q4 component mechanism has an actionable successor path.

Q6CB must not become a substitute for real-model end-to-end validation.

## 3. Mandatory goal-alignment test before every new stage

A proposed action is permitted only if all answers below are YES:

1. Does it directly reduce uncertainty required to adjudicate the frozen Q6CB causal question, or implement the one permitted causal intervention after Q6CB closes?
2. Is the information necessary to decide one of the already-declared terminal outcomes?
3. Is the action within the finite research budget in this document?
4. Does it preserve SA1 and prior Q6CB evidence immutably?
5. Would the action still be scientifically justified if its outcome were negative?

If any answer is NO:

`STOP_DRIFT`

No new experiment may be opened merely because the observation is interesting.

## 4. Hard research budget

The entire line is bounded as follows:

```text
mechanism-identification programs          = 1  (Q6CB)
fresh identification collections           = 1
fresh confirmatory collections             <= 1
successor causal interventions              <= 1  (SI-1)
successor implementation candidates         = 1
valid successor correctness attempts        = 1
successor component performance programs    <= 1
additional mechanism programs               = 0
SI-2 or rescue interventions                = 0
kernel / tile / subgroup searches           = 0
adaptive threshold searches                 = 0
target-model optimization studies in line   = 0
```

A valid scientific FAIL consumes its stage. It cannot be reset by renaming the experiment.

## 5. Q6CB terminal point

Q6CB ends at `Q6CB-4 FINAL ADJUDICATION`.

There is no Q6CB-5.

Allowed terminal Q6CB results:

- `BOUNDARY_NOT_REPRODUCED`
- `H_RTCI_SUPPORTED`
- `H_PDI_SUPPORTED`
- `H_DSA_SUPPORTED`
- `H_SEM_SUPPORTED`
- `MULTIFACTOR`
- `UNRESOLVED_MECHANISM`

`INVALID_F0` is not a scientific outcome and is governed separately by the bounded F0 rule below.

## 6. Scientific value of negative terminal outcomes

Negative outcomes must be retained as knowledge rather than treated as failures to be rescued.

### BOUNDARY_NOT_REPRODUCED

Scientific value:

The SA1 correctness boundary does not generalize to the independently frozen Q6CB fresh regime under the preregistered reproduction rule.

Action:

Close Q6CB. Do not search for harsher fixtures.

### UNRESOLVED_MECHANISM

Scientific value:

The fresh boundary exists, but the preregistered mechanism family and contrasts are insufficient to identify its cause.

Action:

Close Q6CB. Do not open a second mechanism program.

### Hypothesis falsification

Scientific value:

The falsified mechanism family is excluded as a sufficient explanation within the frozen study scope.

Action:

Retain the falsification. Do not modify thresholds or fixtures to restore the hypothesis.

### MULTIFACTOR without one justified intervention

Scientific value:

The boundary appears causally composite and does not support one bounded intervention with adequate identification.

Action:

Close the research line.

### SI-1 correctness FAIL

Scientific value:

Even with a supported causal explanation, the one preregistered intervention is insufficient to establish an actionable correctness-preserving successor.

Action:

Hard-stop the Q6 successor line.

### SI-1 performance NO-VALUE

Scientific value:

The intervention may restore correctness but does not provide the preregistered practical component value needed to justify further architecture investment.

Action:

Hard-stop the Q6 successor line.

### End-to-end return produces no regime advantage

Scientific value:

The successor is technically viable but does not establish a useful ArcLLM regime advantage under matched end-to-end evidence.

Action:

Accept the ArcLLM negative result. Do not reopen component optimization to search for a win.

## 7. Successor intervention gate after Q6CB-4

A successor intervention may open only if Q6CB-4 supports a causal mechanism strongly enough to justify **one concrete intervention derived before seeing successor outcomes**.

If no single intervention follows from the adjudicated evidence:

`STOP_NO_ACTIONABLE_INTERVENTION`

If a single intervention is justified, open exactly one successor program:

`SI-1`

SI-1 must be separately preregistered and may not change Q6CB or SA1 history.

## 8. SI-1 finite path

```text
Q6CB-4 causal result
        ↓
one causal intervention identifiable?
   NO → STOP_NO_ACTIONABLE_INTERVENTION
   YES
        ↓
SI-1 specification + QA
        ↓
exactly one implementation candidate
        ↓
one valid frozen correctness attempt
        ↓
FAIL → STOP_INTERVENTION_CORRECTNESS
PASS
        ↓
at most one frozen component-value program
        ↓
NO VALUE → STOP_INTERVENTION_NO_PRACTICAL_VALUE
VALUE
        ↓
STOP MICRO-RESEARCH
RETURN TO REAL-MODEL END-TO-END VALIDATION
```

There is no SI-2.

A successful SI-1 is not permission for more component optimization.

## 9. Drift triggers

Any of the following immediately triggers `STOP_DRIFT` unless it is required to resolve a genuine F0 invalidation:

- opening another mechanism family after Q6CB-4;
- creating Q6CB-5 or later;
- adding experiments only to understand an interesting side observation;
- searching for fixtures that recreate or strengthen the failure;
- changing thresholds after outcomes;
- changing subgroup size, tile size, workgroup geometry or kernel family to obtain PASS;
- adding Q4/Q5/Q8 or another quant format to broaden the study;
- turning Q6CB into a performance optimization program;
- loading a target model before the permitted return-to-end-to-end point;
- using target-model behavior to rescue Q6CB;
- reopening SA1;
- repeating a valid scientific attempt;
- creating SI-2;
- opening another microbenchmark after SI-1 has shown practical value instead of returning to end-to-end validation;
- pursuing a question whose answer cannot alter a currently permitted terminal decision.

## 10. Bounded F0 rule

F0 means genuine infrastructure, provenance, or measurement invalidation; it is not a scientific negative.

For each execution stage:

- at most one bounded tooling/measurement repair is allowed after a proven F0;
- the repair must not change the scientific hypothesis, fixtures, thresholds, causal contrast, or intended implementation;
- the invalid attempt does not count as the one valid scientific attempt;
- if the repaired stage produces another F0 that prevents valid adjudication:

`STOP_INFRASTRUCTURE_UNSTABLE`

This prevents infinite tooling repair chains.

## 11. Mandatory stop check after every adjudication

After each Q6CB or SI-1 adjudication, record:

```text
ORIGINAL_GOAL:
CURRENT_QUESTION:
DOES_NEXT_STEP_DIRECTLY_SERVE_ORIGINAL_GOAL: true/false
IS_NEXT_STEP_REQUIRED_TO_DISTINGUISH_TERMINAL_DECISIONS: true/false
BUDGET_REMAINING:
NEGATIVE_OUTCOME_ACCEPTED_AS_FINAL_EVIDENCE: true/false
DECISION:
  CONTINUE_WITHIN_LOCK
  or STOP_<REASON>
```

No next stage may open without this record.

## 12. Final terminal states for the entire line

The line must end in one of:

- `STOP_BOUNDARY_NOT_REPRODUCED`
- `STOP_UNRESOLVED_MECHANISM`
- `STOP_MULTIFACTOR_NO_ACTIONABLE_INTERVENTION`
- `STOP_NO_ACTIONABLE_INTERVENTION`
- `STOP_INTERVENTION_CORRECTNESS`
- `STOP_INTERVENTION_NO_PRACTICAL_VALUE`
- `STOP_INFRASTRUCTURE_UNSTABLE`
- `STOP_DRIFT`
- `RETURN_TO_END_TO_END_VALIDATION`

There is no generic `CONTINUE_RESEARCH` terminal state.

## 13. Success criterion for the research line

The research line is successful when it produces a trustworthy decision, not when it produces a positive outcome.

Therefore:

```text
valid negative adjudication
    == scientific success of the study process

positive causal result
    != obligation to continue

successful intervention
    == reason to stop micro-research and return to end-to-end
```

The default after a terminal adjudication is STOP, not "open the next interesting experiment".
