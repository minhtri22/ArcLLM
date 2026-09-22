from pathlib import Path
import json, subprocess

ROOT=Path(__file__).resolve().parents[1]

def txt(p):
    return (ROOT/p).read_text(encoding="utf-8")

def blob(p):
    return subprocess.check_output(["git","-C",str(ROOT),"rev-parse",f"HEAD:{p}"],text=True).strip()

# Canonical governance inputs.
assert blob("src/q2_benchmark.cpp")=="ea1e986e22f6921e7f6c52a4fa5935121cfec663"
assert blob("src/anl64_runtime.cpp")=="dbcb7afed5a08e7aff3ca02a1bd95bd985076f70"
assert blob("src/p8c_segmented_access_correctness.cpp")=="8432ca554b36a2167429b640c5ec6779cf2b3e6b"
assert blob("shaders/sa1_q4k_subgroup_splitk.comp")=="56999d88dc1bef6486e7e1908982f6de4b0f9f6a"
assert blob("config/q2_execution_authorization.json")=="20f556fb18ee4ffd0ce5e17cf8fab1a2b9a992dc"
assert blob("artifacts/ANL64/ANL64_P4_BUILDONLY_ADJUDICATION_v0.1.json")=="e9039de1af496cf724a390e8180ae7d97e99f091"

auth=json.loads(txt("config/arcllm_ttft_m1_p6_implementation_authorization_v0.1.json"))
assert auth["decision"]=="P6_BOUNDED_DIAGNOSTIC_IMPLEMENTATION_AND_BUILDONLY_AUTHORIZED"
assert auth["authorization"]["implementation"] is True
assert auth["authorization"]["buildonly"] is True
assert auth["authorization"]["target_model_execution"] is False
assert auth["authorization"]["gpu_dispatch"] is False
assert auth["authorization"]["performance_measurement"] is False
assert auth["authorization"]["p7"] is False

# Measured prefill source region is exact-equal to frozen safe Q2.
safe=txt("src/q2_benchmark.cpp")
diag=txt("src/ttft_m1_diagnostic.cpp")
def prefill_block(s):
    a=s.index("        auto build_prefill=[&](uint32_t seq){")
    b=s.index("        auto build_decode=",a)
    return s[a:b]
assert prefill_block(diag)==prefill_block(safe)

# The only diagnostic compute difference is factor-controlled decode Q4FAST.
pre=prefill_block(diag)
dec=diag[diag.index("        auto build_decode="):diag.index("        auto ppops=build_prefill")]
assert "anl64_q4_fast.spv" not in pre
assert diag.count("anl64_q4_fast.spv")==5
assert dec.count("anl64_q4_fast.spv")==5
for role in ["q_proj","k_proj","o_proj","ffn_gate","ffn_up"]:
    assert f'p+"{role}",use_q4fast?' in dec,role
for forbidden in ['p+"v_proj",use_q4fast?','p+"ffn_down",use_q4fast?']:
    assert forbidden not in dec,forbidden

# Frozen factors/arm map.
for x in [
    '--decode-pipeline','--conditioning',
    '"SAFE"','"Q4FAST"','"PREFILL_ONLY"','"FULL_INFERENCE"',
    'full_conditioning?"SF":"SP"',
    'full_conditioning?"QF":"QP"',
]:
    assert x in diag,x
assert "warmups" not in diag
assert "measured!=5" in diag
assert "for(int i=0;i<5;++i)attempts.push_back(run_attempt(i));" in diag

# Causal sequence: conditioning is before reset and measured t0.
run=diag[diag.index("        auto run_attempt="):diag.index("        const double setup_ms=")]
p_cond=run.index("a.conditioning_success=condition_once();")
p_reset=run.index("reset_execution();",p_cond)
p_t0=run.index("auto t0=std::chrono::steady_clock::now();",p_reset)
p_prefill=run.index("vk.execute_prepared(ppchain,ppops,false)",p_t0)
p_top2=run.index("Q2Top2 top=q2_top2",p_prefill)
assert p_cond < p_reset < p_t0 < p_prefill < p_top2

# Conditioning semantics are frozen.
cond=diag[diag.index("        auto condition_once="):diag.index("        auto run_attempt=")]
assert "if(!full_conditioning)return ok;" in cond
assert "for(uint32_t di=0;di<31u;++di)" in cond
assert "build_decode(pos,q4fast)" in cond
assert "EXPECT_DECODE" in cond
assert "EXPECT_PREFILL" in cond

# Primary + secondary endpoints exactly permitted.
for x in [
    "ttft_ms","prefill_execute_wall_ms","submit_wait_ms",
    "host_record_and_lifecycle_ms","top2_ms",
    "first_token","final_logits_finite","dispatch_census_pass"
]:
    assert x in diag,x
