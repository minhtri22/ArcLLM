from pathlib import Path
import json, subprocess

ROOT=Path(__file__).resolve().parents[1]

def txt(p):
    return (ROOT/p).read_text(encoding="utf-8")

def blob(p):
    return subprocess.check_output(["git","-C",str(ROOT),"rev-parse",f"HEAD:{p}"],text=True).strip()

# Immutable parent/source evidence.
assert blob("src/q2_benchmark.cpp")=="ea1e986e22f6921e7f6c52a4fa5935121cfec663"
assert blob("shaders/p7_q4k_gemm_2d.comp")=="fb1fb14192ff7d275a4af38c6dd9be7d1b500a7a"
assert blob("shaders/sa1_q4k_subgroup_splitk.comp")=="56999d88dc1bef6486e7e1908982f6de4b0f9f6a"

auth=json.loads(txt("config/anl64_p4_implementation_authorization_v0.1.json"))
assert auth["decision"]=="P4_BOUNDED_IMPLEMENTATION_AUTHORIZED"
assert auth["authorization"]["implementation"] is True
assert auth["authorization"]["target_model_inference"] is False
assert auth["authorization"]["scientific_performance_measurement"] is False

plan=txt("src/anl64_plan.hpp")
for x in [
    "plan.nodes.reserve(469u)",
    "plan.regions.reserve(24104u)",
    "plan.quant_linear_nodes != 215u",
    "plan.fixed_q4_fast_nodes != 140u",
    "plan.regions.size() != 24104u",
    "plan.fixed_q4_regions != 19936u",
    "plan.metadata_bytes > 2097152ull",
    "0xffffffffffffffffull",
]:
    assert x in plan,x
assert "Q4FastFixed" in plan
assert "Region64 descriptor exceeds P2 budget" in plan
assert "PlanNode descriptor exceeds P2 budget" in plan

src=txt("src/anl64_runtime.cpp")
assert '#include "anl64_plan.hpp"' in src
assert "const Anl64Plan anl64_plan=anl64_build_plan" in src
assert "anl64_plan.nodes.size()!=EXPECT_DECODE" in src
assert src.count('"anl64_q4_fast.spv"')==5
assert "arcllm.anl64.runtime_cell.v0.1" in src
assert '"system\\":\\\"ANL64' in src
assert '"anl64_plan\\":{' in src

pre=src[src.index("auto build_prefill="):src.index("auto build_decode=")]
dec=src[src.index("auto build_decode="):src.index("auto ppops=build_prefill")]
assert "anl64_q4_fast.spv" not in pre
assert dec.count('"anl64_q4_fast.spv"')==5

for role in ["q_proj","k_proj","o_proj","ffn_gate","ffn_up"]:
    assert f'p+"{role}","anl64_q4_fast.spv"' in dec,role
for forbidden in [
    'p+"v_proj","anl64_q4_fast.spv"',
    'p+"ffn_down","anl64_q4_fast.spv"',
]:
    assert forbidden not in dec,forbidden

assert 'p+"q_proj","anl64_q4_fast.spv"' in dec and "),H/4u);" in dec
assert 'p+"k_proj","anl64_q4_fast.spv"' in dec and "),KV/4u);" in dec
assert dec.count("),FFN/4u);")>=2
assert 'z.vw->ggml_type==Q4?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv"' in dec
assert 'z.dw->ggml_type==Q4?"p7_q4k_gemm_2d.spv":"p7_q6k_gemm_2d.spv"' in dec

# Prefill remains exact legacy safe path.
for x in [
    '"p7c_ffn_q4k_tiled.spv"',
    '"p7l_ffn_q4k_gateup_fused.spv"',
    '"p7g_ffn_q4k_tiled16.spv"',
]:
    assert x in pre,x

compile_ps=txt("tools/compile_anl64_p4_shaders.ps1")
assert compile_ps.count('@{source=')==17
assert 'source="sa1_q4k_subgroup_splitk.comp";spv="anl64_q4_fast.spv"' in compile_ps
assert 'shader_count=$Compiled.Count' in compile_ps
assert '$Compiled.Count -ne 17' in compile_ps
assert '16.5.0' in compile_ps and 'vulkan1.2' in compile_ps

build_ps=txt("tools/build_anl64_p4.ps1")
assert "anl64_runtime.cpp" in build_ps
assert "anl64_p4.exe" in build_ps
assert "executable_launched=$false" in build_ps
assert "model_loaded=$false" in build_ps
assert "gpu_dispatch=$false" in build_ps
assert "performance_measurement=$false" in build_ps
assert "& $Exe" not in build_ps

m=json.loads(txt("manifest.json"))
assert m["anl64"]["stage_state"]["P4"]=="IMPLEMENTATION_AUTHORIZED"
assert m["anl64"]["authorization"]["implementation"] is True
assert m["anl64"]["authorization"]["target_execution"] is False
assert m["anl64"]["authorization"]["scientific_measurement"] is False

print("ANL64 P4 static package: PASS")
