import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
CFG=json.loads((ROOT/"config/arcllm_v1_b1_selection_lifetime_policy_v0.2.json").read_text(encoding="utf-8"))
POL=(ROOT/"src/arcllm_v1_b1_policy.h").read_text(encoding="utf-8")
HOOK=(ROOT/"src/arcllm_v1_b1_policy_runtime_hook.h").read_text(encoding="utf-8")
TEST=(ROOT/"tests/arcllm_v1_b1_policy_zero_science.cpp").read_text(encoding="utf-8")

def req(c,m):
    if not c: raise AssertionError(m)

req(CFG["status"]=="FROZEN_B1_SELECTION_LIFETIME_POLICY_V0_2","active policy status drift")
req(CFG["residency_lease"]["bytes"]==549527552,"resident bytes drift")
req(CFG["acquisition_paths"]["P1_GPU_CREATE_IN_PLACE"]["strict_create_threshold_tokens"]=={"W-S":2,"W-C":4},"P1 thresholds drift")
req(CFG["acquisition_paths"]["P3_COLD_UNBUFFERED"]["strict_create_threshold_tokens"]=={"W-S":15,"W-C":33},"P3 cold thresholds drift")
req(CFG["acquisition_paths"]["P0_CPU_DIRECT"]["strict_create_threshold_tokens"]=={"W-S":17,"W-C":37},"P0 thresholds drift")
req(CFG["acquisition_paths"]["P3_WARM_PAGE_CACHE"]["enabled"] is False,"P3 warm must remain disabled")
req(CFG["route_residency_separation"]["outside_domain_request_evicts_valid_B"] is False,"outside-domain eviction must remain disabled")

for token in [
    "kExec148ResidentBytes = 549527552ull",
    "return w == Workload::WS ? 2ull : 4ull",
    "return w == Workload::WS ? 15ull : 33ull",
    "return w == Workload::WS ? 17ull : 37ull",
    "Do not reapply the 2/4-token creation threshold here",
    "OUTSIDE_VALIDATED_DOMAIN, true",
]:
    req(token in POL,f"policy source contract missing: {token}")

for token in [
    "Resource manager decides residency_lease_granted",
    "Acquisition actions are single plans, not automatic retry chains",
    "require_post_acquisition_identity_validation",
    "preserve_existing_b",
]:
    req(token in HOOK,f"runtime hook contract missing: {token}")

for token in [
    "exhaustive_invariant_tests",
    "same_decision",
    "outside-domain request must use A without destroying valid B",
    "lease revocation must evict resident B",
    "resident B must ignore creation threshold",
    "B1_POLICY_ZERO_SCIENCE_QA=PASS",
]:
    req(token in TEST,f"zero-science QA coverage missing: {token}")

for forbidden in ["std::chrono","vkQueue","Vulkan","materialize_tensor","buffered_load(","unbuffered_load("]:
    req(forbidden not in POL and forbidden not in HOOK, f"science/performance surface leaked into policy core: {forbidden}")

print("B1_POLICY_STATIC_QA=PASS")
print("NO MODEL LOAD. NO GPU. NO VULKAN. NO PERFORMANCE TIMING. NO PLACEMENT BENCHMARK.")
