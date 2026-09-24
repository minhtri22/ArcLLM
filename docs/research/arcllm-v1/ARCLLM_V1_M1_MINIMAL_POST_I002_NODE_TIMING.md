# ArcLLM v1 — M1 Minimal Post-I002 Node Timing

Purpose: fill current post-I002 Arc node/family timing in the central hardware ledger before M2 maps pinned llama on the same semantic path.

Frozen discovery design:
- exact target model and post-I002 469-dispatch decode graph;
- gate/up exactly `sa1_q4k_subgroup_splitk.spv`; non-target decode nodes unchanged;
- workloads W-S and W-C;
- one warmup + two measured attempts per workload;
- timestamp only decode indices 0, 15, 30;
- 4 measured inferences, 12 profiled token steps, 5,628 op timestamp values;
- no A/B sessions, no repeated machine profile, no quiet-host requirement, no hardware counters.

Primary evidence: raw Vulkan query ticks and tick shares. `wall_attributed_ms_proxy = submit_wait_ms * tick_share` provides a practical absolute attribution but is explicitly not timestampPeriod-calibrated pure GPU time.

M1 is localization/discovery only. No new kernel is authorized by M1 alone. After M1, patch the central hardware model and open M2 pinned-llama same-node execution mapping.
