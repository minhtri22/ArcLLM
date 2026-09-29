# CORE-0D — Zero-Science Preflight Final Adjudication

Date: 2026-09-29

## Verdict

**PASS_CORE0D_ZERO_SCIENCE_PREFLIGHT**

Open findings: **0**.

No model inference and no measured CORE-0D request was executed during this gate.

## Qualified implementation

The implementation is benchmark-only. Canonical ArcLLM runtime, public API, Q4V4 backend and shader sources remain unchanged.

Built artifacts:

- ArcLLM CONTROL/PHASE derivative: `72FC72A5...D8ABD2`
- ArcLLM GPU_TRACE derivative: `7DF37ED8...8F0AFE`
- llama.cpp teacher-forced adapter: `A9BA63AB...29204C`

Exact external pins remain:

- llama.cpp `v0.4.1@b29c606e28a01b1bc8c1351026a0fa6e616bf6c4`
- Token-XRay `35f86ac68f98ffe60fc441a790274cd1f1269dfe`
- model SHA256 `60E05F...2463`

## Zero-science qualification

PASS:

- deterministic runtime rematerialization;
- both 31-token common teacher-forced trajectories;
- 36-request schedule construction;
- ArcLLM control/phase and trace binary build;
- llama teacher-forced adapter build;
- binary self-tests;
- llama Vulkan perf parser synthetic 32-block test;
- exact phase-boundary presence;
- canonical runtime/shader diff guard;
- measured runner fail-closed without authorization;
- independent adjudication.

## Frozen phase surface

ArcLLM PHASE_ONLY records exactly:

1. `model_map_inspect_ns`
2. `static_graph_contract_ns`
3. `vulkan_weight_init_ns`
4. `request_context_prepare_ns`
5. `prefill_and_scan_ns`
6. `decode_loop_and_scans_ns`
7. `close_and_cleanup_ns`
8. `runtime_generate_ns`

llama uses its pinned `llama_perf_context_data` fields as secondary evidence only.

## Frozen GPU trace surface

ArcLLM reuses the qualified CORE-0C Token-XRay timestamp contract.

llama uses its built-in Vulkan perf logger with concurrent mode disabled and frequency 1. Exactly 32 timing blocks are required: one prefill plus 31 cached-decode graphs.

Both trace surfaces remain **DIAGNOSTIC_ONLY**.

## Consequence

The exact 36-request collection may now be authorized. Passing this implementation gate does not select GPU vs outside-GPU, does not choose a kernel, and does not authorize NPU or any product optimization.

This zero-science PASS is not appended to the scientific lineage.
