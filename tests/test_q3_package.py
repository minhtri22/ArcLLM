from pathlib import Path
import json, re
ROOT=Path(__file__).resolve().parents[1]
def txt(p): return (ROOT/p).read_text(encoding="utf-8")
def js(p): return json.loads(txt(p))

required=[
 "docs/Q3_NO_PRACTICAL_ADVANTAGE_CONFIRMATORY_DESIGN.md","docs/Q3_IMPLEMENTATION.md",
 "config/q3_confirmatory_design.json","run_q3.ps1","run_q3_preflight.ps1","package_q3.ps1",
 "tools/adjudicate_q3.py","tools/package_q3_evidence.py",
 "config/q2_execution_authorization.json","inputs/q2_formal_result.authoritative.json"
]
for p in required: assert (ROOT/p).is_file(),p

d=js("config/q3_confirmatory_design.json")
assert d["schema"]=="arcllm.q3.no_practical_advantage_confirmatory_design.v1"
assert d["status"]=="DESIGN_FROZEN_EXECUTION_CLOSED"
assert d["frozen_scope"]["workloads"]==["W-S","W-C"]
assert d["frozen_scope"]["arcllm_architecture_change_permitted"] is False
assert d["frozen_scope"]["kernel_tuning_permitted"] is False
assert d["frozen_scope"]["workload_search_permitted"] is False
assert d["frozen_scope"]["nexus_input_permitted"] is False
fr=d["fresh_reproduction"]
assert fr["sessions"]==2 and fr["total_measured_attempts"]==40
assert fr["session_A_order"]==["ArcLLM/W-S","llama.cpp/W-S","llama.cpp/W-C","ArcLLM/W-C"]
assert fr["session_B_order"]==["llama.cpp/W-S","ArcLLM/W-S","ArcLLM/W-C","llama.cpp/W-C"]
t=d["practical_effect_thresholds"]
assert t=={"speed_or_latency_minimum_relative_improvement":0.1,"working_set_minimum_relative_improvement":0.15,"maximum_allowed_blocking_harm":0.1}

q2=js("inputs/q2_formal_result.authoritative.json")
assert q2["classification"]=="Q2_MATCHED_CHARACTERIZATION_COMPLETE" and q2["successful_attempts"]==20
q2auth=js("config/q2_execution_authorization.json")
assert q2auth["preflight_implementation_commit"]=="43afd71161c4dc8c766c09c3b55d5eca48352bde"

runner=txt("run_q3.ps1")
assert '[ValidateSet("A","B")]' in runner
assert 'q3_session_"+$Session' in runner and "already exists; frozen sessions cannot be silently rerun" in runner
assert '$Order=@("arcllm_ws","baseline_ws","baseline_wc","arcllm_wc")' in runner
assert '$Order=@("baseline_ws","arcllm_ws","arcllm_wc","baseline_wc")' in runner
assert '"--warmups","1","--measured","5"' in runner
assert "q2_resource_sampler.py" in runner and "q2_gpu_sampler.ps1" in runner
assert "q3_execution_authorization.json" in runner and "Q3_EXECUTION_AUTHORIZED" in runner
assert "merge-base --is-ancestor" in runner
assert "critical file drift" in runner and "frozen Q2 runtime path changed" in runner
assert "& py @Args | Out-Host" in runner and "return [int]$Code" in runner
assert "runner_process_id=$PID" in runner
assert "execution_authorization_sha256=$AuthorizationHash" in runner
assert "authorization Q2-runtime binding mismatch" in runner and "authorization compiled-shader binding mismatch" in runner
assert runner.index("New-Item -ItemType Directory -Path $SessionDir") > runner.index("Q3 power scheme drift since preflight")
assert "compile_q2_shaders.ps1" not in runner and "build_q2.ps1" not in runner and "qualify_q2_baseline.ps1" not in runner
assert "NEXUS" not in runner

