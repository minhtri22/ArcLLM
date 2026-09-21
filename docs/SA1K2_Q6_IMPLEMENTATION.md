# SA1-K2 Q6 Implementation

**Date:** 2026-09-21  
**Parent Q6 implementation lock:** `2a7a7a2ecec38aac0853f112e7bea01bbad7514a`

Exactly one new Q6 candidate shader is implemented:

`shaders/sa1_q6k_subgroup_splitk.comp`

It preserves the Q4-established mechanism and geometry: 128-thread workgroup, required subgroup size 32 at pipeline creation, four subgroups/workgroup, one subgroup/output row, K stride 32, direct packed Q6_K decode, FP32 partial accumulation, subgroup arithmetic reduction and lane-0 store.

The packed Q6_K layout is inherited exactly from the frozen baseline: 128 ql bytes + 64 qh bytes + 16 signed scale bytes + FP16 d = 210 bytes/block.

The component harness now accepts `--quant q6` while defaulting to the already-closed Q4 path. Q6 fixture generation uses the frozen SplitMix64 banks and creates direct valid Q6_K blocks with d=0.03125. CPU reference decoding mirrors the frozen baseline byte layout.

Only the two preregistered Q6 cells are active in Q6 mode. Q6 preflight executes banks 0 and 3 only, creates no timestamp query and emits zero measured pairs. Q6 measured mode is present only behind a future `SA1_Q6_EXECUTION_AUTHORIZED` file, which does not exist at this stage.

Build artifacts are isolated under `artifacts/SA1_K2/build`; Q4 artifacts/results are not overwritten.

Q6 timing remains forbidden.
