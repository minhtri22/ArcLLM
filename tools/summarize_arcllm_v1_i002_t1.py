import argparse,json,statistics,math
from pathlib import Path

CELLS={"A_WS":("A","W-S"),"A_WC":("A","W-C"),"B_WC":("B","W-C"),"B_WS":("B","W-S")}
ap=argparse.ArgumentParser();ap.add_argument("--results-dir",required=True);ap.add_argument("--out",required=True)
a=ap.parse_args();root=Path(a.results_dir)
errors=[];cells={}
for fn,(sess,wl) in CELLS.items():
    p=root/f"i002_t1_{fn}.json"
    if not p.exists(): errors.append(f"missing {p.name}");continue
    d=json.loads(p.read_text(encoding="utf-8-sig"));cells[fn]=d
    if d.get("schema")!="arcllm.v1.i002.t1.real_model_transfer.v0.1":errors.append(f"{fn}: schema")
    if d.get("session")!=sess or d.get("workload")!=wl:errors.append(f"{fn}: identity")
    obs=d.get("comparisons",[])
    if len(obs)!=18:errors.append(f"{fn}: comparisons {len(obs)}")
    keys={(x.get("layer"),x.get("decode_index"),x.get("operator")) for x in obs}
    expected={(l,di,op) for l in [0,13,27] for di in [0,15,30] for op in ["gate","up"]}
    if keys!=expected:errors.append(f"{fn}: sample matrix")
    for i,x in enumerate(obs):
        if not x.get("finite") or not x.get("pass"):errors.append(f"{fn}/{i}: correctness")
        if x.get("max_abs",1)>0.02 or x.get("rmse",1)>0.005:errors.append(f"{fn}/{i}: tolerance")
        if x.get("baseline_ticks",0)<=0 or x.get("candidate_ticks",0)<=0:errors.append(f"{fn}/{i}: ticks")

# Baseline continuation semantics should replicate across sessions by workload.
for wl in ["W-S","W-C"]:
    seq=[tuple(d.get("generated_token_ids",[])) for d in cells.values() if d.get("workload")==wl]
    if len(seq)==2 and seq[0]!=seq[1]:errors.append(f"{wl}: generated sequence session drift")

summary={"schema":"arcllm.v1.i002.t1.adjudication_input.v0.1","validity":"FAIL" if errors else "PASS","errors":errors}
if not errors:
    per={}
    for k,d in cells.items():
        speeds=[x["baseline_ticks"]/x["candidate_ticks"] for x in d["comparisons"]]
        per[k]={
            "median_component_speedup":statistics.median(speeds),
            "min_component_speedup":min(speeds),
            "max_abs_max":max(x["max_abs"] for x in d["comparisons"]),
            "rmse_max":max(x["rmse"] for x in d["comparisons"]),
            "correctness_pass":all(x["pass"] for x in d["comparisons"]),
            "g1_component_speedup_pass":statistics.median(speeds)>=1.5,
        }
    summary["cells"]=per
    summary["decision"]="PASS_T1_REAL_MODEL_COMPONENT_TRANSFER" if all(x["g1_component_speedup_pass"] and x["correctness_pass"] for x in per.values()) else "FAIL_T1_REAL_MODEL_COMPONENT_TRANSFER"
Path(a.out).write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
print(summary.get("decision","INVALID"))
raise SystemExit(0 if not errors else 2)