assert "parent_p6_timing_reused\\\":false" in diag
for forbidden in [
    "ANL64_P6_FORMAL_ADJUDICATION",
    "anl64_p6_confirmatory",
    "P6_CANDIDATE_ADJUDICATION",
]:
    assert forbidden not in diag,forbidden

# H-ART BuildOnly reproduces the exact candidate toolchain output and
# compares it to the frozen historical SAFE SPIR-V hashes.
cp=txt("tools/compile_ttft_m1_p6_shaders.ps1")
common_names=[
    "p7_rmsnorm_seq.comp","p7c_ffn_q4k_tiled.comp","p7c_ffn_q6k_tiled.comp",
    "p7_rope_seq.comp","p7_kv_store.comp","p7_attention_prefill_online.comp",
    "p7_attention_kv_online.comp","p7_add.comp","p7l_ffn_q4k_gateup_fused.comp",
    "p7_swiglu.comp","p7g_ffn_q4k_tiled16.comp","p7g_ffn_q6k_tiled16.comp",
    "p7_q4k_gemm_2d.comp","p7_q6k_gemm_2d.comp",
    "p8c_embedding_q4k_segmented_probe.comp","p8q1_lmhead_q6k_segmented_chunk.comp",
]
assert len(common_names)==16
assert all(f'"{name}"' in cp for name in common_names)
assert "config\\q2_execution_authorization.json" in cp
assert "artifacts\\ANL64\\ANL64_P4_BUILDONLY_ADJUDICATION_v0.1.json" in cp
assert 'method="CANDIDATE_REPRODUCTION_VS_FROZEN_HISTORICAL_SAFE"' in cp
assert "historical_safe_sha256" in cp
assert "candidate_reproduction_sha256" in cp
assert 'common_prefill_artifacts_exact_match=$AllHistoricalMatch' in cp
assert 'H_ART_FALSIFIED_STATIC' in cp
assert 'H_ART_SUPPORTED_STATIC_STOP_TIMING' in cp
assert "q4_safe_spv_sha256" in cp and "q6_safe_spv_sha256" in cp
assert "B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569" in cp
assert "16.5.0" in cp and "vulkan1.2" in cp
assert "shaders_safe" not in cp and "shaders_q4fast" not in cp

safe_auth=json.loads(txt("config/q2_execution_authorization.json"))
assert len(safe_auth["compiled_shader_sha256"])==16
assert safe_auth["compiled_shader_sha256"]["p7_q4k_gemm_2d.spv"]=="2EFD94ACDA45555AF1C082C916AF7C3F1AA1BE46AD7868B7C4BE868816CAEE4A"
assert safe_auth["compiled_shader_sha256"]["p7_q6k_gemm_2d.spv"]=="F2267838D099128F233EF30817464658AAD71AAFA3933461FB315FAD10ED3F67"

cand_adj=json.loads(txt("artifacts/ANL64/ANL64_P4_BUILDONLY_ADJUDICATION_v0.1.json"))
assert cand_adj["shader_build"]["compiler_release"]=="16.5.0"
assert cand_adj["shader_build"]["target_env"]=="vulkan1.2"
assert cand_adj["shader_build"]["q4_fast_spv_sha256"]=="B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"
assert cand_adj["shader_build"]["q4_safe_spv_sha256"]==safe_auth["compiled_shader_sha256"]["p7_q4k_gemm_2d.spv"]
assert cand_adj["shader_build"]["q6_safe_spv_sha256"]==safe_auth["compiled_shader_sha256"]["p7_q6k_gemm_2d.spv"]

bp=txt("tools/build_ttft_m1_p6.ps1")
assert "ttft_m1_diagnostic.cpp" in bp
assert "ttft_m1_diagnostic.exe" in bp
assert "executable_launched=$false" in bp
assert "model_loaded=$false" in bp
assert "gpu_dispatch=$false" in bp
assert "performance_measurement=$false" in bp
assert "& $Exe" not in bp

g=json.loads(txt("config/arcllm_ttft_mechanism_governance_v0.1.json"))
assert g["status"] in {
    "P6_DIAGNOSTIC_IMPLEMENTATION_AUTHORIZED_BUILDONLY_ONLY",
    "P6_IMPLEMENTATION_LOCKED_BUILDONLY_PENDING",
}
assert g["stage_state"]["P6"] in {
    "IMPLEMENTATION_AUTHORIZED_BUILDONLY_ONLY",
    "IMPLEMENTATION_LOCKED_BUILDONLY_PENDING",
}
assert g["authorization"]["build"] is True
assert g["authorization"]["target_model_execution"] is False
assert g["authorization"]["gpu_dispatch"] is False
assert g["authorization"]["performance_measurement"] is False

print("TTFT M1 P6 static package: PASS")
