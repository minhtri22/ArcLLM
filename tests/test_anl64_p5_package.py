from pathlib import Path
import json, subprocess

ROOT=Path(__file__).resolve().parents[1]

def txt(p):
    return (ROOT/p).read_text(encoding="utf-8")

def blob(p):
    return subprocess.check_output(["git","-C",str(ROOT),"rev-parse",f"HEAD:{p}"],text=True).strip()

# P5 frozen science/governance.
spec=json.loads(txt("config/anl64_p5_integration_validation_spec_v0.1.json"))
qa=json.loads(txt("artifacts/ANL64/ANL64_P5_SPECIFICATION_QA_v0.1.json"))
auth=json.loads(txt("config/anl64_p5_execution_authorization_v0.1.json"))

assert spec["status"]=="P5_SPECIFICATION_FROZEN_EXECUTION_BLOCKED"
assert qa["result"]=="PASS_ZERO_SCIENCE_P5_SPECIFICATION_QA"
assert auth["decision"]=="P5_BOUNDED_SEMANTIC_INTEGRATION_EXECUTION_AUTHORIZED"
assert auth["authorization"]["p5_target_model_load"] is True
assert auth["authorization"]["p5_gpu_dispatch"] is True
assert auth["authorization"]["p5_semantic_execution"] is True
assert auth["authorization"]["p5_performance_adjudication"] is False
assert auth["authorization"]["p6"] is False
assert auth["rerun_policy"]["selective_cell_or_attempt_rerun"] is False
assert auth["rerun_policy"]["after_valid_repair"]=="FULL_FROZEN_P5_REPLAY_ONLY"

# Production candidate remains exactly P4-locked.
assert blob("src/anl64_runtime.cpp")=="dbcb7afed5a08e7aff3ca02a1bd95bd985076f70"
assert blob("src/anl64_plan.hpp")=="157be15c63363ba2d55093af829ca68be9107e27"
assert blob("shaders/sa1_q4k_subgroup_splitk.comp")=="56999d88dc1bef6486e7e1908982f6de4b0f9f6a"

# Safe reference source/build path is exactly the historical Q2 implementation.
expected={
"src/q2_benchmark.cpp":"ea1e986e22f6921e7f6c52a4fa5935121cfec663",
"src/gguf.cpp":"bda721b3f856d4bfb9dd2a56208dc2c3c2ae974b",
"src/gguf.h":"d71d22cc1db54a9643ecafa6d3b616a96221c726",
"src/tensor_store.cpp":"0eb52588d30620bfeb323e62c8546e7191a006d6",
"src/tensor_store.h":"fdc8788b72da4c0f9464aabdaa48e680d153fc5d",
"tools/compile_q2_shaders.ps1":"a7e08a196ee6c6bf94f460ef4d710175a1b85166",
"tools/build_q2.ps1":"18325b0a2b3c319f304724397cfe8dfb6d17377f",
"shaders/p7_rmsnorm_seq.comp":"69b16bda755551664afec768e1afe1cb08a295bc",
"shaders/p7c_ffn_q4k_tiled.comp":"52a457171ebcc23b005d10a80ef3d3175ae8b3d3",
"shaders/p7c_ffn_q6k_tiled.comp":"5ba8cb83e7a3a7cdafc0253b3d150d5c78cb412d",
"shaders/p7_rope_seq.comp":"8013a58efcb8a83b41ac12b32a1e8a5d69cec3d2",
"shaders/p7_kv_store.comp":"ce6488589dbdfe2c88f1426858f8bb625ba6231b",
"shaders/p7_attention_prefill_online.comp":"a43180205f7d3e44ed60faef22be6f25443a8fa3",
"shaders/p7_attention_kv_online.comp":"da5f7d9ac54245cf90db473212cbc050a8ec24d3",
"shaders/p7_add.comp":"8803bcef591e1c1a0a4ad1434af14832daa1984b",
"shaders/p7l_ffn_q4k_gateup_fused.comp":"ad6b7c5bf338c604506a0120e6550eaa09e8e053",
"shaders/p7_swiglu.comp":"df71ff50b1c533a8e70aec8f80321a00b9536e41",
"shaders/p7g_ffn_q4k_tiled16.comp":"4d66a5448959259ae71d87bbba68886ae948aff7",
"shaders/p7g_ffn_q6k_tiled16.comp":"169a470158ad6b3daefbb3003fb728a899f03bfb",
"shaders/p7_q4k_gemm_2d.comp":"fb1fb14192ff7d275a4af38c6dd9be7d1b500a7a",
"shaders/p7_q6k_gemm_2d.comp":"a0de99f972db6cd202606aad95fb9eab223639e6",
"shaders/p8c_embedding_q4k_segmented_probe.comp":"84561dbe9a0e0b0412480a7a758ba90f937a6746",
"shaders/p8q1_lmhead_q6k_segmented_chunk.comp":"184b769a18de89d1f7fa53a8df19cfc0bc5a8fd3",
}
for p,h in expected.items():
    assert blob(p)==h,(p,blob(p),h)

