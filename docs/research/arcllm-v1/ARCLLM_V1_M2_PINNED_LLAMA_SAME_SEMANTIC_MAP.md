# ArcLLM v1 — M2 Pinned llama Same-Semantic Vulkan Map

Goal: map pinned llama.cpp v0.4.1 / b29c606e on the same semantic decode families revealed by the Arc hardware ledger.

No llama source modification is required. The exact pinned Vulkan backend already contains `GGML_VK_PERF_LOGGER`, uses Vulkan timestamp queries and converts ticks with the device `timestampPeriod`.

Runner keeps the exact I003 baseline adapter/config/model and enables only `GGML_VK_PERF_LOGGER=1`. Warmup and measured generation semantics remain unchanged.

Semantic anchors at decode n=1:
- gate/up Q4_K: m=18944,k=3584,count=56
- down Q4_K: m=3584,k=18944,count=14
- down Q6_K: m=3584,k=18944,count=14
- lm-head Q6_K: m=152064,k=3584,count=1
- Q+O combined Q4_K: m=3584,k=3584,count=56
- K+V(Q4) combined Q4_K: m=512,k=3584,count=42
- V(Q6) Q6_K: m=512,k=3584,count=14.

For each workload, the parser takes the final 31 decode timing blocks belonging to the measured attempt and selects decode indices 0,15,30. Output is calibrated llama Vulkan microseconds and timing shares.

M2 is discovery only. After PASS, combine M1 Arc shares + M2 llama shares with the already-closed I003 practical token latencies to construct the cross-runtime EXCESS-COST MAP. No kernel intervention is authorized before that map.
