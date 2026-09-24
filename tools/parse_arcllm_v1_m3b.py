import argparse,json,math
from pathlib import Path

SCHEMA="arcllm.v1.m3b.command_scope_counter_collection.v0.1"
MODEL_SHA="60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
PROFILE_ID="intel.core_ultra_7_258v.arc_140v.devhost_32gib.v0.1"
EXPECTED_HASH={"W-S":"f31d4bb9fe5eb9c3","W-C":"471519ddc45b232e"}
GROUPS={
 "memory_cache":[0,2,6,30,31,52,53,56,57,58,59,71,72,73,74,75,102,110,111,112,117,120],
 "execution_occupancy":[0,1,2,4,6,39,40,41,42,54,55,64,65,66,68,172,173,174,175,176,177,208,212,215,216,218,242,248],
 "stall_cause":[0,31,49,64,73,120,127,129,226,227,228,229,230,231,233,234],
}
INDEX_NAME={
 0:"GpuTime",1:"GpuCoreClocks",2:"AvgGpuCoreFrequencyMHz",4:"GPU_BUSY",6:"GPGPU_THREADGROUP_COUNT",
 30:"TLB_MISS",31:"GPU_MEMORY_REQUEST_QUEUE_FULL",39:"XVE_ACTIVE",40:"XVE_INST_EXECUTED_ALU0_ALL",
 41:"XVE_INST_EXECUTED_ALU1_ALL",42:"XVE_INST_EXECUTED_SEND_ALL",49:"XVE_SHARED_FUNCTION_ACCESS_HOLD",
 52:"LOAD_STORE_CACHE_HIT",53:"LOAD_STORE_CACHE_ACCESS",54:"XVE_THREADS_OCCUPANCY_ALL",55:"XVE_INST_ISSUED_ALL",
 56:"GPU_MEMORY_BYTE_READ",57:"GPU_MEMORY_BYTE_WRITE",58:"LOAD_STORE_CACHE_BYTE_READ",59:"LOAD_STORE_CACHE_BYTE_WRITE",
 64:"XVE_STALL",65:"XVE_INST_EXECUTED_ALU0_ALL_UTILIZATION",66:"XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION",
 68:"XVE_MULTIPLE_PIPE_ACTIVE",71:"L3_HIT",72:"L3_MISS",73:"L3_STALL",74:"GPU_MEMORY_L3_READ",
 75:"GPU_MEMORY_L3_WRITE",102:"GPU_MEMORY_ACTIVE",110:"LOAD_STORE_CACHE_L3_HIT",111:"LOAD_STORE_CACHE_L3_READ",
 112:"LOAD_STORE_CACHE_L3_WRITE",117:"L3_BUSY",120:"L3_SUPERQ_FULL",127:"LOAD_STORE_CACHE_INPUT_AVAILABLE",
 129:"LOAD_STORE_CACHE_OUTPUT_READY",172:"THREAD_DISPATCH_QUEUE0_ACTIVE",173:"THREAD_DISPATCH_QUEUE0_STALL",
 174:"THREAD_DISPATCH_QUEUE1_ACTIVE",175:"THREAD_DISPATCH_QUEUE1_STALL",
 176:"THREADGROUP_DISPATCH_QUEUE0_RESOURCE_STALL",177:"THREADGROUP_DISPATCH_QUEUE1_RESOURCE_STALL",
 208:"XVE_INST_EXECUTED_BARRIER",212:"XVE_INST_EXECUTED_FP32",215:"XVE_INST_EXECUTED_INT16",
 216:"XVE_INST_EXECUTED_INT32",218:"XVE_INST_EXECUTED_MATH",226:"XVE_STALL_ALUWR",
 227:"XVE_STALL_BARRIER",228:"XVE_STALL_CONTROL",229:"XVE_STALL_INSTFETCH",230:"XVE_STALL_OTHER",
 231:"XVE_STALL_PIPESTALL",233:"XVE_STALL_SBID",234:"XVE_STALL_SENDWR",
 242:"XVE_INST_EXECUTED_ALU0_CS_UTILIZATION",248:"XVE_INST_EXECUTED_ALU1_CS_UTILIZATION",
}
NORMALIZED={
 "GpuTime":"gpu_time_ns","GpuCoreClocks":"gpu_core_clocks","AvgGpuCoreFrequencyMHz":"gpu_core_frequency_hz",
 "GPU_BUSY":"gpu_busy_percent","GPGPU_THREADGROUP_COUNT":"threadgroup_count","TLB_MISS":"tlb_miss_count",
 "GPU_MEMORY_REQUEST_QUEUE_FULL":"gpu_memory_request_queue_full_percent","XVE_ACTIVE":"xve_active_percent",
 "XVE_INST_EXECUTED_ALU0_ALL":"xve_alu0_slots","XVE_INST_EXECUTED_ALU1_ALL":"xve_alu1_slots",
 "XVE_INST_EXECUTED_SEND_ALL":"xve_send_instruction_count","XVE_SHARED_FUNCTION_ACCESS_HOLD":"xve_shared_function_hold_percent",
 "LOAD_STORE_CACHE_HIT":"lsc_hit_count","LOAD_STORE_CACHE_ACCESS":"lsc_access_count",
 "XVE_THREADS_OCCUPANCY_ALL":"occupancy_percent","XVE_INST_ISSUED_ALL":"xve_instruction_issued_count",
 "GPU_MEMORY_BYTE_READ":"dram_read_bytes","GPU_MEMORY_BYTE_WRITE":"dram_write_bytes",
 "LOAD_STORE_CACHE_BYTE_READ":"lsc_read_bytes","LOAD_STORE_CACHE_BYTE_WRITE":"lsc_write_bytes",
 "XVE_STALL":"xve_stall_percent","XVE_INST_EXECUTED_ALU0_ALL_UTILIZATION":"xve_alu0_utilization_percent",
 "XVE_INST_EXECUTED_ALU1_ALL_UTILIZATION":"xve_alu1_utilization_percent","XVE_MULTIPLE_PIPE_ACTIVE":"xve_multiple_pipe_active_percent",
 "L3_HIT":"l3_hit_count","L3_MISS":"l3_miss_count","L3_STALL":"l3_stall_percent",
 "GPU_MEMORY_L3_READ":"l3_miss_memory_read_count","GPU_MEMORY_L3_WRITE":"l3_memory_write_count",
 "GPU_MEMORY_ACTIVE":"gpu_memory_active_percent","LOAD_STORE_CACHE_L3_HIT":"lsc_l3_hit_count",
 "LOAD_STORE_CACHE_L3_READ":"lsc_l3_read_count","LOAD_STORE_CACHE_L3_WRITE":"lsc_l3_write_count",
 "L3_BUSY":"l3_busy_percent","L3_SUPERQ_FULL":"l3_superq_full_percent",
 "LOAD_STORE_CACHE_INPUT_AVAILABLE":"lsc_input_available_percent","LOAD_STORE_CACHE_OUTPUT_READY":"lsc_output_ready_percent",
 "THREAD_DISPATCH_QUEUE0_ACTIVE":"thread_dispatch_queue0_active_percent","THREAD_DISPATCH_QUEUE0_STALL":"thread_dispatch_queue0_stall_percent",
 "THREAD_DISPATCH_QUEUE1_ACTIVE":"thread_dispatch_queue1_active_percent","THREAD_DISPATCH_QUEUE1_STALL":"thread_dispatch_queue1_stall_percent",
 "THREADGROUP_DISPATCH_QUEUE0_RESOURCE_STALL":"threadgroup_queue0_resource_stall_percent",
 "THREADGROUP_DISPATCH_QUEUE1_RESOURCE_STALL":"threadgroup_queue1_resource_stall_percent",
 "XVE_INST_EXECUTED_BARRIER":"xve_barrier_instruction_count","XVE_INST_EXECUTED_FP32":"xve_fp32_slot_count",
 "XVE_INST_EXECUTED_INT16":"xve_int16_slot_count","XVE_INST_EXECUTED_INT32":"xve_int32_slot_count",
 "XVE_INST_EXECUTED_MATH":"xve_math_slot_count","XVE_STALL_ALUWR":"xve_stall_aluwr_percent",
 "XVE_STALL_BARRIER":"xve_stall_barrier_percent","XVE_STALL_CONTROL":"xve_stall_control_percent",
 "XVE_STALL_INSTFETCH":"xve_stall_instfetch_percent","XVE_STALL_OTHER":"xve_stall_other_percent",
 "XVE_STALL_PIPESTALL":"xve_stall_pipestall_percent","XVE_STALL_SBID":"xve_stall_sbid_percent",
 "XVE_STALL_SENDWR":"xve_stall_sendwr_percent","XVE_INST_EXECUTED_ALU0_CS_UTILIZATION":"xve_alu0_cs_utilization_percent",
 "XVE_INST_EXECUTED_ALU1_CS_UTILIZATION":"xve_alu1_cs_utilization_percent",
}
UNIT={0:"generic",1:"percentage",2:"nanoseconds",3:"bytes",4:"bytes_per_second",5:"kelvin",6:"watts",7:"volts",8:"amps",9:"hertz",10:"cycles"}
STORAGE={0:"INT32",1:"INT64",2:"UINT32",3:"UINT64",4:"FLOAT32",5:"FLOAT64"}

