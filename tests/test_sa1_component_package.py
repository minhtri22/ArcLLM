#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def req(x,m):
    if not x: raise AssertionError(m)
lock=json.loads((R/"config/sa1p_implementation_lock_v0.1.json").read_text(encoding="utf-8"))
cfg=json.loads((R/"config/sa1_k1_implementation_contract_v0.1.json").read_text(encoding="utf-8"))
man=json.loads((R/"manifest.json").read_text(encoding="utf-8"))
sh=(R/"shaders/sa1_q4k_subgroup_splitk.comp").read_text(encoding="utf-8")
cpp=(R/"src/sa1_component_benchmark.cpp").read_text(encoding="utf-8")
pre=(R/"run_sa1_component_preflight.ps1").read_text(encoding="utf-8")
run=(R/"run_sa1_component_q4.ps1").read_text(encoding="utf-8")
def blob(p): return subprocess.check_output(["git","-C",str(R),"rev-parse","HEAD:"+p],text=True).strip()
req(lock["status"]=="SA1P_LOCK_PASS_SA1K1_IMPLEMENTATION_AUTHORIZED","parent lock")
req(cfg["parent_implementation_lock_commit"]=="60b0fdf91ffe859c918055afa9e3b1071142f6c4","parent commit")
req(cfg["primary_mechanism"]=="SUBGROUP32_SPLIT_K_PER_OUTPUT_ROW","mechanism")
req(cfg["measurement_permitted"] is False and cfg["target_model_execution_permitted"] is False and cfg["q6_implementation_permitted"] is False,"closed gates")
req(blob("shaders/p7_q4k_gemm_2d.comp")=="fb1fb14192ff7d275a4af38c6dd9be7d1b500a7a","baseline Q4 drift")
sa1=list((R/"shaders").glob("sa1_*.comp"));req(len(sa1)==1 and sa1[0].name=="sa1_q4k_subgroup_splitk.comp","exactly one SA1 shader")
for s in ["local_size_x = 128","GL_KHR_shader_subgroup_basic","GL_KHR_shader_subgroup_arithmetic","gl_SubgroupID","gl_SubgroupInvocationID","subgroupAdd","k += 32u","gl_WorkGroupID.x * 4u"]:
    req(s in sh,"shader missing "+s)
for s in ["shared ","coopmat","cooperative","float16_t","int8_t"]:
    req(s.lower() not in sh.lower(),"shader forbidden "+s)
for s in ["VkPipelineShaderStageRequiredSubgroupSizeCreateInfo","requiredSubgroupSize=32","VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT","timestampValidBits!=64u","--mode","preflight","measure","10","30"]:
    req(s in cpp,"harness missing "+s)
req("performance_measurement" in cpp and "PASS_CORRECTNESS_ZERO_MEASUREMENT" in cpp,"preflight evidence")
req("--mode preflight" in pre and "--mode measure" not in pre,"preflight must not measure")
req("timestamp_queries-ne0" in pre and "measured_pairs-ne0" in pre and "performance_gate_evaluated" in pre,"zero-measurement guards")
req("SA1 Q4 execution authorization missing; measurement forbidden" in run and "SA1_Q4_EXECUTION_AUTHORIZED" in run,"measurement authorization gate")
req(man["sa1"]["q4_implementation_permitted"] is True and man["sa1"]["q6_implementation_permitted"] is False and man["sa1"]["component_measurement_permitted"] is False,"manifest gates")
print("SA1_K1_STATIC_QA_PASS")
