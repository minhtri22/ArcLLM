# TTFT_M2 — P9 Explicit Fresh Mechanism-Identification Authorization Gate

**Date:** 2026-09-22  
**Decision:** `DENY / LEAVE M2_P9 BLOCKED`

## Gate result

`M2_P9_FRESH_MECHANISM_IDENTIFICATION_NOT_AUTHORIZED`

Reason:

`MISSING_FRESH_M2_SCIENTIFIC_DESIGN_ADOPTION_AND_REVALIDATION`

## What passed

The infrastructure prerequisite is now satisfied.

P8L result:

`PASS_M2_P8L_LOCAL_BUILDONLY_EXECUTION_QUALIFICATION`

P8L adjudication blob:

`b1eda822a3580370ebdeda704af8a169c4991dec`

The exact frozen BuildOnly package executed successfully on the local Windows substrate. No frozen-package defect was observed.

Therefore infrastructure is **not** the reason for this denial.

## Why P9 cannot be authorized yet

M2-P2 explicitly classified the M1 scientific assets as methodological templates, not M2 canonical science.

Method-transfer register:

`config/arcllm_ttft_m2_method_transfer_register_v0.1.json`

blob:

`825d5b5a6d67fc92f66c667582a12b55de7737c7`

P2 dispositions include:

```text
finite hypotheses
  TEMPLATE_ONLY_REQUIRES_FRESH_M2_ADOPTION_GATE

2x2 causal design
  CONDITIONALLY_ADMISSIBLE_AS_DESIGN_TEMPLATE_ONLY

falsification/STOP contract
  ADMISSIBLE_AS_GOVERNANCE_TEMPLATE_ONLY
```

P2 also states that these assets are not automatically canonical in M2 and that before any M2 scientific execution M2 must independently revalidate:

- source / measurement boundaries;
- factor manipulability;
- exact target invariants;
- workload relevance;
- endpoint semantics;
- session/order/repetition rules;
- falsification logic.

That prospective M2 adoption/revalidation has not yet been performed.

## Historical templates

The exact historical M1 template blobs remain admissible as prior methodological evidence only:

```text
hypotheses
559cf3a60789074045ac0c3f30c4982b99800707

causal design
66b6ca2bf1da1e05ddc416bf8f9d73074e8f3fdb

falsification / STOP contract
fb856b27385278878114316ebd667b97fb38ba59
```

They are **not** authorized as M2 canonical execution contracts.

## Gate boundary

P8L PASS removes the infrastructure blocker.

It does not automatically activate scientific design.

Authorizing P9 now would silently promote M1 templates into M2 canonical science and would violate the explicit P2 transfer rule.

Therefore this gate must fail closed.

## Authorization state

Still forbidden:

```text
target model execution          false
diagnostic executable launch    false
model load                      false
GPU dispatch                    false
performance measurement         false
fresh TTFT observation          false
mechanism adjudication          false
```

No scientific observation has been collected.

## Next admissible step

`M2_P9A_FRESH_SCIENTIFIC_DESIGN_ADOPTION_AND_REVALIDATION`

P9A is a pre-science specification/adoption step.

It must produce fresh M2 canonical artifacts for:

1. hypotheses;
2. causal decomposition / factor design;
3. target/environment invariants;
4. workloads;
5. endpoint semantics;
6. session/order/repetition rules;
7. falsification and STOP rules.

It may adopt historical M1 design elements only after independent M2 revalidation and exact new M2 bindings.

Only after P9A PASS may this explicit P9 authorization gate be repeated.

No model/GPU/timing science is permitted in P9A.
