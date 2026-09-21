# Q6CB-1 Zero-Science Specification QA

**Date:** 2026-09-21  
**Result:** `PASS_ZERO_SCIENCE_SPECIFICATION_QA`  
**Classification:** `SPECIFICATION_VALID_EXECUTION_CLOSED`

## Audited state

Base SA1-closeout commit:

`3352dbc841a06f1cab8e70f9d8353d683c411ccc`

Audited Q6CB specification HEAD:

`af36dbc5382d3d0b1ec009f1562230ffb68456a3`

The branch delta contains exactly four changed files:

- `config/q6cb1_specification_v0.1.json`
- `docs/Q6CB1_Q6_CORRECTNESS_BOUNDARY_PROGRAM.md`
- `lineage.md`
- `manifest.json`

No shader, C/C++ source, PowerShell runner, scientific harness, benchmark runner or execution-authorization file was changed or created.

## SA1 immutability check

The SA1 Q6 candidate shader remains Git blob:

`0fdc0c8f195872396a653b38ee2283156fbaeaa0`

The SA1 closeout artifact remains blob:

`83b30549410ce89e8d5047685c8bcbddc87983ee`

The SA1-K2 Q6 adjudication remains blob:

`a2419af5a98ec931424bda46de840d3495e0bc38`

All three are identical at the SA1-closeout base and the audited Q6CB specification HEAD.

## Scientific design QA

The specification passes because it does not assume the post-SA1 causal explanation.

It preregisters competing mechanism families:

- reduction-topology × conditioning interaction;
- packed-dequant access interaction;
- device/subgroup arithmetic contribution;
- hidden semantic/reference defect;
- no stable fresh boundary.

It defines a causal arm set capable of distinguishing topology, packed-path, device and semantic explanations, while retaining a high-precision reference and serial FP32 reference.

It also permits scientifically negative outcomes: `BOUNDARY_NOT_REPRODUCED`, `MULTIFACTOR`, and `UNRESOLVED_MECHANISM`. The program therefore does not require a mechanistic winner.

## Fresh-data QA

The specification excludes the two exact SA1 Q6 cells, SA1 banks 0 and 3, SA1 exact fixtures/seeds/outputs and post-hoc failure mining from fresh Q6CB primary/confirmatory evidence.

Both identification and confirmatory partitions must be frozen before the first scientific execution. Numeric tolerances, fixture counts, seed derivation, classification rules and evidence schema must also be frozen before execution.

## Governance decision

Q6CB-0 specification is complete and passes zero-science QA.

This PASS does **not** authorize execution and does not authorize implementation automatically.

Current permissions remain:

```text
causal_harness_implementation = false
fresh_fixture_execution       = false
scientific_CPU_execution      = false
scientific_GPU_execution      = false
performance_timing            = false
target_model_load             = false
SA1_rerun                     = false
```

## Next gate

The only next valid action is an **explicit authorization gate for Q6CB-1 causal-harness implementation lock only**.

That next stage may create the five-arm correctness-only harness, canonical independent decoder/reference, deterministic fixture generator, static/unit tests and BuildOnly tooling. It still may not execute fresh scientific fixtures or dispatch the target GPU until a later, separately frozen execution contract is adjudicated and explicitly authorized.
