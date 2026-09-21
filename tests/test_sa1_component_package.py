#!/usr/bin/env python3
from __future__ import annotations
import json,re,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1]
def req(x,m):
    if not x: raise AssertionError(m)
def blob(p): return subprocess.check_output(["git","-C",str(R),"rev-parse","HEAD:"+p],text=True).strip()
q6lock=json.loads((R/"config/sa1_k2_q6_implementation_lock_v0.1.json").read_text(encoding="utf-8"))
q6cfg=json.loads((R/"config/sa1_k2_q6_implementation_contract_v0.1.json").read_text(encoding="utf-8"))
man=json.loads((R/"manifest.json").read_text(encoding="utf-8"))
q4=(R/"shaders/sa1_q4k_subgroup_splitk.comp").read_text(encoding="utf-8")
q6=(R/"shaders/sa1_q6k_subgroup_splitk.comp").read_text(encoding="utf-8")
cpp=(R/"src/sa1_component_benchmark.cpp").read_text(encoding="utf-8")
pre=(R/"run_sa1_component_preflight.ps1").read_text(encoding="utf-8")
runq6=(R/"run_sa1_component_q6.ps1").read_text(encoding="utf-8")
compile_ps=(R/"tools/compile_sa1_component.ps1").read_text(encoding="utf-8")
build_ps=(R/"tools/build_sa1_component.ps1").read_text(encoding="utf-8")
req(q6lock["status"]=="SA1_K2_Q6_IMPLEMENTATION_AUTHORIZED","Q6 lock")
req(q6lock["prerequisite"]["q4_result"]=="Q4_STAGE_PASS","Q4 prerequisite")
req(q6cfg["parent_q6_implementation_lock_commit"]=="2a7a7a2ecec38aac0853f112e7bea01bbad7514a","Q6 parent")
req(q6cfg["measurement_permitted"] is False and q6cfg["target_model_execution_permitted"] is False,"Q6 closed execution gates")
req(blob("shaders/sa1_q4k_subgroup_splitk.comp")=="56999d88dc1bef6486e7e1908982f6de4b0f9f6a","Q4 candidate drift")
req(blob("shaders/p7_q6k_gemm_2d.comp")=="a0de99f972db6cd202606aad95fb9eab223639e6","Q6 baseline drift")
sa1=sorted(p.name for p in (R/"shaders").glob("sa1_*.comp"))
req(sa1==["sa1_q4k_subgroup_splitk.comp","sa1_q6k_subgroup_splitk.comp"],"exact SA1 shader census")
for s in ["local_size_x = 128","GL_KHR_shader_subgroup_basic","GL_KHR_shader_subgroup_arithmetic","gl_SubgroupID","gl_SubgroupInvocationID","subgroupAdd","k += 32u","gl_WorkGroupID.x * 4u","base + 208u","ib * 210u"]:
    req(s in q6,"Q6 shader missing "+s)
for s in ["shared ","coopmat","cooperative","float16_t","int8_t"]:
    req(s.lower() not in q6.lower(),"Q6 shader forbidden "+s)
for s in ["Q6_H3584_R512_BIAS","Q6_H18944_R3584_NOBIAS","q6_value_cpu","q6_weight","--quant","quant==\"q6\"","SA1_Q6_EXECUTION_AUTHORIZED"]:
    req(s in cpp,"Q6 harness missing "+s)
req("timed_dispatches=measured_pairs*2u" in cpp and "timestamp_values=timed_dispatches*2u" in cpp,"dynamic timing census")
req("--quant q6 --mode preflight" in pre and "-Stage K2" in pre,"Q6 preflight path")
req("[switch]$PackageFailedExisting" in pre and "Q6_CORRECTNESS_FAIL_PENDING_INDEPENDENT_ADJUDICATION" in pre,"Q6 failure packaging recovery")
req("failure packaging raw error mismatch" in pre and "executed shader/harness drift" in pre,"Q6 failure package binds exact failed execution")
req("NO Q6 RERUN OR PERFORMANCE MEASUREMENT AUTHORIZED" in pre,"Q6 failure recovery remains stop-closed")
req("timestamp_queries-ne0" in pre and "measured_pairs-ne0" in pre and "performance_gate_evaluated" in pre,"zero-measurement guards")
req("SA1 Q6 execution authorization missing; measurement forbidden" in runq6 and "SA1_Q6_EXECUTION_AUTHORIZED" in runq6,"Q6 measurement fail-closed")
req('ValidateSet("K1","K2")' in compile_ps and "F2267838D099128F233EF30817464658AAD71AAFA3933461FB315FAD10ED3F67" in compile_ps,"Q6 compile frozen baseline")
req('ValidateSet("K1","K2")' in build_ps and '"artifacts\\SA1_"+$Stage+"\\build"' in build_ps,"Q6 build stage isolation")
for ps in [pre,runq6,compile_ps,build_ps]:
    req(not re.search(r"\b(?:Test-Path|Get-Content|Get-FileHash|Remove-Item|Compress-Archive)\$[A-Za-z_]",ps),"PowerShell cmdlet/variable tokenization")
req(man["sa1"]["q6_implementation_permitted"] is False and man["sa1"]["component_measurement_permitted"] is False and man["sa1"]["k2"]["rerun_authorized"] is False,"manifest Q6 correctness-fail stop gate")
print("SA1_K2_STATIC_QA_PASS")
