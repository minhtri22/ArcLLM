import argparse,json,math
from pathlib import Path

ARMS=["0","A","B","AB"]
WORKLOADS=["W-S","W-C"]
COUNTERS=[56,66,64,233]
NAMES={
  56:"GPU_MEMORY_BYTE_READ",
  66:"XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION",
  64:"XVE_STALL",
  233:"XVE_STALL_SBID",
}
EXPECTED_HASH={"W-S":"f31d4bb9fe5eb9c3","W-C":"471519ddc45b232e"}
LAYERS=[3,4,6,7,8,11,12,14,15,17,18,19,21,22]
SOURCE_WEIGHT_BYTES=534675456
ARM_SHADER={
  "0":("p7_q4k_gemm_2d.spv",56),
  "A":("sa1_q4k_subgroup_splitk.spv",896),
  "B":("q4_down_exec148_serial.spv",56),
  "AB":("q4_down_exec148_splitk32.spv",896),
}

def req(c,m):
    if not c: raise SystemExit("Q4 native counter adjudication STOP: "+m)

def load_one(path,w,a):
    d=json.loads(Path(path).read_text(encoding="utf-8"))
    req(d.get("schema")=="arcllm.v1.q4_down.4arm.native_counter_probe.v0.1",f"{w}/{a} schema")
    req(d.get("status")=="PASS_NATIVE_COUNTER_PROBE",f"{w}/{a} status")
    req(d.get("workload")==w and d.get("arm")==a,f"{w}/{a} identity")
    p=d.get("counter_probe",{})
    req(p.get("decode_index")==15 and p.get("scope")=="COMMAND",f"{w}/{a} probe identity")
    req(p.get("pass_count",0)>=1,f"{w}/{a} pass_count")
    req(p.get("pass_semantics")=="COMBINED_AFTER_REQUIRED_PASSES",f"{w}/{a} pass semantics")
    req(p.get("target_dispatches")==14 and p.get("full_graph_dispatches")==469,f"{w}/{a} dispatch scope")
    req(p.get("restored_bytes_per_pass")==4,f"{w}/{a} restore bytes")
    req(p.get("quiet_host_required") is True,f"{w}/{a} quiet host")
    req(p.get("timing_use")=="INSTRUMENTED_DIAGNOSTIC_ONLY",f"{w}/{a} timing misuse")
    meta=d.get("counter_meta",[])
    req([int(x["index"]) for x in meta]==COUNTERS,f"{w}/{a} counter indices")
    req([x["name"] for x in meta]==[NAMES[i] for i in COUNTERS],f"{w}/{a} counter names")
    ds=d.get("dispatches",[])
    req(len(ds)==14,f"{w}/{a} target dispatch count")
    exp_names=[f"L{l:02d}.ffn_down" for l in LAYERS]
    req([x["runtime_name"] for x in ds]==exp_names,f"{w}/{a} target names")
    sh,wg=ARM_SHADER[a]
    for q,x in enumerate(ds):
        req(int(x.get("query_id",-1))==q,f"{w}/{a} query order")
        req(x.get("shader")==sh,f"{w}/{a} shader")
        req(x.get("workgroups")==[wg,1,1],f"{w}/{a} workgroups")
        cs=x.get("counters",[])
        req([int(c["index"]) for c in cs]==COUNTERS,f"{w}/{a} dispatch counter indices")
        for c in cs:
            v=float(c["value"]);req(math.isfinite(v),f"{w}/{a} finite counter")
    sg=d.get("semantic_guard",{})
    req(sg.get("warmup_success") is True and sg.get("measured_success") is True and sg.get("dispatch_census_pass") is True,f"{w}/{a} semantic guard")
    req(sg.get("expected_generated_hash_fnv1a64")==EXPECTED_HASH[w] and sg.get("generated_hash_fnv1a64")==EXPECTED_HASH[w],f"{w}/{a} generated hash")
    ex=d.get("exec148",{})
    req(ex.get("tuple_exact") is True and ex.get("family_source_sha256")==ex.get("family_exec_sha256"),f"{w}/{a} EXEC148")
    g=d.get("governance",{})
    req(g.get("counter_evidence_only") is True and g.get("primary_timing_substitution_forbidden") is True,f"{w}/{a} governance")
    req(g.get("primary_timing_rerun") is False and g.get("token_xray") is False and g.get("child_c") is False,f"{w}/{a} forbidden scope")
    return d

