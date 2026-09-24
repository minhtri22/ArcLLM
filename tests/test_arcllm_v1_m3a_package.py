from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/"src/arcllm_v1_m3_vulkan_counter_capability.cpp").read_text(encoding="utf-8")
for x in ["VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME","vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR","vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR","performanceCounterQueryPools","CONCURRENTLY_IMPACTED"]:
    assert x in s,x
for bad in ["llama_decode","candidate_benchmark","run_attempt","GetQueryPoolResults"]:
    assert bad not in s,bad
r=(ROOT/"run_arcllm_v1_m3_capability_probe.ps1").read_text(encoding="utf-8")
for x in ["COUNTER_CAPABILITY","vtune","arcllm_v1_m3_counter_capability.exe","M3_A_RESULT=PASS","QUIET_HOST_REQUIRED=false"]:
    assert x in r,x
sm=(ROOT/"tools/summarize_arcllm_v1_m3_capability.py").read_text(encoding="utf-8")
for x in ["VULKAN_KHR_PERFORMANCE_QUERY","INTEL_VTUNE","INTEL_GPA","quiet_host_required_for_collection"]:
    assert x in sm,x
print("ArcLLM v1 M3-A static package PASS")
