import argparse,json,statistics
from pathlib import Path
SCHEMA="arcllm.v1.m1.post_i002_node_timing.v0.1"
FILES={"W-S":"m1_W_S.json","W-C":"m1_W_C.json"}
def med(xs): return statistics.median(xs) if xs else None
def family(name,dtype):
    if name=="token_embedding": return "embedding"
    if name=="output_norm" or "rmsnorm" in name: return "rmsnorm"
    if ".q_proj" in name: return "q_proj_q4_k"
    if ".k_proj" in name: return "k_proj_q4_k"
    if ".v_proj" in name: return "v_proj_"+dtype.get(name,"UNKNOWN").lower()
    if "rope" in name: return "rope"
    if "kv_store" in name: return "kv_store"
    if "cached_gqa" in name: return "attention"
    if ".o_proj" in name: return "o_proj_q4_k"
    if "residual" in name: return "residual_add"
    if "ffn_gate" in name or "ffn_up" in name: return "ffn_gate_up_q4_k_splitk"
    if "swiglu" in name: return "swiglu"
    if "ffn_down" in name: return "ffn_down_"+dtype.get(name,"UNKNOWN").lower()
    if name=="lm_head": return "lm_head_q6_k"
    return "other"

ap=argparse.ArgumentParser()
ap.add_argument("--results-dir",required=True)
ap.add_argument("--hardware-model",required=True)
ap.add_argument("--out",required=True)
args=ap.parse_args()
root=Path(args.results_dir)
hm=json.loads(Path(args.hardware_model).read_text(encoding="utf-8"))
if hm.get("schema")!="arcllm.v1.one_token_hardware_model.v0.2": raise SystemExit("hardware model schema mismatch")
dtype={x["name"]:x.get("dtype_quantization","") for x in hm["expanded_decode_dispatch_ledger"]}
data={}; errors=[]
for w,fn in FILES.items():
    p=root/fn
    if not p.is_file(): errors.append(w+":missing"); continue
    d=json.loads(p.read_text(encoding="utf-8")); data[w]=d
    if d.get("schema")!=SCHEMA: errors.append(w+":schema")
    if len(d.get("decode_op_names",[]))!=469: errors.append(w+":op-census")
    prod=d.get("production",{})
    if prod.get("decode_dispatches_per_step")!=469 or prod.get("profile_probe_decode_indices")!=[0,15,30]: errors.append(w+":production")
    ats=d.get("attempts",[])
    if len(ats)!=2: errors.append(w+":attempt-count")
    for i,a in enumerate(ats):
        if not a.get("success") or not a.get("dispatch_census_pass"): errors.append(f"{w}:{i}:attempt")
        probes=a.get("probes",[])
        if len(probes)!=3: errors.append(f"{w}:{i}:probe-count")
        for p0 in probes:
            if p0.get("decode_index") not in [0,15,30]: errors.append(f"{w}:{i}:probe-index")
            if len(p0.get("op_ticks",[]))!=469 or p0.get("chain_ticks",0)<=0: errors.append(f"{w}:{i}:ticks")
            if p0.get("timestamp_valid_bits",0)<=0 or p0.get("submit_wait_ms",0)<=0: errors.append(f"{w}:{i}:clock")
if errors:
    out={"schema":"arcllm.v1.m1.summary.v0.1","status":"FAIL","errors":errors}
else:
    names=data["W-S"]["decode_op_names"]
    if data["W-C"]["decode_op_names"]!=names: raise SystemExit("op-name mismatch")
    node_all={n:[] for n in names}; fam_all={}; workloads={}
    for w,d in data.items():
        node_w={n:[] for n in names}; fam_w={}; walls=[]
        for a in d["attempts"]:
            for p0 in a["probes"]:
                ct=float(p0["chain_ticks"]); wall=float(p0["submit_wait_ms"]); walls.append(wall)
                fsum={}
                for n,t in zip(names,p0["op_ticks"]):
                    share=float(t)/ct
                    x={"ticks":int(t),"tick_share":share,"wall_attributed_ms_proxy":wall*share}
                    node_w[n].append(x); node_all[n].append(x)
                    f=family(n,dtype); fsum[f]=fsum.get(f,0)+int(t)
                for f,t in fsum.items():
                    x={"tick_share":float(t)/ct,"wall_attributed_ms_proxy":wall*float(t)/ct}
                    fam_w.setdefault(f,[]).append(x); fam_all.setdefault(f,[]).append(x)
        workloads[w]={
          "profiled_steps":sum(len(a["probes"]) for a in d["attempts"]),
          "median_profiled_submit_wait_ms":med(walls),
          "node_metrics":{n:{"median_ticks":med([x["ticks"] for x in xs]),"median_tick_share":med([x["tick_share"] for x in xs]),"median_wall_attributed_ms_proxy":med([x["wall_attributed_ms_proxy"] for x in xs])} for n,xs in node_w.items()},
          "family_metrics":{f:{"median_tick_share":med([x["tick_share"] for x in xs]),"median_wall_attributed_ms_proxy":med([x["wall_attributed_ms_proxy"] for x in xs])} for f,xs in fam_w.items()}
        }
    nodes={n:{"median_ticks":med([x["ticks"] for x in xs]),"median_tick_share":med([x["tick_share"] for x in xs]),"median_wall_attributed_ms_proxy":med([x["wall_attributed_ms_proxy"] for x in xs]),"samples":len(xs)} for n,xs in node_all.items()}
    fams={f:{"median_tick_share":med([x["tick_share"] for x in xs]),"median_wall_attributed_ms_proxy":med([x["wall_attributed_ms_proxy"] for x in xs]),"samples":len(xs)} for f,xs in fam_all.items()}
    out={
      "schema":"arcllm.v1.m1.summary.v0.1","status":"PASS",
      "semantics":{
        "gpu_timestamps":"raw Vulkan query ticks; tick ratios/shares are exact under the same GPU timestamp clock",
        "wall_attributed_ms_proxy":"submit_wait_ms * tick_share; diagnostic attribution only, not timestampPeriod-calibrated pure-GPU milliseconds"
      },
      "measured_attempts":4,"profiled_decode_steps":12,
      "workloads":workloads,"global_node_metrics":nodes,"global_family_metrics":fams,
      "top_nodes_by_tick_share":sorted([{"name":n,**m} for n,m in nodes.items()],key=lambda x:x["median_tick_share"],reverse=True)[:40],
      "families_by_tick_share":sorted([{"family":f,**m} for f,m in fams.items()],key=lambda x:x["median_tick_share"],reverse=True),
      "next":"PATCH_ONE_TOKEN_HARDWARE_MODEL_WITH_M1_THEN_OPEN_M2_LLAMA_MAP"
    }
Path(args.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print(out["status"])
