from pathlib import Path
import ast,json,subprocess,sys,tempfile

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"src/arcllm_v1_q4_down_4arm_counter_collection.cpp"
RT=ROOT/"src/arcllm_v1_q4_down_4arm_counter_runtime.cpp"
SHIM=ROOT/"src/arcllm_v1_m3_counter_shim.cpp"
BUILD=ROOT/"tools/build_arcllm_v1_q4_down_4arm_native_counter.ps1"
PARSER=ROOT/"tools/adjudicate_arcllm_v1_q4_down_4arm_native_counters.py"
TIMING=ROOT/"src/arcllm_v1_q4_down_4arm_primary_timing.cpp"
AUTH=ROOT/"config/arcllm_v1_q4_down_4arm_native_counter_campaign_authorization_v0.1.json"
for p in [SRC,RT,SHIM,BUILD,PARSER,TIMING,AUTH]: assert p.is_file(),p

a=json.loads(AUTH.read_text(encoding="utf-8"))
assert a["status"]=="PREREGISTERED_NATIVE_COUNTER_IMPLEMENTATION_AUTHORIZED_EXECUTION_LOCK_REQUIRED"
assert a["authorization"]["fresh_counter_execution"] is False
assert a["authorization"]["token_xray"] is False and a["authorization"]["child_c"] is False
fc=a["frozen_campaign"]
assert fc["decode_index"]==15
assert [x["index"] for x in fc["selected_native_counters"]]==[56,66,64,233]
assert fc["total_arm_workload_probes"]==8

s=SRC.read_text(encoding="utf-8")
r=RT.read_text(encoding="utf-8")
shim=SHIM.read_text(encoding="utf-8")
t=TIMING.read_text(encoding="utf-8")
for x in [
  "arcllm.v1.q4_down.4arm.native_counter_probe.v0.1",
  "selected_counters={56u,66u,64u,233u}",
  "di==15u",
  "execute_counter_profiled_targeted",
  "target dispatch census != 14",
  "restored_bytes_per_pass==sizeof(uint32_t)",
  "INSTRUMENTED_DIAGNOSTIC_ONLY",
  "primary_timing_rerun",
]:
    assert x in s,x
for x in [
  "execute_counter_profiled_targeted",
  "m3_perf_acquire_lock",
  "m3_perf_submit_pass",
  "cmd_reset_query_pool_(rcb,perf_qp,0,query_count)",
  "if(qi>=0)m3_perf_cmd_begin_query",
  "if(qi>=0){",
  "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT",
]:
    assert x in r,x
# Submit-pass contract crosses the runtime/shim boundary: runtime calls the proven shim; shim binds VkPerformanceQuerySubmitInfoKHR.counterPassIndex.
assert "m3_perf_submit_pass" in r
assert "VkPerformanceQuerySubmitInfoKHR" in shim
assert "counterPassIndex" in shim
# Proven M3-B multipass core remains present; targeted method adds only target query mapping.
assert r.count("m3_perf_acquire_lock")>=2
assert r.count("m3_perf_submit_pass")>=2
# Exact 4-arm mechanism routing and decode graph must stay byte-identical to frozen timing harness.
def sec(x,a,b):
    i=x.index(a);j=x.index(b,i);return x[i:j]
assert sec(s,"auto add_decode_down=[&]","auto build_decode=[&]")==sec(t,"auto add_decode_down=[&]","auto build_decode=[&]")
assert sec(s,"auto build_decode=[&]","auto ppops=build_prefill(seq)")==sec(t,"auto build_decode=[&]","auto ppops=build_prefill(seq)")
assert "execute_targeted_profiled" not in s
assert "component_latency_ms_per_token" not in s
assert "token_xray::" not in s

b=BUILD.read_text(encoding="utf-8")
for x in ["arcllm_v1_q4_down_4arm_counter_collection.cpp","arcllm_v1_m3_counter_shim.cpp","vulkan-1.lib"]:
    assert x in b,x

