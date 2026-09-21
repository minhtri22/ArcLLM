# ARCLLM_TTFT_M1 — P4 Falsification and STOP Contract

P4 freezes the logic that can kill every attractive explanation.

No numerical effect threshold is invented here. Exact power, confidence/precision target and equivalence margin must be justified and frozen **after** this design passes zero-science QA and **before** execution.

Key STOP rules:

- if GPU timestamps or shared-footprint conditioning cannot make host-vs-GPU / executed-vs-prepared effects identifiable: `STOP_MECHANISM_NOT_IDENTIFIABLE`;
- if semantic/plan invariants drift: `STOP_SEMANTIC_OR_ARCHITECTURAL_DRIFT`;
- if mechanism-specific contrasts are equivalent and neutral process order explains the gap: H5 may be supported;
- if no mechanism is distinguishable at frozen precision: `MECHANISM_NOT_IDENTIFIED`;
- no generic TTFT optimization is allowed after `MECHANISM_NOT_IDENTIFIED`.

A supported mechanism does not automatically authorize an intervention.
