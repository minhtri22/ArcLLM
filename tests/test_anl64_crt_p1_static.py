from pathlib import Path

root=Path(__file__).resolve().parents[1]
src=(root/"src/anl64_crt_p1_runtime.cpp").read_text(encoding="utf-8")
spec=(root/"config/anl64_crt_p1_plan_vs_residual_executor_causal_separation_gate_v0.1.json").read_text(encoding="utf-8")

required=[
    '#include "anl64_plan.hpp"',
    "A_CANONICAL",
    "B_PLAN_SHADOW",
    "C_PLAN_ACTIVE_RESIDUAL",
    "anl64_build_plan",
    "04f3f884c0fc4fcc",
    "residual_active",
    "Anl64Role::QProj",
    "Anl64Role::KProj",
    "Anl64Role::OProj",
    "Anl64Role::FfnGate",
    "Anl64Role::FfnUp",
    "Anl64Role::FfnDown",
    'residual_active?"sa1_q4k_subgroup_splitk.spv":"p7_q4k_gemm_2d.spv"',
    'addop(ops,p+"ffn_gate","sa1_q4k_subgroup_splitk.spv"',
    'q4_backend.append_ffn_down_op',
    "warmups!=1||measured!=5",
    "historical_P6_timing_reused",
]
for x in required:
    assert x in src, x

decode=src[src.index("auto build_decode="):src.index("auto ppops=build_prefill(seq);")]
assert decode.count('residual_active?"sa1_q4k_subgroup_splitk.spv":"p7_q4k_gemm_2d.spv"') == 3
assert decode.count('addop(ops,p+"ffn_gate","sa1_q4k_subgroup_splitk.spv"') == 1
assert decode.count('addop(ops,p+"ffn_up","sa1_q4k_subgroup_splitk.spv"') == 1
assert decode.count("q4_backend.append_ffn_down_op") == 1
assert "anl64_q4_fast.spv" not in decode
assert "historical_ANL64_P6_timing_reuse" in spec and "false" in spec
assert '"residual_q4_fast_nodes": 84' in spec
assert '"measured_attempts_total": 60' in spec
print("ANL64_CRT_P1_STATIC_QA=PASS")