# Parser syntax + exact synthetic 8-probe regression.
ptxt=PARSER.read_text(encoding="utf-8");compile(ptxt,str(PARSER),"exec")
ORDER=["0","A","B","AB"]; WS=["W-S","W-C"]
NAMES={56:"GPU_MEMORY_BYTE_READ",66:"XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION",64:"XVE_STALL",233:"XVE_STALL_SBID"}
LAYERS=[3,4,6,7,8,11,12,14,15,17,18,19,21,22]
SH={"0":("p7_q4k_gemm_2d.spv",56),"A":("sa1_q4k_subgroup_splitk.spv",896),"B":("q4_down_exec148_serial.spv",56),"AB":("q4_down_exec148_splitk32.spv",896)}
EH={"W-S":"f31d4bb9fe5eb9c3","W-C":"471519ddc45b232e"}
vals={
 "0":{56:100.0,66:10.0,64:60.0,233:50.0},
 "A":{56:95.0,66:80.0,64:10.0,233:8.0},
 "B":{56:40.0,66:12.0,64:55.0,233:45.0},
 "AB":{56:35.0,66:70.0,64:15.0,233:12.0},
}
def fake(w,a):
    sh,wg=SH[a]
    ds=[]
    for qi,l in enumerate(LAYERS):
        ds.append({"query_id":qi,"full_dispatch_id":qi,"runtime_name":f"L{l:02d}.ffn_down","shader":sh,"workgroups":[wg,1,1],
                   "counters":[{"index":i,"name":NAMES[i],"storage":5,"value":vals[a][i]} for i in [56,66,64,233]]})
    return {
      "schema":"arcllm.v1.q4_down.4arm.native_counter_probe.v0.1","status":"PASS_NATIVE_COUNTER_PROBE","workload":w,"arm":a,
      "counter_probe":{"decode_index":15,"scope":"COMMAND","pass_count":2,"pass_semantics":"COMBINED_AFTER_REQUIRED_PASSES",
                       "target_dispatches":14,"full_graph_dispatches":469,"restored_bytes_per_pass":4,
                       "quiet_host_required":True,"timing_use":"INSTRUMENTED_DIAGNOSTIC_ONLY"},
      "counter_meta":[{"index":i,"name":NAMES[i]} for i in [56,66,64,233]],
      "dispatches":ds,
      "semantic_guard":{"warmup_success":True,"measured_success":True,"dispatch_census_pass":True,
                        "expected_generated_hash_fnv1a64":EH[w],"generated_hash_fnv1a64":EH[w]},
      "exec148":{"tuple_exact":True,"family_source_sha256":"x","family_exec_sha256":"x"},
      "governance":{"counter_evidence_only":True,"primary_timing_substitution_forbidden":True,
                    "primary_timing_rerun":False,"token_xray":False,"child_c":False}
    }
with tempfile.TemporaryDirectory() as td0:
    td=Path(td0)
    for w in WS:
        for a in ORDER:(td/f"Q4_COUNTER_{w.replace('-','_')}_{a}.json").write_text(json.dumps(fake(w,a)),encoding="utf-8")
    timing={"status":"PASS_PRIMARY_TIMING_EVIDENCE_FROZEN","timing_stage_adjudication":{
      "A_latency_condition":True,"B_latency_condition":True,"composition_supported":False,
      "interaction_classification":"ANTAGONISTIC_INTERACTION"}}
    tp=td/"timing.json";tp.write_text(json.dumps(timing),encoding="utf-8")
    out=td/"out.json"
    cp=subprocess.run([sys.executable,str(PARSER),"--results-dir",str(td),"--timing-canonical",str(tp),"--out",str(out)],capture_output=True,text=True)
    assert cp.returncode==0,(cp.stdout,cp.stderr)
    d=json.loads(out.read_text(encoding="utf-8"))
    assert d["status"]=="PASS_NATIVE_4_COUNTER_RECOMPUTE"
    assert d["preregistered_claims"]["A_supported"] is True
    assert d["preregistered_claims"]["B_supported"] is True

print("Q4_DOWN_4ARM_NATIVE_COUNTER_STATIC_QA=PASS")
