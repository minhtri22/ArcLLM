from pathlib import Path
import json, subprocess

ROOT=Path(__file__).resolve().parents[1]

def txt(p):
    return (ROOT/p).read_text(encoding="utf-8")

def blob(p):
    return subprocess.check_output(["git","-C",str(ROOT),"rev-parse",f"HEAD:{p}"],text=True).strip()

# P9A canonical design identities.
assert blob("config/arcllm_ttft_m2_p9a_source_revalidation_v0.1.json")=="17759ae80ce0cbbfb4d2dedd88c0aabe8216ac9c"
assert blob("config/arcllm_ttft_m2_p9a_hypotheses_v0.1.json")=="55192815d02637cc6e469794565ba8134ff63ae1"
assert blob("config/arcllm_ttft_m2_p9a_causal_decomposition_v0.1.json")=="9d1960ee253e59f1835ea0d320b40e9975039ed3"
assert blob("config/arcllm_ttft_m2_p9a_target_environment_contract_v0.1.json")=="5b79f7b42b238471e0d4bb3ee802bab7ffe83185"
assert blob("config/arcllm_ttft_m2_p9a_falsification_stop_v0.1.json")=="1ed4187934be119611f9fd1f0ce9f8e61bf88116"
assert blob("config/arcllm_ttft_m2_p9a_scientific_design_manifest_v0.1.json")=="abce0545cec33367f9b3a82f15d5ffd4fb026f64"

# Immutable production/source identities revalidated by P9A.
assert blob("src/q2_benchmark.cpp")=="ea1e986e22f6921e7f6c52a4fa5935121cfec663"
assert blob("src/anl64_runtime.cpp")=="dbcb7afed5a08e7aff3ca02a1bd95bd985076f70"
assert blob("src/p8c_segmented_access_correctness.cpp")=="8432ca554b36a2167429b640c5ec6779cf2b3e6b"
assert blob("shaders/sa1_q4k_subgroup_splitk.comp")=="56999d88dc1bef6486e7e1908982f6de4b0f9f6a"

safe=txt("src/q2_benchmark.cpp")
diag=txt("src/ttft_m2_diagnostic.cpp")
assert "TTFT M1" not in diag and "TTFT_M1_" not in diag
assert "ARCLLM_TTFT_M1" not in diag and "arcllm.ttft_m1" not in diag
assert "ARCLLM_TTFT_M2" in diag and "arcllm.ttft_m2.diagnostic_cell.v0.1" in diag

def prefill_block(s):
    a=s.index("        auto build_prefill=[&](uint32_t seq){")
    b=s.index("        auto build_decode=",a)
    return s[a:b]
assert prefill_block(diag)==prefill_block(safe)
pre=prefill_block(diag)
dec=diag[diag.index("        auto build_decode="):diag.index("        auto ppops=build_prefill")]
assert "anl64_q4_fast.spv" not in pre
assert dec.count("anl64_q4_fast.spv")==5
for x in ['--decode-pipeline','--conditioning','"SAFE"','"Q4FAST"','"PREFILL_ONLY"','"FULL_INFERENCE"']:
    assert x in diag,x
assert 'full_conditioning?"SF":"SP"' in diag
assert 'full_conditioning?"QF":"QP"' in diag
assert "measured!=5" in diag
assert "for(int i=0;i<5;++i)attempts.push_back(run_attempt(i));" in diag
for x in ["ttft_ms","prefill_execute_wall_ms","submit_wait_ms","host_record_and_lifecycle_ms","top2_ms","first_token","final_logits_finite","dispatch_census_pass"]:
    assert x in diag,x
assert "scientific_execution_authorization_required" in diag

build=txt("tools/build_ttft_m2_p9b.ps1")
assert "ttft_m2_diagnostic.cpp" in build and "ttft_m2_diagnostic.exe" in build
assert "executable_launched=$false" in build
assert "model_loaded=$false" in build
assert "gpu_dispatch=$false" in build
assert "performance_measurement=$false" in build
assert "& $Exe" not in build

sh=txt("tools/compile_ttft_m2_p9b_shaders.ps1")
assert "glslang-16.5.0" in sh and "vulkan1.2" in sh
assert "H-ART not adjudicated" in sh
assert "h_art_mechanism_adjudication_performed=$false" in sh
assert "performance_measurement=$false" in sh
assert "anl64_q4_fast.spv" in sh

science=txt("scripts/ttft_m2/run_p9_science.ps1")
assert "arcllm_ttft_m2_p9c_execution_authorization_v0.1.json" in science
assert "M2_P9C_FRESH_MECHANISM_IDENTIFICATION_EXECUTION_AUTHORIZED" in science
assert "H_ART_SUPPORTED_STATIC_STOP_TIMING" in science
assert science.index("P9C execution authorization missing") < science.index("Get-FileHash $Model")
for cell in ["SP/W-S","QP/W-S","QF/W-S","SF/W-S","SF/W-C","QF/W-C","QP/W-C","SP/W-C"]:
    assert cell in science
assert '--measured 5' in science
assert "planned_observations=80" in science
assert "mechanism_adjudication_performed=$false" in science

g=json.loads(txt("config/arcllm_ttft_m2_governance_v0.1.json"))
assert g["stage_state"]["M2_P9"] in {"OPEN_IMPLEMENTATION_ONLY_EXECUTION_BLOCKED","OPEN_P9B_BUILDONLY_QUALIFICATION"}
assert g["authorization"]["diagnostic_science_implementation"] is True
assert g["authorization"]["target_model_execution"] is False
assert g["authorization"]["model_load"] is False
assert g["authorization"]["gpu_dispatch"] is False
assert g["authorization"]["performance_measurement"] is False
assert g["authorization"]["fresh_ttft_observation"] is False

print("TTFT M2 P9B static package QA: PASS")