# Semantic extractor has a strict allow-list and cannot copy performance fields.
ext=txt("tools/extract_anl64_p5_semantics.py")
for field in [
    '"success"','"final_logits_finite"','"dispatch_census_pass"',
    '"generated_token_ids"','"generated_hash_fnv1a64"',
    '"final_logits_hash_fnv1a64"','"final_hidden_hash_fnv1a64"'
]:
    assert field in ext,field
for forbidden in ['"ttft_ms"','"decode_ms"','"decode_tps"','"e2e_ms"','"setup_ms_descriptive"']:
    assert forbidden not in ext,forbidden
assert '"performance_fields_extracted":False' in ext
assert '"raw_timing_decision_role":"NONE_SPENT_FOR_P6"' in ext
assert 'rs==cs' in ext
assert 'len(rs)==32' in ext
assert 'p.get("nodes")==469' in ext
assert 'p.get("fixed_q4_fast_nodes")==140' in ext
assert 'p.get("regions")==24104' in ext
assert 'p.get("fixed_q4_regions")==19936' in ext

# One-shot runner is bound to a future lock and exact four-cell order.
run=txt("run_anl64_p5_integration.ps1")
assert 'config\\anl64_p5_execution_lock_v0.1.json' in run
assert 'P5_BOUNDED_SEMANTIC_INTEGRATION_EXECUTION_AUTHORIZED' in run
assert 'p5_performance_adjudication' in run
assert 'P6_NOT_AUTHORIZED' in run
assert 'PERFORMANCE_FIELDS_QUARANTINED' in run
assert 'tools\\extract_anl64_p5_semantics.py' in run
assert 'tools\\compile_q2_shaders.ps1' in run
assert 'tools\\build_q2.ps1' in run
assert 'q2_resource_sampler' not in run
assert 'summarize_q2' not in run

anchors=[
    '$Exit.reference_ws=Invoke-Cell "ARC_SAFE_REFERENCE/W-S"',
    '$Exit.candidate_ws=Invoke-Cell "ANL64_P4_LOCKED/W-S"',
    '$Exit.reference_wc=Invoke-Cell "ARC_SAFE_REFERENCE/W-C"',
    '$Exit.candidate_wc=Invoke-Cell "ANL64_P4_LOCKED/W-C"',
]
pos=[run.index(x) for x in anchors]
assert pos==sorted(pos)
assert all(run.count(x)==1 for x in anchors)
assert "--warmups 1 --measured 5" in run

# Runner verifies exact P4 candidate artifact and exact target before science.
assert "1F45DA9D8CACE3CF78FE31B7B7041B6027CB41E180F7FC99E50E1127EA4F5451" not in run  # value is read from authorization
assert "candidate executable SHA mismatch" in run
assert "model SHA256 mismatch" in run
assert "GPU driver mismatch" in run
assert "ANL64 runtime drift" in run
assert "safe reference drift" in run

print("ANL64 P5 runner static package: PASS")
