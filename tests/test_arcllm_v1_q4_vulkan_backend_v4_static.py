from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
src = (root / "src/arcllm_v1_q4_vulkan_backend_v4.cpp").read_text(encoding="utf-8")
contract = (root / "config/arcllm_v1_phase2_q4_vulkan_backend_v4_convergence_contract_v0.1.json").read_text(encoding="utf-8")

required = [
    "generic_policy_engine_v4.h",
    "generic_backend_binding_v4.h",
    "q4k_down_reference_registration_v2.h",
    "class Q4VulkanBackendV4",
    "binding_v4::BackendAdapter",
    "kExecFamilyBytes",
    "kAcquirePrimary",
    "kAcquireSecondary",
    "kAcquireTertiary",
    "A_WITHOUT_B_ALLOCATION=PASS",
    "P1_EXPLICIT_ACQUIRE_VALIDATE_ROUTE_B=PASS",
    "RESIDENT_B_REUSE_NO_REACQUIRE=PASS",
    "EVICT_B_THEN_A=PASS",
]
for token in required:
    assert token in src, token

for forbidden in [
    '#include "arcllm_v1_q4_down_arch_4arm.cpp"',
    '#include "arcllm_v1_q4_down_4arm_timing_runtime.cpp"',
    '#include "arcllm_v1_b1_2_zero_science.cpp"',
    '#include "arcllm_v1_b1_2_performance.cpp"',
    "Q4Arm::AB",
]:
    assert forbidden not in src, forbidden

acq = src.index("bind::BackendStatus acquire(")
val = src.index("bind::BackendStatus validate(", acq)
ctor = src.index("Q4VulkanBackendV4(")
assert "make_buffer(kExecFamilyBytes" not in src[ctor:acq]
assert "make_buffer(kExecFamilyBytes" in src[acq:val]

s1 = src.index("// S1:")
s3 = src.index("// S3:")
assert "b_allocations==0u" in src[s1:s3]
assert "b_materializations==0u" in src[s1:s3]

assert '"science_change_authorized": false' in contract
assert '"policy_change_authorized": false' in contract
assert '"representation_bytes": 549527552' in contract
assert "performance" in src and "timing_emitted" in src
print("Q4_VULKAN_BACKEND_V4_STATIC_QA=PASS")