def value_map(d):
    vals={i:[] for i in COUNTERS}
    for x in d["dispatches"]:
        for c in x["counters"]: vals[int(c["index"])].append(float(c["value"]))
    return {
      "device_read_over_weight":sum(vals[56])/SOURCE_WEIGHT_BYTES,
      "alu1_util_mean":sum(vals[66])/14.0,
      "xve_stall_mean":sum(vals[64])/14.0,
      "sbid_stall_mean":sum(vals[233])/14.0,
      "raw_sum_gpu_memory_byte_read":sum(vals[56]),
    }

ap=argparse.ArgumentParser()
ap.add_argument("--results-dir",required=True)
ap.add_argument("--timing-canonical",required=True)
ap.add_argument("--out",required=True)
args=ap.parse_args()
td=json.loads(Path(args.timing_canonical).read_text(encoding="utf-8"))
req(td.get("status")=="PASS_PRIMARY_TIMING_EVIDENCE_FROZEN","timing canonical status")
req(td["timing_stage_adjudication"]["A_latency_condition"] is True,"A timing condition")
req(td["timing_stage_adjudication"]["B_latency_condition"] is True,"B timing condition")
req(td["timing_stage_adjudication"]["composition_supported"] is False,"composition timing freeze")
req(td["timing_stage_adjudication"]["interaction_classification"]=="ANTAGONISTIC_INTERACTION","interaction freeze")

root=Path(args.results_dir)
cells={}
raw={}
for w in WORKLOADS:
    cells[w]={};raw[w]={}
    stem=w.replace("-","_")
    for a in ARMS:
        p=root/f"Q4_COUNTER_{stem}_{a}.json"
        d=load_one(p,w,a);raw[w][a]=d;cells[w][a]=value_map(d)

direction={}
for w in WORKLOADS:
    c=cells[w]
    direction[w]={
      "A_alu1_up":c["A"]["alu1_util_mean"]>c["0"]["alu1_util_mean"],
      "A_xve_stall_down":c["A"]["xve_stall_mean"]<c["0"]["xve_stall_mean"],
      "A_sbid_down":c["A"]["sbid_stall_mean"]<c["0"]["sbid_stall_mean"],
      "B_device_read_over_weight_down":c["B"]["device_read_over_weight"]<c["0"]["device_read_over_weight"],
    }
A_counter=all(direction[w]["A_alu1_up"] and direction[w]["A_xve_stall_down"] and direction[w]["A_sbid_down"] for w in WORKLOADS)
B_counter=all(direction[w]["B_device_read_over_weight_down"] for w in WORKLOADS)

out={
 "schema":"arcllm.v1.q4_down.4arm.native_counter.adjudication.v0.1",
 "status":"PASS_NATIVE_4_COUNTER_RECOMPUTE",
 "provider":"VULKAN_KHR_PERFORMANCE_QUERY",
 "scope":"COMMAND",
 "decode_index":15,
 "counter_indices":COUNTERS,
 "target_dispatches_per_probe":14,
 "arm_workload_probes":8,
 "cells":cells,
 "directional_conditions":direction,
 "preregistered_claims":{
   "A_latency_condition_from_frozen_timing":True,
   "A_counter_condition":A_counter,
   "A_supported":A_counter,
   "B_latency_condition_from_frozen_timing":True,
   "B_counter_condition":B_counter,
   "B_supported":B_counter,
   "composition_supported":False,
   "interaction_classification":"ANTAGONISTIC_INTERACTION"
 },
 "claim_boundary":{
   "counter_timing_primary":False,
   "primary_timing_rerun":False,
   "token_xray":False,
   "child_c":False,
   "new_mechanism":False
 }
}
Path(args.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print("Q4_DOWN_4ARM_NATIVE_COUNTER_RECOMPUTE=PASS")
