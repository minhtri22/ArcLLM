# Q6CB-1 Static-Equivalent QA

**Result:** `PASS_STATIC_EQUIVALENT_QA`  
**Scientific execution:** CLOSED

## Evidence chain

The final implementation qualification uses two exact provenance points:

- target-local BuildOnly at `6ba51a9d968cd0ed46f808b2f241cd8a50b7e118`;
- repaired static/unit QA PASS at `627bcc52759b25649b49fd494ba6c802d7bf3d50`.

The BuildOnly run compiled both frozen diagnostic shaders and linked the native causal harness. It explicitly recorded:

- `executable_launched=false`
- `scientific_execution=false`
- `gpu_dispatch=false`
- `performance_timing=false`
- `model_loaded=false`

The static test initially produced a false negative because it searched for the unescaped text form of a C++ JSON string literal. Commit `54d1f9d4f0dfc0994e4d072366a1101c3a5ef753` changed only that test assertion.

A commit comparison from the BuildOnly head to the static-PASS head shows changes only in the static test, manifest and lineage. The harness, generator, canonical reference, both GPU shaders and both BuildOnly tools are byte-identical by Git blob. Therefore no third build is required.

## Exact build artifacts

- packed SPIR-V SHA-256: `CE326D6CFB6474D3B255D0530FE3BE789BACC8B1540A709223EF7F06E1C49B61`
- expanded SPIR-V SHA-256: `73F35157417A2BB28E0BBB97D1F9CFE4AD7C54C0DFED599DEA0E4EE53F5AA6D2`
- native executable SHA-256: `83E2A04C83418258EB2BB77E4350D3661766EFDBB140E754AE704805D62A1AA9`

## Scientific adequacy before execution

The frozen implementation now contains:

1. R64 canonical binary64 reference;
2. S32 serial binary32 arm;
3. deterministic 32-lane CPU topology proxy;
4. GPU packed-Q6 split-32 arm;
5. GPU expanded-FP32 split-32 control;
6. an independent generated-intent versus packed-reconstruction semantic invariant for q/scale/d.

The semantic invariant allows H-SEM to be distinguished from a packed-access interaction rather than conflating the two.

No threshold, scientific fixture partition, identification seed, confirmatory seed or outcome rule has yet been selected from fresh observations.

## Decision

Q6CB-1 implementation is statically qualified. Implementation mutation must now close.

The next mandatory action is the final implementation/evidence lock followed by the governance goal-alignment check. Neither action authorizes fresh scientific execution.


## Mandatory goal-alignment check

This check was performed only after the implementation/evidence lock was frozen.

```text
ORIGINAL_GOAL:
Identify, within one finite preregistered causal program, whether the Q6_K
correctness boundary has a reproducible causal mechanism among the frozen
competing mechanism families.

CURRENT_QUESTION:
Can a frozen execution contract use the already locked causal harness to
distinguish the declared Q6CB terminal outcomes without changing hypotheses,
implementation, fixtures post hoc, or thresholds after data?

DIRECTLY_SERVES_ORIGINAL_GOAL                         = true
REQUIRED_TO_DISTINGUISH_TERMINAL_DECISIONS           = true
WITHIN_HARD_RESEARCH_BUDGET                           = true
PRESERVES_PRIOR_EVIDENCE                              = true
JUSTIFIED_EVEN_IF_NEGATIVE                            = true
NEGATIVE_OUTCOME_ACCEPTED_AS_FINAL_EVIDENCE           = true
DRIFT_DETECTED                                        = false
```

**Decision:** `CONTINUE_WITHIN_LOCK_TO_EXECUTION_CONTRACT_DESIGN_ONLY`

This decision permits only specification/freeze of the execution contract. It does not authorize fresh fixture execution, CPU scientific execution, GPU dispatch, performance timing or target-model loading.