def aggregation(unit):
    return {"percentage":"PERCENT","bytes":"BYTES","bytes_per_second":"RATE"}.get(unit,"RAW")

def finite(v):
    return isinstance(v,(int,float)) and math.isfinite(float(v))

ap=argparse.ArgumentParser()
ap.add_argument("--results-dir",required=True)
ap.add_argument("--out-observations",required=True)
ap.add_argument("--out-summary",required=True)
a=ap.parse_args()
root=Path(a.results_dir)
errors=[]; observations=[]; runs=[]
for workload,stemw in [("W-S","W_S"),("W-C","W_C")]:
  for group in ["memory_cache","execution_occupancy","stall_cause"]:
    fn=f"M3B_{stemw}_{group}.json"; p=root/fn
    if not p.is_file(): errors.append(fn+":missing"); continue
    try: d=json.loads(p.read_text(encoding="utf-8"))
    except Exception as exc: errors.append(fn+":json:"+str(exc)); continue
    if d.get("schema")!=SCHEMA or d.get("status")!="PASS": errors.append(fn+":schema/status"); continue
    if d.get("workload")!=workload or d.get("counter_group")!=group: errors.append(fn+":identity")
    probe=d.get("counter_probe",{})
    if probe.get("decode_index")!=15 or probe.get("scope")!="COMMAND": errors.append(fn+":probe")
    if probe.get("pass_semantics")!="COMBINED_AFTER_REQUIRED_PASSES" or int(probe.get("pass_count",0))<1: errors.append(fn+":pass")
    if probe.get("logical_dispatches")!=469: errors.append(fn+":dispatch-census")
    if probe.get("physical_dispatch_executions")!=469*int(probe.get("pass_count",0)): errors.append(fn+":physical-dispatch-census")
    if probe.get("quiet_host_required") is not True or probe.get("timing_use")!="INSTRUMENTED_DIAGNOSTIC_ONLY": errors.append(fn+":semantics")
    guard=d.get("semantic_guard",{})
    if not guard.get("warmup_success") or not guard.get("measured_success") or not guard.get("dispatch_census_pass"): errors.append(fn+":semantic-pass")
    if guard.get("generated_hash_fnv1a64")!=EXPECTED_HASH[workload]: errors.append(fn+":generated-hash")

    meta=d.get("counter_meta",[])
    idx=[int(x.get("index",-1)) for x in meta]
    if idx!=GROUPS[group]: errors.append(fn+":counter-indices")
    meta_by={int(x["index"]):x for x in meta}
    for x in meta:
      if x.get("name")!=INDEX_NAME.get(int(x.get("index",-1))): errors.append(fn+":index-name-mismatch")
      if int(x.get("scope",-1))!=2: errors.append(fn+":non-command-scope")
      if (int(x.get("flags",0))&1)==0 or (int(x.get("flags",0))&2)==0: errors.append(fn+":counter-flags")
      if x.get("name") not in NORMALIZED: errors.append(fn+":unknown-native:"+str(x.get("name")))

    ds=d.get("dispatches",[])
    if len(ds)!=469: errors.append(fn+":dispatch-count"); continue
    ids=[x.get("dispatch_id") for x in ds]
    if ids!=list(range(469)): errors.append(fn+":dispatch-id-order")
    semantics=[x.get("semantic_node_id") for x in ds]
    if len(set(semantics))!=451 or semantics.count("decode.output.lm_head")!=19: errors.append(fn+":semantic-node-census")
    for disp in ds:
      vals=disp.get("counters",[])
      if len(vals)!=len(meta): errors.append(fn+f":counter-count-d{disp.get('dispatch_id')}"); continue
      if [int(x.get("index",-1)) for x in vals]!=idx: errors.append(fn+f":counter-order-d{disp.get('dispatch_id')}")
      counters=[]
      for v in vals:
        ci=int(v["index"]); m=meta_by[ci]; value=v.get("value")
        if not finite(value): errors.append(fn+f":nonfinite-d{disp.get('dispatch_id')}-c{ci}")
        unit=UNIT.get(int(m.get("unit",-1)),"unknown")
        counters.append({
          "native_name":m["name"],"native_category":m.get("category"),
          "normalized_name":NORMALIZED.get(m["name"]),"value":value,"unit":unit,
          "aggregation":aggregation(unit),"storage":STORAGE.get(int(m.get("storage",-1)),"UNKNOWN"),
          "uuid":m.get("uuid"),"description":m.get("description"),"evidence":"MEASURED"
        })
      observations.append({
        "schema_version":"0.1","artifact_type":"HARDWARE_OBSERVATION",
        "observation_id":f"arcllm-m3b-{workload}-{group}-d{int(disp['dispatch_id']):03d}",
        "hardware_profile_id":PROFILE_ID,"source_model_sha256":MODEL_SHA,
        "provider":{"kind":"VULKAN_KHR_PERFORMANCE_QUERY","name":"Intel Arc 140V Vulkan performance query",
                    "version":None,"native_source":"VK_KHR_performance_query","driver_version":None},
        "scope":{"kind":"DISPATCH","run_id":f"arcllm-v1-m3b-{workload}-{group}",
                 "dispatch_ids":[int(disp["dispatch_id"])],"semantic_node_ids":[disp["semantic_node_id"]],"token_index":15},
        "collection":{"quiet_host":True,"pass_index":None,"pass_count":int(probe["pass_count"]),
                      "pass_semantics":"COMBINED_AFTER_REQUIRED_PASSES","repetition_index":0,"repetition_count":1,
                      "concurrently_impacted":True,"profiling_lock":True,"admin_required":False,
                      "notes":["Counter timing is instrumented diagnostic only; do not substitute for I003/M1 timing."]},
        "counters":counters,"raw_artifact":fn,
        "notes":[f"runtime_name={disp.get('runtime_name')}","shader="+str(disp.get("shader"))]
      })
    runs.append({"workload":workload,"counter_group":group,"file":fn,"pass_count":probe.get("pass_count"),
                 "counter_count":len(meta),"dispatches":len(ds),"generated_hash":guard.get("generated_hash_fnv1a64"),
                 "restored_bytes_per_pass":probe.get("restored_bytes_per_pass")})

