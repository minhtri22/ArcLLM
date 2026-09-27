from pathlib import Path

root=Path(__file__).resolve().parents[1]
src=(root/"src/anl64_crt_p1r_runtime.cpp").read_text(encoding="utf-8")
spec=(root/"config/anl64_crt_p1r_prebound_plan_causal_separation_gate_v0.1.json").read_text(encoding="utf-8")
contract=(root/"config/anl64_p2_architecture_contract_v0.1.json").read_text(encoding="utf-8")

required=[
    '#include "anl64_plan.hpp"',
    "A_CANONICAL",
    "B_PLAN_PREBOUND",
    "C_PLAN_PREBOUND_RESIDUAL",
    "anl64_build_plan",
    "04f3f884c0fc4fcc",
    "PreboundPlan",
    "q_proj_shader",
    "k_proj_shader",
    "o_proj_shader",
    "prebound_before_timing",
    "per_token_plan_lookup",
    'addop(ops,p+"ffn_gate","sa1_q4k_subgroup_splitk.spv"',
    "q4_backend.append_ffn_down_op",
    "invalid_P1_partial_reused",
    "warmups!=1||measured!=5",
]
for x in required:
    assert x in src, x

assert "plan_node" not in src
assert '"per_token_executor_selection": false' in contract
assert '"construction_time": "MODEL_LOAD_ONLY"' in contract
assert '"partial_P1_evidence_reused": false' in spec
assert '"measured_attempts_total": 60' in spec

build_pos=src.index("anl64_build_plan")
decode_start=src.index("auto build_decode=")
decode_end=src.index("auto ppops=build_prefill(seq);")
decode=src[decode_start:decode_end]
assert build_pos < decode_start
assert "Anl64Role::" not in decode
assert "residual_active" not in decode
assert "prebound." not in decode
assert decode.count("q_proj_shader")==1
assert decode.count("k_proj_shader")==1
assert decode.count("o_proj_shader")==1
assert decode.count('addop(ops,p+"ffn_gate","sa1_q4k_subgroup_splitk.spv"')==1
assert decode.count('addop(ops,p+"ffn_up","sa1_q4k_subgroup_splitk.spv"')==1
assert decode.count("q4_backend.append_ffn_down_op")==1
print("ANL64_CRT_P1R_STATIC_QA=PASS")
