# ArcLLM Product and Research Governance

## Canonical roles

- `main` is the only canonical product/runtime integration line and the canonical convergence point for accepted scientific knowledge.
- `research/<single-question>` branches are temporary scientific workspaces. A research branch exists to answer one frozen question; it is not a second product main.
- `programs/arcllm_v1/lineage.md` is append-only and records scientific questions, evidence-bearing PASS/FAIL/UNRESOLVED results, quantitative scientifically meaningful findings, claim boundaries, and scientific closure/convergence only.
- Build, CI, runner repair, dependency, transport, package, static-QA, process-crash, implementation-lock, and other non-scientific details do not belong in scientific lineage.
- Immutable historical checkpoints are preserved with commit SHAs and, when a stable human name is useful, `archive/*` tags.

## Research lifecycle

A new study starts from the current `main` and freezes exactly one scientific question before fresh outcome-bearing execution.

```text
main
  -> research/<single-question>
  -> preregistration / exact freeze
  -> bounded execution
  -> independent adjudication when required
  -> PASS / FAIL / UNRESOLVED
  -> formal close
  -> convergence decision
  -> main
```

Do not open another ArcLLM scientific branch while the current active ArcLLM study has not been formally adjudicated and closed/converged, unless an explicit governance decision declares the work scientifically independent.

## Convergence rules

### PASS

If a PASS establishes a mechanism that is selected for product use:
- append the scientific result and claim boundary to lineage;
- converge only the product/runtime code and configuration required by the accepted mechanism;
- rerun canonical build, regression, and handoff checks on the resulting `main`.

A PASS does not authorize unrelated experimental harnesses to enter `main`.

### FAIL

For a valid negative result:
- append the result and boundary to lineage;
- do not merge the failed experimental mechanism into the canonical runtime;
- preserve exact evidence by commit/artifact provenance;
- close the study branch.

FAIL is retained as first-class scientific knowledge.

### UNRESOLVED

For a valid unresolved result:
- append what was established and why the scientific question remains unresolved;
- do not tune the same study post hoc to rescue a verdict;
- do not promote experimental code into the canonical runtime;
- any future attempt must open a new prospective study.

### No scientific result

A branch closed before outcome-bearing science (for example duplicate scope or implementation-only qualification) does not create a scientific lineage entry. Its closure may be preserved in technical/governance artifacts, but it must not be promoted into PASS/FAIL scientific evidence.

## Branch closure and archival policy

After convergence:
1. verify the scientific result is represented in the canonical lineage when applicable;
2. verify any accepted runtime change is present on `main`;
3. verify rejected/experimental-only runtime code is absent from the canonical product path;
4. preserve an immutable commit/tag when needed for durable historical reference;
5. delete the temporary research branch when it no longer serves an active scientific purpose.

Branch deletion does not erase science: commit hashes, tags, artifacts, and canonical lineage are the durable record.

## Product handoff gate

Before declaring a `main` revision ready for handoff:
- static/runtime-boundary QA must pass;
- shaders and native runtime must build from a clean checkout;
- the exact validated model must resolve;
- frozen canonical regression profiles must execute on the target machine;
- at least one non-fixture request must execute through the declared safe product boundary;
- lifecycle counters/routes must match the active runtime contract;
- unsupported capabilities must remain explicitly unsupported rather than silently inferred;
- `checklist.md` must record the tested commit, evidence, PASS/FAIL state, and open finding count.

The handoff gate is functional/correctness evidence. It does not create a new performance claim unless a separate preregistered performance study authorizes one.

## Current product boundary

The active ArcLLM runtime is bound by `config/arcllm_v1_runtime_active_v0.2.json` and currently exposes:

- public C++ entrypoint `arcllm::v1::runtime::generate(const RunRequest&)`;
- caller-supplied token IDs and generation length;
- greedy generation;
- Intel Arc/Vulkan execution;
- I002 Gate/Up within the validated domain;
- generic policy/binding v4;
- Q4VulkanBackendV4 FFN-down;
- safe fallback behavior outside the validated domain.

Current explicit non-capabilities include persistent model sessions, canonical NPU execution, arbitrary prompt-quality validation, and a current external llama.cpp performance advantage claim.