if errors:
  summary={"schema":"arcllm.v1.m3b.collection_summary.v0.1","status":"FAIL","errors":errors,"runs":runs}
  Path(a.out_summary).write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
  raise SystemExit("M3-B parser FAIL: "+"; ".join(errors[:20]))

if len(observations)!=2814: raise SystemExit(f"M3-B expected 2814 observations, got {len(observations)}")
with Path(a.out_observations).open("w",encoding="utf-8") as f:
  for x in observations: f.write(json.dumps(x,separators=(",",":"))+"\n")
summary={
 "schema":"arcllm.v1.m3b.collection_summary.v0.1","status":"PASS",
 "raw_runs":6,"dispatch_observations":len(observations),"unique_semantic_nodes_per_run":451,
 "workloads":["W-S","W-C"],"groups":["memory_cache","execution_occupancy","stall_cause"],
 "decode_index":15,"scope":"COMMAND","quiet_host":True,
 "pass_semantics":"COMBINED_AFTER_REQUIRED_PASSES",
 "timing_use":"INSTRUMENTED_DIAGNOSTIC_ONLY","benchmark_timing_substitution_forbidden":True,
 "runs":runs,"next":"INDEPENDENT_M3B_ADJUDICATION_AND_HARDWARE_MODEL_COUNTER_PATCH"
}
Path(a.out_summary).write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
print("M3_B_PARSER=PASS")
print("OBSERVATIONS=2814")
