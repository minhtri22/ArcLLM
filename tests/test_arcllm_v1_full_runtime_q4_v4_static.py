from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
src=(root/"src/arcllm_v1_full_runtime_q4_v4.cpp").read_text(encoding="utf-8")
backend=(root/"src/arcllm_v1_q4_vulkan_backend_v4.cpp").read_text(encoding="utf-8")
freeze=(root/"config/arcllm_v1_phase2_generic_extension_surface_freeze_v4.0.json").read_text(encoding="utf-8")

required=[
    '#include "arcllm_v1_q4_vulkan_backend_v4.cpp"',
    "policy_v4",
    "apply_decision",
    "q4_decide",
    "append_ffn_down_op",
    "kProfile0",
    "kProfile1",
    "future_reuse_units=future_reuse",
    "route_B_steps==31u",
    "b_allocations==1u",
    "b_releases==1u",
    "f31d4bb9fe5eb9c3",
    "471519ddc45b232e",
    "PASS_FULL_ARCLLM_RUNTIME_Q4_V4_INTEGRATION",
]
for token in required:
    assert token in src, token

for forbidden in [
    "session==",
    "warm_base",
    "warm_cand",
    "run_attempt(",
    'candidate?"sa1_q4k_subgroup_splitk.spv"',
    'z.dw->ggml_type==Q4?"p7_q4k_gemm_2d.spv"',
]:
    assert forbidden not in src, forbidden

assert "append_ffn_down_op(" in backend
assert "FROZEN_PHASE2_GENERIC_EXTENSION_SURFACE_V4" in freeze
assert '"universality": false' in freeze

decode=src[src.index("auto build_decode="):src.index("auto ppops=build_prefill(seq);")]
assert decode.count("append_ffn_down_op(")==1
assert '"p7_q6k_gemm_2d.spv"' in decode
assert '"sa1_q4k_subgroup_splitk.spv"' in decode
assert "q4_down_exec148_serial.spv" not in decode, "B executor must stay encapsulated in backend"
print("FULL_ARCLLM_RUNTIME_Q4_V4_STATIC_QA=PASS")
