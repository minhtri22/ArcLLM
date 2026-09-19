# P7-M Implementation

P7-M reuses the proven timestamp-query profiler from P7-H and changes only the prefill graph to the frozen P7-L winner.

Expected dispatch counts:
- P7-L prefill: 441.
- unchanged decode: 469.

The profiler categorizes the fused operation as ffn_gate_up, preserving comparability with prior P7-H attribution while the dispatch count changes from 56 separate gate/up GEMMs to 28 fused dispatches.

There is no A/B timing gate in this phase.
