from pathlib import Path
import ast,json,subprocess,sys,tempfile

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"src/arcllm_v1_m3b_counter_collection.cpp"
RT=ROOT/"src/arcllm_v1_m3_counter_runtime.cpp"
SHIM=ROOT/"src/arcllm_v1_m3_counter_shim.cpp"
PARSER=ROOT/"tools/parse_arcllm_v1_m3b.py"
RUNNER=ROOT/"run_arcllm_v1_m3b.ps1"
BUILD=ROOT/"tools/build_arcllm_v1_m3b.ps1"
ADAPTER=ROOT/"third_party/token_xray/token_xray_arcllm_v1_adapter.h"

for p in [SRC,RT,SHIM,PARSER,RUNNER,BUILD,ADAPTER]:
    assert p.is_file(),p

s=SRC.read_text(encoding="utf-8")
for x in [
    "arcllm.v1.m3b.command_scope_counter_collection.v0.1",
    "counter_group","execute_counter_profiled","m3_counter_decode_index=15",
    "COMBINED_AFTER_REQUIRED_PASSES","quiet_host_required",
    "token_xray::arcllm_v1_semantic_node_id",
    "M3-B non-finite float counter",
]:
    assert x in s,x
assert "int warmups=1,measured=1;" in s
assert "profile_probe_decode_indices" not in s
assert s.count("sa1_q4k_subgroup_splitk.spv")==2

rt=RT.read_text(encoding="utf-8")
m=rt[rt.index("M3CounterProfileStats execute_counter_profiled"):]
for x in [
    "m3_perf_acquire_lock","m3_perf_submit_pass","m3_perf_get_results",
    "restore();","counterPassIndex",
]:
    if x=="counterPassIndex":
        continue
    assert x in m,x
assert m.count("cmd_reset_query_pool_")==1
pass_pos=m.index("for(uint32_t pass=0;pass<pass_count;++pass)")
reset_pos=m.index("cmd_reset_query_pool_(rcb,perf_qp")
assert reset_pos < pass_pos
assert "cmd_reset_query_pool_" not in m[pass_pos:]
assert m.index("m3_perf_acquire_lock") < m.index("begin_command_buffer_(cb,&bi)")
assert m.index("m3_perf_release_lock") < m.index("m3_perf_get_results")
assert "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT" in m
assert m.index("cmd_dispatch_(cb,op.gx,op.gy,op.gz)") < m.index("VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT", m.index("M3CounterProfileStats execute_counter_profiled")) < m.index("m3_perf_cmd_end_query")

shim=SHIM.read_text(encoding="utf-8")
for x in [
    "VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME",
    "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR",
    "VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR",
    "vkAcquireProfilingLockKHR",
    "VkPerformanceQuerySubmitInfoKHR",
    "counterPassIndex",
    "vkGetQueryPoolResults",
]:
    assert x in shim,x

r=RUNNER.read_text(encoding="utf-8")
for x in [
    "QuietHostConfirmed","M3-B requires quiet-host confirmation",
    "Start-Process","M3_B_RESULT=PASS",
    "HARDWARE_OBSERVATIONS.jsonl","M3B_COLLECTION_SUMMARY.json",
]:
    assert x in r,x
assert "Get-CimInstance" not in r and "powercfg" not in r

b=BUILD.read_text(encoding="utf-8")
for x in [
    "arcllm_v1_m3b_counter_collection.cpp",
    "arcllm_v1_m3_counter_shim.cpp",
    "gguf.cpp","tensor_store.cpp","vulkan-1.lib",
]:
    assert x in b,x
assert "arcllm_v1_m3_counter_runtime.cpp" not in b

# Parser syntax and a complete synthetic 6-run/469-dispatch regression.
ptxt=PARSER.read_text(encoding="utf-8")
compile(ptxt,str(PARSER),"exec")
tree=ast.parse(ptxt)
vals={}
for node in tree.body:
    if isinstance(node,ast.Assign) and len(node.targets)==1 and isinstance(node.targets[0],ast.Name):
        if node.targets[0].id in {"GROUPS","INDEX_NAME","EXPECTED_HASH"}:
            vals[node.targets[0].id]=ast.literal_eval(node.value)
GROUPS=vals["GROUPS"]; INDEX_NAME=vals["INDEX_NAME"]; EXPECTED_HASH=vals["EXPECTED_HASH"]

def semantic_ids():
    return [f"decode.synthetic.{i:03d}" for i in range(450)]+["decode.output.lm_head"]*19

with tempfile.TemporaryDirectory() as td0:
    td=Path(td0); sem=semantic_ids()
    for workload,stem in [("W-S","W_S"),("W-C","W_C")]:
        for group,indices in GROUPS.items():
            meta=[{
                "index":i,"name":INDEX_NAME[i],"category":"synthetic","description":"synthetic",
                "unit":0,"scope":2,"storage":5,"flags":3,"uuid":f"{i:032x}"
            } for i in indices]
            ds=[]
            for di in range(469):
                ds.append({
                    "dispatch_id":di,"runtime_name":f"op{di}","semantic_node_id":sem[di],
                    "shader":"synthetic.spv","workgroups":[1,1,1],
                    "counters":[{"index":i,"name":INDEX_NAME[i],"storage":5,"value":float(di+i+1)} for i in indices],
                })
            obj={
                "schema":"arcllm.v1.m3b.command_scope_counter_collection.v0.1","status":"PASS",
                "workload":workload,"counter_group":group,
                "counter_probe":{"decode_index":15,"scope":"COMMAND","pass_count":2,
                    "pass_semantics":"COMBINED_AFTER_REQUIRED_PASSES","logical_dispatches":469,
                    "physical_dispatch_executions":938,"restored_bytes_per_pass":1,
                    "quiet_host_required":True,"timing_use":"INSTRUMENTED_DIAGNOSTIC_ONLY"},
                "counter_meta":meta,"dispatches":ds,
                "semantic_guard":{"warmup_success":True,"measured_success":True,"dispatch_census_pass":True,
                    "generated_hash_fnv1a64":EXPECTED_HASH[workload]},
            }
            (td/f"M3B_{stem}_{group}.json").write_text(json.dumps(obj),encoding="utf-8")
    obs=td/"obs.jsonl"; summary=td/"summary.json"
    cp=subprocess.run([sys.executable,str(PARSER),"--results-dir",str(td),
                       "--out-observations",str(obs),"--out-summary",str(summary)],
                      capture_output=True,text=True)
    assert cp.returncode==0,(cp.stdout,cp.stderr)
    sj=json.loads(summary.read_text(encoding="utf-8"))
    assert sj["status"]=="PASS",sj
    assert sj["dispatch_observations"]==2814
    lines=obs.read_text(encoding="utf-8").splitlines()
    assert len(lines)==2814
    one=json.loads(lines[0])
    assert one["artifact_type"]=="HARDWARE_OBSERVATION"
    assert one["scope"]["kind"]=="DISPATCH"
    assert one["collection"]["pass_semantics"]=="COMBINED_AFTER_REQUIRED_PASSES"
    assert one["collection"]["pass_index"] is None
    assert one["collection"]["pass_indices_executed"]==[0,1]

print("ArcLLM v1 M3-B static + synthetic package PASS")

# Shared-memory cache-pollution guard: only fixed 4-byte decode token is host-restored.
assert 'std::vector<Buffer*> m3_restore_buffers={&b_dec_id};' in s
assert 'm3_restore_buffers.push_back(&b_kcache)' not in s
assert 'm3_restore_buffers.push_back(&b_vcache)' not in s
