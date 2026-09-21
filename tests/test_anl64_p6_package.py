from pathlib import Path
import json, subprocess

ROOT=Path(__file__).resolve().parents[1]

def txt(p): return (ROOT/p).read_text(encoding="utf-8")
def blob(p): return subprocess.check_output(["git","-C",str(ROOT),"rev-parse",f"HEAD:{p}"],text=True).strip()

spec=json.loads(txt("config/anl64_p6_matched_e2e_spec_v0.1.json"))
qa=json.loads(txt("artifacts/ANL64/ANL64_P6_SPECIFICATION_QA_v0.1.json"))
auth=json.loads(txt("config/anl64_p6_execution_authorization_v0.1.json"))

assert spec["status"]=="P6_SPECIFICATION_FROZEN_EXECUTION_BLOCKED"
assert qa["result"]=="PASS_ZERO_SCIENCE_P6_SPECIFICATION_QA"
assert auth["decision"]=="P6_MATCHED_E2E_EXECUTION_AUTHORIZED"
assert auth["authorization"]["performance_measurement"] is True
assert auth["authorization"]["production_source_mutation"] is False
assert auth["authorization"]["shader_mutation"] is False
assert auth["authorization"]["threshold_mutation"] is False
assert auth["authorization"]["workload_mutation"] is False
assert auth["authorization"]["p7"] is False
assert auth["freshness"]["p5_timing_reuse"] is False
assert auth["freshness"]["selective_rerun"] is False

# Candidate and safe-reference production identity.
assert blob("src/anl64_runtime.cpp")=="dbcb7afed5a08e7aff3ca02a1bd95bd985076f70"
assert blob("src/anl64_plan.hpp")=="157be15c63363ba2d55093af829ca68be9107e27"
assert blob("src/q2_benchmark.cpp")=="ea1e986e22f6921e7f6c52a4fa5935121cfec663"
assert blob("tools/compile_q2_shaders.ps1")=="a7e08a196ee6c6bf94f460ef4d710175a1b85166"
assert blob("tools/build_q2.ps1")=="18325b0a2b3c319f304724397cfe8dfb6d17377f"
assert blob("shaders/p7_q4k_gemm_2d.comp")=="fb1fb14192ff7d275a4af38c6dd9be7d1b500a7a"
assert blob("shaders/p7_q6k_gemm_2d.comp")=="a0de99f972db6cd202606aad95fb9eab223639e6"
assert blob("shaders/sa1_q4k_subgroup_splitk.comp")=="56999d88dc1bef6486e7e1908982f6de4b0f9f6a"

adj=txt("tools/adjudicate_anl64_p6.py")
for x in [
    'dr>=1.10','er<=0.90','tr<=1.10',
    'statistics.median',
    'rs==cs','len(rs)==32',
    'plan.get("nodes")==469',
    'plan.get("fixed_q4_fast_nodes")==140',
    'plan.get("regions")==24104',
    'plan.get("fixed_q4_regions")==19936',
    '"p5_timings_used":False',
    '"llama_cpp_advantage_claimed":False',
]:
    assert x in adj,x
assert "P6_MATCHED_E2E_PASS" in adj
assert "P6_NO_MATERIAL_E2E_BENEFIT" in adj
assert "P6_SEMANTIC_OR_STRUCTURAL_FAIL" in adj

run=txt("run_anl64_p6_confirmatory.ps1")
assert 'config\\anl64_p6_execution_lock_v0.1.json' in run
assert 'P6_MATCHED_E2E_EXECUTION_AUTHORIZED' in run
assert 'results\\anl64_p6_confirmatory' in run
assert 'results\\anl64_p5_integration' not in run
assert "P5 timings are forbidden and are not read." in run
assert run.count('powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Self -InternalSession')==2
assert '-Session A' in run and '-Session B' in run
assert run.index('-Session A') < run.index('-Session B')
assert run.count('--warmups 1 --measured 5')==1  # shared Invoke-Cell implementation
assert "reference native build failed" in run
assert run.index("P6 preflight BuildOnly: exact safe reference") < run.index("P6 Session A in separate child process")
assert run.index("P6 Session A in separate child process") < run.index("P6 Session B in separate child process")
assert "reference executable changed after preflight" in run
assert "sessions did not use distinct runner processes" in run
assert "P7_NOT_AUTHORIZED" in run

# Exact counterbalanced orders appear once in child branches.
order_a='@("ARC_SAFE_REFERENCE/W-S","ANL64_P4_LOCKED/W-S","ANL64_P4_LOCKED/W-C","ARC_SAFE_REFERENCE/W-C")'
order_b='@("ANL64_P4_LOCKED/W-S","ARC_SAFE_REFERENCE/W-S","ARC_SAFE_REFERENCE/W-C","ANL64_P4_LOCKED/W-C")'
assert order_a in run
assert order_b in run

# No performance source mutation or external comparison sneaks into the runner.
assert "q2_llama_adapter" not in run\nassert "artifacts\\\\q2_baseline" not in run
assert "q2_resource_sampler" not in run
assert "summarize_q2" not in run

print("ANL64 P6 runner static package: PASS")
