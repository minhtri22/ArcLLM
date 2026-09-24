from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/"src/arcllm_v1_m1_post_i002_node_timing.cpp").read_text(encoding="utf-8")
assert "arcllm.v1.m1.post_i002_node_timing.v0.1" in s
assert "int warmups=1,measured=2;" in s
assert "warmups!=1||measured!=2" in s
assert "for(int i=0;i<measured;++i)" in s
assert s.count('"sa1_q4k_subgroup_splitk.spv"')==2
assert 'p+"ffn_gate","p7_q4k_gemm_2d.spv"' not in s
assert 'p+"ffn_up","p7_q4k_gemm_2d.spv"' not in s
assert "profile_probe_decode_indices" in s and "[0,15,30]" in s
assert "decode_dispatches_per_step" in s and "469" in s
b=(ROOT/"tools/build_arcllm_v1_m1.ps1").read_text(encoding="utf-8")
assert "arcllm_v1_m1_post_i002_node_timing.cpp" in b and "gguf.cpp" in b and "tensor_store.cpp" in b
sm=(ROOT/"tools/summarize_arcllm_v1_m1.py").read_text(encoding="utf-8")
for x in ["wall_attributed_ms_proxy","v_proj_","ffn_down_","measured_attempts","profiled_decode_steps"]: assert x in sm,x
print("ArcLLM v1 M1 static package PASS")
