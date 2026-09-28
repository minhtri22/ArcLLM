from pathlib import Path

root=Path(__file__).resolve().parents[1]
api=(root/"include/arcllm/v1/runtime.h").read_text(encoding="utf-8")
rt=(root/"src/arcllm_v1_runtime.cpp").read_text(encoding="utf-8")
backend=(root/"src/arcllm_v1_q4_vulkan_backend_v4_runtime.h").read_text(encoding="utf-8")
support=(root/"src/arcllm_v1_vulkan_runtime_support.h").read_text(encoding="utf-8")
cli=(root/"src/arcllm_v1_runtime_cli.cpp").read_text(encoding="utf-8")
oracle=(root/"src/arcllm_v1_full_runtime_q4_v4.cpp").read_text(encoding="utf-8")
historical_backend=(root/"src/arcllm_v1_q4_vulkan_backend_v4.cpp").read_text(encoding="utf-8")

for token in [
    "RunRequest","RunResult","RuntimeStats","generate(const RunRequest& request)",
    "input_token_ids","max_new_tokens","request_within_validated_domain"
]:
    assert token in api or token in rt, token

for forbidden in [
    '"W-S"','"W-C"',
    "f31d4bb9fe5eb9c3","471519ddc45b232e",
    "PASS_FULL_ARCLLM_RUNTIME_Q4_V4_INTEGRATION",
    "NO_PERFORMANCE_ADJUDICATION",
    "implementation_commit",
    "expected_frozen_i002_hash",
    "p8c_segmented_access_correctness.cpp",
    "arcllm_v1_q4_vulkan_backend_v4.cpp",
    "P8GLayer","P8G","Q1StepObs","Q1ExecutionObs",
]:
    assert forbidden not in rt, forbidden
    assert forbidden not in cli, forbidden

for forbidden in [
    "p8c_segmented_access_correctness.cpp",
    "int main(",
    "component_oracle(",
    "execute_component(",
    "PASS_",
]:
    assert forbidden not in backend, forbidden

for forbidden in [
    "P7A","P8-G","Q2","int main(","compare_vec(","attention_cpu(","swiglu_cpu("
]:
    assert forbidden not in support, forbidden

assert '#include "arcllm_v1_q4_vulkan_backend_v4_runtime.h"' in rt
assert '#include "arcllm_v1_vulkan_runtime_support.h"' in backend
assert "sa1_q4k_subgroup_splitk.spv" in rt
assert "request.request_within_validated_domain" in rt
assert '"p7_q4k_gemm_2d.spv"' in rt
assert "gp::evaluate" in rt
assert "bind::apply_decision" in rt
assert "append_ffn_down_op" in rt
assert "request.max_new_tokens" in rt

# Historical integration/validation harnesses remain unchanged as external regression oracles.
for token in ['"W-S"','"W-C"',"f31d4bb9fe5eb9c3","471519ddc45b232e","PASS_FULL_ARCLLM_RUNTIME_Q4_V4_INTEGRATION"]:
    assert token in oracle, token
assert "int main(" in historical_backend
assert "component_oracle(" in historical_backend

print("ARCLLM_V1_CANONICAL_RUNTIME_EXTRACTION_STATIC_QA=PASS")
