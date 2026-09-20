from pathlib import Path
import json, re

ROOT=Path(__file__).resolve().parents[1]
def text(p): return (ROOT/p).read_text(encoding="utf-8")

src=text("src/q1_end_to_end.cpp")
shader=text("shaders/p8q1_lmhead_q6k_segmented_chunk.comp")
compile_ps=text("tools/compile_q1_shaders.ps1")
build_ps=text("tools/build_q1.ps1")
runner=text("run_q1.ps1")
contract=text("docs/P8_Q1_END_TO_END_CONTRACT.md")
impl=text("docs/P8_Q1_IMPLEMENTATION.md")
manifest=json.loads(text("manifest.json"))

assert "H=3584" in src and "LAYERS=28" in src and "VOC=152064" in src
assert "QH=28" in src and "KVH=4" in src and "FFN=18944" in src and "MAXCTX=4096" in src
assert "input_ids={1u,133151u,133152u,152062u}" in src
assert "EXPECT_PREFILL=441" in src and "EXPECT_DECODE=469" in src
assert "generated_token_count\\\":5" in src
assert "e.generated.size()==5u" in src
assert 'Q1ExecutionObs A=run_one("A"),B=run_one("B")' in src
assert "std::memset(b_kcache.mapped,0" in src and "std::memset(b_vcache.mapped,0" in src
assert "p8c_embedding_q4k_segmented_probe.spv" in src
assert "p8q1_lmhead_q6k_segmented_chunk.spv" in src
assert "p7_attention_prefill_online.spv" in src and "p7_attention_kv_online.spv" in src
assert "p7l_ffn_q4k_gateup_fused.spv" in src
assert "p7g_ffn_q4k_tiled16.spv" in src and "p7g_ffn_q6k_tiled16.spv" in src
assert "p7_q4k_gemm_2d.spv" in src and "p7_q6k_gemm_2d.spv" in src
assert src.count("for(uint32_t l=0;l<LAYERS;++l)") >= 3
main=src[src.index("int main("):]
assert "matmul_q4_cpu(" not in main and "matmul_q6_cpu(" not in main
assert '"cpu_model_math_fallback":false' in main
assert '"cpu_teacher_forcing":false' in main
assert "q1_top2(lp,VOC)" in main
assert "actual_input==expected_input" in main
assert "A.generated==B.generated" in main

assert "layout(local_size_x = 64)" in shader
assert "uint row=pc.row_start+i;" in shader
assert "row<pc.boundary" in shader
assert "Y.y[row]=sum;" in shader
assert "pc.x_base+ib*256u+k" in shader

required_shaders=[
 "p7_rmsnorm_seq.comp","p7c_ffn_q4k_tiled.comp","p7c_ffn_q6k_tiled.comp",
 "p7_rope_seq.comp","p7_kv_store.comp","p7_attention_prefill_online.comp",
 "p7_attention_kv_online.comp","p7_add.comp","p7l_ffn_q4k_gateup_fused.comp",
 "p7_swiglu.comp","p7g_ffn_q4k_tiled16.comp","p7g_ffn_q6k_tiled16.comp",
 "p7_q4k_gemm_2d.comp","p7_q6k_gemm_2d.comp",
 "p8c_embedding_q4k_segmented_probe.comp","p8q1_lmhead_q6k_segmented_chunk.comp",
]
for name in required_shaders:
    assert name in compile_ps, name
assert '-ne 16' in compile_ps
assert "arcllm.q1.shader_provenance.v1" in compile_ps
assert "q1_end_to_end.cpp" in build_ps and "arcllm_q1.exe" in build_ps

for h in [
 "1490475D0D7EAA0498FEEA5CD0A37460C4881FFFF676A7C912E0E113E2CAAC84",
 "0526D3F1400080AF6B68D1D44CBD2AF897671B727EA924EC0D0C953C16FDD259",
 "13C36E5EB14D08F60C3DC9277F7A21EE4033DB50D84806839DE62B3E9F7EE303",
 "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463",
]:
    assert h in runner or h in contract
assert "Q1 critical working-tree files differ from HEAD" in runner
assert "arcllm.q1.environment.v1" in runner
assert "arcllm.q1.evidence_manifest.v1" in runner
assert "F3_evidence_complete=$true" in runner
assert "q1_return_to_chatgpt.zip" in runner
assert "Q1_FEASIBILITY_ESTABLISHED" in runner

assert "five generated token IDs total" in contract
assert "No Q1 evidence existed when this clarification was made." in contract
assert "requested weight + KV + working residency" in contract
assert "IMPLEMENTATION_LOCKED / TARGET RUN AUTHORIZED" in impl

assert manifest["phase"]=="Q1-END-TO-END"
assert manifest["status"]=="IMPLEMENTATION_LOCKED"
assert manifest["q1"]["generated_tokens_total"]==5
assert manifest["q1"]["cached_decode_steps"]==4
assert manifest["q1"]["prefill_dispatches"]==441
assert manifest["q1"]["decode_dispatches_per_step"]==469
assert manifest["q1"]["cpu_model_math_fallback_permitted"] is False
assert manifest["q1"]["cpu_teacher_forcing_permitted"] is False
assert manifest["target_run_permitted"] is True
assert manifest["q1_end_to_end_target_run_permitted"] is True
assert manifest["q2_permitted"] is False and manifest["q3_permitted"] is False
print("Q1 static package: PASS")
