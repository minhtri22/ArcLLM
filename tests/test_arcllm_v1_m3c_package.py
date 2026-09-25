from pathlib import Path
import ast,json,subprocess,sys,tempfile

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"src/arcllm_v1_m3c_targeted_counter_collection.cpp"
RT=ROOT/"src/arcllm_v1_m3_counter_runtime.cpp"
PARSER=ROOT/"tools/parse_arcllm_v1_m3c.py"
ADJ=ROOT/"tools/adjudicate_arcllm_v1_m3c.py"
RUNNER=ROOT/"run_arcllm_v1_m3c.ps1"
BUILD=ROOT/"tools/build_arcllm_v1_m3c.ps1"

for p in [SRC,RT,PARSER,ADJ,RUNNER,BUILD]:
    assert p.is_file(),p

s=SRC.read_text(encoding="utf-8")
for x in [
    "arcllm.v1.m3c.command_scope_counter_collection.v0.1",
    "execute_counter_profiled_selected",
    "m3c_target_dispatches.size()!=103u",
    'm3c_target_counts["lm_head_q6"]!=19u',
    'm3c_target_counts["ffn_down_q4"]!=14u',
    'm3c_target_counts["ffn_down_q6"]!=14u',
    'm3c_target_counts["split_k_q4_control"]!=56u',
    "queried_dispatches",
    "queried_dispatch_executions",
    "query_slot",
]:
    assert x in s,x

rt=RT.read_text(encoding="utf-8")
m=rt[rt.index("M3CounterProfileStats execute_counter_profiled_selected"):]
for x in [
    "query_op_indices",
    "query_slot_for_op",
    "queried_dispatch_count",
    "queried_dispatch_ids",
    "uint32_t(query_op_indices.size())",
    "const bool queried=qslot!=UINT32_MAX",
]:
    assert x in m,x
assert 'cmd_reset_query_pool_(rcb,perf_qp,0,uint32_t(query_op_indices.size()))' in m
assert 'st.values.resize(size_t(query_op_indices.size())*counter_indices.size())' in m
assert m.index("if(queried)m3_perf_cmd_begin_query") < m.index("cmd_dispatch_(cb,op.gx,op.gy,op.gz)")
assert m.index("cmd_dispatch_(cb,op.gx,op.gy,op.gz)") < m.index("if(queried){")

r=RUNNER.read_text(encoding="utf-8")
for x in [
    "QuietHostConfirmed",
    "M3-C requires quiet-host confirmation",
    "build_arcllm_v1_m3c.ps1",
    "parse_arcllm_v1_m3c.py",
    "adjudicate_arcllm_v1_m3c.py",
    "DISPATCH_OBSERVATIONS=618",
    "QUERIED_DISPATCHES_PER_RUN=103",
]:
    assert x in r,x

b=BUILD.read_text(encoding="utf-8")
assert "arcllm_v1_m3c_targeted_counter_collection.cpp" in b
assert "arcllm_v1_m3_counter_shim.cpp" in b
assert "vulkan-1.lib" in b

ptxt=PARSER.read_text(encoding="utf-8")
atxt=ADJ.read_text(encoding="utf-8")
compile(ptxt,str(PARSER),"exec")
compile(atxt,str(ADJ),"exec")
tree=ast.parse(ptxt)
vals={}
for node in tree.body:
    if isinstance(node,ast.Assign) and len(node.targets)==1 and isinstance(node.targets[0],ast.Name):
        if node.targets[0].id in {"GROUPS","INDEX_NAME","EXPECTED_HASH","TARGET_FAMILY_COUNTS","TARGET_DISPATCH_IDS"}:
            try: vals[node.targets[0].id]=ast.literal_eval(node.value)
            except Exception: pass
GROUPS=vals["GROUPS"]; INDEX_NAME=vals["INDEX_NAME"]; EXPECTED_HASH=vals["EXPECTED_HASH"]
TARGET_FAMILY_COUNTS=vals["TARGET_FAMILY_COUNTS"]
TARGET_IDS=sorted([12+16*i for i in range(28)]+[13+16*i for i in range(28)]+[15+16*i for i in range(28)]+list(range(450,469)))
assert len(TARGET_IDS)==103 and len(set(TARGET_IDS))==103

def family_for_id(di):
    if 450<=di<=468: return "lm_head_q6"
    if di in {12+16*i for i in range(28)} or di in {13+16*i for i in range(28)}:
        return "split_k_q4_control"
    downs=[15+16*i for i in range(28)]
    if di in downs:
        layer=(di-15)//16
        return "ffn_down_q4" if layer%2==0 else "ffn_down_q6"
    raise AssertionError(di)