pre=txt("run_q3_preflight.ps1")
assert "q3_execution_authorization.json" in pre and "requires execution authorization to remain absent" in pre
assert '("q3_session_"+$S)' in pre
assert "Q3_IMPLEMENTATION_STATIC_LOCKED" in pre
assert "GetSystemPowerStatus" in pre and "ACLineStatus=1" in pre
assert "config\\q2_execution_authorization.json" in pre
assert "architecture/instrumentation changed since Q2" in pre
assert "tests\\test_q3_package.py" in pre
assert "compile_q2_shaders.ps1" in pre and "build_q2.ps1" in pre and "qualify_q2_baseline.ps1" in pre
assert "--qualify-only" in pre
assert "decode_executed" in pre and "measured_attempts -ne 0" in pre
assert "q3_preflight_lock.json" in pre and "q3_preflight_return_to_chatgpt.zip" in pre
assert "q3_execution_authorized=$false" in pre and "measurements_executed=$false" in pre and "measured_attempts=0" in pre
assert "run_q3.ps1" in pre and "Invoke-Q3Cell" not in pre
assert "NEXUS" not in pre

adj=txt("tools/adjudicate_q3.py")
for s in ["REGIME_ADVANTAGE_SUPPORTED","FEASIBLE_NO_DEMONSTRATED_ADVANTAGE","UNRESOLVED"]:
    assert s in adj
assert '("TTFT","DECODE","E2E","WORKING_SET")' in adj
assert "private_bytes_can_establish_advantage" in adj and "False" in adj
assert "cpu_utilization_can_establish_advantage" in adj and "gpu_counter_can_establish_advantage" in adj
assert "runner_process_id" in adj and "sessions_not_separate_runner_processes" in adj
assert "complete_hash_mismatch" in adj and "authorization_hash_binding" in adj
assert "baseline_runtime_config" in adj and "arcllm_runtime_config" in adj and 'warm.get("success") is not True' in adj
assert 'session_eval["A"][wl]["dimension_pass"][dim] and session_eval["B"][wl]["dimension_pass"][dim]' in adj
assert 'len(attempts)!=5' in adj and 'len(summaries)!=5' in adj
assert "reproduced_primary_advantages" in adj

pkg=txt("tools/package_q3_evidence.py")
assert "q3.evidence_manifest.v1" in pkg and "expected_measured_attempts" in pkg
assert '("session_A",Path(a.session_a))' in pkg and '("session_B",Path(a.session_b))' in pkg
assert "q3_adjudication_candidate.json" in pkg and "q3_preflight_lock.json" in pkg and "q3_execution_authorization.json" in pkg
packps=txt("package_q3.ps1")
assert "q3_session_A" in packps and "q3_session_B" in packps and "q3_return_to_chatgpt.zip" in packps
assert "adjudicate_q3.py" in packps and "package_q3_evidence.py" in packps

m=js("manifest.json")
assert m["phase"]=="Q3-NO-PRACTICAL-ADVANTAGE-CONFIRMATORY"
assert m["q3"]["total_fresh_measured_attempts"]==40
assert m["q3"]["arcllm_tuning_permitted"] is False and m["q3"]["nexus_input_permitted"] is False
auth=ROOT/"config/q3_execution_authorization.json"
if m["q3"]["execution_permitted"]:
    assert m["status"]=="Q3_EXECUTION_AUTHORIZED" and m["q3"]["status"]=="Q3_EXECUTION_AUTHORIZED"
    assert m["q3_permitted"] is True and m["target_run_permitted"] is True and auth.is_file()
    a=js("config/q3_execution_authorization.json")
    assert a["schema"]=="arcllm.q3.execution_authorization.v1" and a["authorized"] is True
    assert a["decision"]=="Q3_EXECUTION_AUTHORIZED" and a["measurements_executed"] is False and a["measured_attempts"]==0
else:
    assert m["status"]=="Q3_IMPLEMENTATION_STATIC_LOCKED" and m["q3"]["status"]=="Q3_IMPLEMENTATION_STATIC_LOCKED"
    assert m["q3_permitted"] is False and m["target_run_permitted"] is False and not auth.exists()

print("ArcLLM Q3 static package: PASS")
