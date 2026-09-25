from pathlib import Path
import json,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1]
AUTH=ROOT/"config/arcllm_v1_q4_down_4arm_primary_timing_authorization_v0.1.json"
CAN=ROOT/"artifacts/ARCLLM_V1/ARCLLM_V1_Q4_DOWN_4ARM_CORRECTNESS_CANONICAL_v0.1.json"
CORR=ROOT/"src/arcllm_v1_q4_down_arch_4arm.cpp"
SRC=ROOT/"src/arcllm_v1_q4_down_4arm_primary_timing.cpp"
RT=ROOT/"src/arcllm_v1_q4_down_4arm_timing_runtime.cpp"
ADJ=ROOT/"tools/adjudicate_arcllm_v1_q4_down_4arm_primary_timing.py"
for p in [AUTH,CAN,CORR,SRC,RT,ADJ]: assert p.is_file(),p
a=json.loads(AUTH.read_text(encoding="utf-8"));c=json.loads(CAN.read_text(encoding="utf-8"))
assert a["status"]=="PRIMARY_4_ARM_TIMING_AUTHORIZED_UNDER_FROZEN_PREREGISTRATION"
assert c["status"]=="PASS_CANONICAL_CORRECTNESS_EVIDENCE_FROZEN"
assert a["authorization"]["native_hardware_counter_campaign"] is False
assert a["authorization"]["token_xray"] is False and a["authorization"]["child_c"] is False
assert a["primary_timing_campaign"]["measured_blocks_per_workload"]==8
ORDER=a["primary_timing_campaign"]["arm_order_by_block"]
assert ORDER==[["0","A","B","AB"],["A","B","AB","0"],["B","AB","0","A"],["AB","0","A","B"],["AB","B","A","0"],["0","AB","B","A"],["A","0","AB","B"],["B","A","0","AB"]]
s=SRC.read_text(encoding="utf-8");r=RT.read_text(encoding="utf-8")
for x in ["execute_targeted_profiled","target dispatch census != 14","q4_down_ms_by_decode","component_latency_ms_per_token",
          "materialization_time_ms","validation_time_ms","CANONICAL_REFERENCE","INTEGRITY_REPLICATE_ONLY",
          "std::array<std::array<Q4Arm,4>,8>","attempts.reserve(32u)"]:
    assert x in s,x
for x in ["timestampPeriod","timestamp_period_ns","VkPhysicalDevicePropertiesTimingCompat","execute_targeted_profiled"]:
    assert x in r,x
for forbidden in ["VK_KHR_performance_query","vkAcquireProfilingLockKHR","Token-XRay","CHILD_C"]:
    assert forbidden not in s and forbidden not in r,forbidden
# Mechanism-defining decode routing must be byte-identical to correctness-qualified source.
def section(t,a,b):
    i=t.index(a);j=t.index(b,i);return t[i:j]
corr=CORR.read_text(encoding="utf-8")
assert section(s,"auto add_decode_down=[&]","auto build_decode=[&]")==section(corr,"auto add_decode_down=[&]","auto build_decode=[&]")
assert section(s,"auto build_decode=[&]","auto ppops=build_prefill(seq)")==section(corr,"auto build_decode=[&]","auto ppops=build_prefill(seq)")
# Synthetic analysis regression: exact frozen order, ratios, no science execution.
def make(w):
    expected={"W-S":"f31d4bb9fe5eb9c3","W-C":"471519ddc45b232e"}[w]
    attempts=[]
    vals={"0":10.0,"A":8.0,"B":9.0,"AB":7.0}
    for b,row in enumerate(ORDER):
        for o,arm in enumerate(row):
            q=[vals[arm]]*31
            attempts.append({"block":b,"ordinal":o,"arm":arm,"profiled":True,"success":True,"dispatch_census_pass":True,
              "generated_hash_fnv1a64":expected,"component_latency_ms_per_token":vals[arm],
              "timestamp_valid_bits":64,"timestamp_period_ns":1.0,"q4_down_ms_by_decode":q,"error":""})
    return {"schema":"arcllm.v1.q4_down.4arm.primary_timing.collection.v0.1","status":"PASS_PRIMARY_TIMING_COLLECTION","workload":w,
      "measurement":{"primary_metric":"Q4_DOWN_COMPONENT_LATENCY_MS_PER_TOKEN","target_dispatches_per_decode":14,"decode_samples_per_attempt":31,
                     "hardware_counters_executed":False,"token_xray_executed":False},
      "correctness_recheck":{"expected_generated_hash_fnv1a64":expected},
      "architecture_cost":{"role":"CANONICAL_REFERENCE" if w=="W-S" else "INTEGRITY_REPLICATE_ONLY","materialization_time_ms":100.0,"validation_time_ms":10.0},
      "attempts":attempts}
with tempfile.TemporaryDirectory() as td0:
    td=Path(td0);ws=td/"ws.json";wc=td/"wc.json";out=td/"out.json"
    ws.write_text(json.dumps(make("W-S")),encoding="utf-8");wc.write_text(json.dumps(make("W-C")),encoding="utf-8")
    cp=subprocess.run([sys.executable,str(ADJ),"--w-s",str(ws),"--w-c",str(wc),"--out",str(out)],capture_output=True,text=True)
    assert cp.returncode==0,(cp.stdout,cp.stderr)
    d=json.loads(out.read_text(encoding="utf-8"))
    assert d["status"]=="PASS_PRIMARY_TIMING_RECOMPUTE"
    assert abs(d["workloads"]["W-S"]["cell_medians_ms"]["AB"]-7.0)<1e-12
    assert d["claim_boundary"]["mechanism_supported_A_B_claims_allowed"] is False
print("Q4_DOWN_4ARM_PRIMARY_TIMING_STATIC_QA=PASS")