def semantic_for(di,fam):
    if fam=="lm_head_q6": return "decode.output.lm_head"
    layer=(di-1)//16
    suffix={"split_k_q4_control":"ffn.gate" if di==(12+16*layer) else "ffn.up",
            "ffn_down_q4":"ffn.down","ffn_down_q6":"ffn.down"}[fam]
    return f"decode.layer.{layer:02d}.{suffix}"

with tempfile.TemporaryDirectory() as td0:
    td=Path(td0)
    for workload,stem in [("W-S","W_S"),("W-C","W_C")]:
        for group,indices in GROUPS.items():
            meta=[{
                "index":i,"name":INDEX_NAME[i],"category":"synthetic","description":"synthetic",
                "unit":1 if ("ACTIVE" in INDEX_NAME[i] or "STALL" in INDEX_NAME[i] or "BUSY" in INDEX_NAME[i] or "OCCUPANCY" in INDEX_NAME[i] or "UTILIZATION" in INDEX_NAME[i] or "FULL" in INDEX_NAME[i] or "HOLD" in INDEX_NAME[i] or "AVAILABLE" in INDEX_NAME[i] or "READY" in INDEX_NAME[i]) else (3 if "BYTE_" in INDEX_NAME[i] else 0),
                "scope":2,"storage":5,"flags":3,"uuid":f"{i:032x}"
            } for i in indices]
            ds=[]
            for qi,di in enumerate(TARGET_IDS):
                fam=family_for_id(di)
                ds.append({
                    "query_slot":qi,"dispatch_id":di,"target_family":fam,
                    "runtime_name":f"op{di}","semantic_node_id":semantic_for(di,fam),
                    "shader":"synthetic_q4k.spv" if "q4" in fam else "synthetic_q6k.spv",
                    "workgroups":[4736,1,1] if fam=="split_k_q4_control" else [56,1,1],
                    "counters":[{"index":i,"name":INDEX_NAME[i],"storage":5,"value":float(1000+di+i)} for i in indices],
                })
            obj={
                "schema":"arcllm.v1.m3c.command_scope_counter_collection.v0.1","status":"PASS",
                "workload":workload,"counter_group":group,
                "counter_probe":{"decode_index":15,"scope":"COMMAND","pass_count":2,
                    "pass_semantics":"COMBINED_AFTER_REQUIRED_PASSES","logical_dispatches":469,
                    "queried_dispatches":103,"physical_dispatch_executions":938,
                    "queried_dispatch_executions":206,"restored_bytes_per_pass":4,
                    "quiet_host_required":True,"timing_use":"INSTRUMENTED_DIAGNOSTIC_ONLY"},
                "counter_meta":meta,"dispatches":ds,
                "semantic_guard":{"warmup_success":True,"measured_success":True,"dispatch_census_pass":True,
                    "generated_hash_fnv1a64":EXPECTED_HASH[workload]},
            }
            (td/f"M3C_{stem}_{group}.json").write_text(json.dumps(obj),encoding="utf-8")
    obs=td/"obs.jsonl"; summary=td/"summary.json"; mech=td/"mechanism.json"
    cp=subprocess.run([sys.executable,str(PARSER),"--results-dir",str(td),
                       "--out-observations",str(obs),"--out-summary",str(summary)],
                      capture_output=True,text=True)
    assert cp.returncode==0,(cp.stdout,cp.stderr)
    sj=json.loads(summary.read_text(encoding="utf-8"))
    assert sj["status"]=="PASS",sj
    assert sj["dispatch_observations"]==618
    assert sj["queried_dispatches_per_run"]==103
    assert sj["target_family_counts"]==TARGET_FAMILY_COUNTS
    assert len(obs.read_text(encoding="utf-8").splitlines())==618

    cp2=subprocess.run([sys.executable,str(ADJ),"--results-dir",str(td),"--out",str(mech)],
                       capture_output=True,text=True)
    assert cp2.returncode==0,(cp2.stdout,cp2.stderr)
    mj=json.loads(mech.read_text(encoding="utf-8"))
    assert mj["status"]=="PASS"
    assert len(mj["comparisons"])==3
    assert mj["phase2_targets"]["primary"]=="lm_head_q6"

print("ArcLLM v1 M3-C static + synthetic package PASS")
