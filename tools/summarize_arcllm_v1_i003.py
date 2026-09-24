#!/usr/bin/env python3
import argparse,json,math,statistics
from pathlib import Path

CELLS=[("A_WS","A","W-S"),("A_WC","A","W-C"),("B_WC","B","W-C"),("B_WS","B","W-S")]

def load(p):
    return json.loads(Path(p).read_text(encoding="utf-8-sig"))

def mad(v):
    if not v:return None
    m=statistics.median(v)
    return statistics.median([abs(x-m) for x in v])

def stats(v):
    if not v:return None
    return {"median":statistics.median(v),"min":min(v),"max":max(v),"mad":mad(v),"n":len(v)}

def measured_attempt(d):
    xs=d.get("attempts",[])
    return xs[0] if len(xs)==1 else None

def measured_resource(d):
    xs=d.get("attempt_summaries",[])
    for x in xs:
        if x.get("index")==0:return x
    return None

ap=argparse.ArgumentParser()
ap.add_argument("--results-dir",required=True)
ap.add_argument("--out",required=True)
a=ap.parse_args()
root=Path(a.results_dir)

errors=[]
cells={}
all_decode_latency_ratios=[]
all_e2e_ratios=[]
all_decode_tps_ratios=[]
recorded_pairs=0

for cell,session,wl in CELLS:
    pairs=[]
    for p in range(5):
        cr=root/f"i003_{cell}_p{p}_candidate_result.json"
        lr=root/f"i003_{cell}_p{p}_llama_result.json"
        ct=root/f"i003_{cell}_p{p}_candidate_resources.json"
        lt=root/f"i003_{cell}_p{p}_llama_resources.json"
        if not all(x.exists() for x in [cr,lr,ct,lt]):
            errors.append(f"{cell}/p{p}: missing artifact")
            continue
        cd,ld,ctr,ltr=map(load,[cr,lr,ct,lt])
        ca,la=measured_attempt(cd),measured_attempt(ld)
        cres,lres=measured_resource(ctr),measured_resource(ltr)
        order="candidate_first" if ((p + (1 if session=="B" else 0))%2)==0 else "llama_first"
        valid=bool(
            ca and la and ca.get("success") is True and la.get("success") is True and
            ca.get("final_logits_finite") is True and la.get("final_logits_finite") is True and
            len(ca.get("generated_token_ids",[]))==32 and len(la.get("generated_token_ids",[]))==32 and
            ca.get("decode_ms",0)>0 and la.get("decode_ms",0)>0 and
            ca.get("ttft_ms",0)>0 and la.get("ttft_ms",0)>0 and
            ca.get("e2e_ms",0)>0 and la.get("e2e_ms",0)>0
        )
        row={"pair":p,"order":order,"valid":valid}
        if valid:
            row.update({
                "candidate_ttft_ms":ca["ttft_ms"],"llama_ttft_ms":la["ttft_ms"],
                "candidate_decode_ms":ca["decode_ms"],"llama_decode_ms":la["decode_ms"],
                "candidate_decode_tps":ca["decode_tps"],"llama_decode_tps":la["decode_tps"],
                "candidate_e2e_ms":ca["e2e_ms"],"llama_e2e_ms":la["e2e_ms"],
                "ttft_latency_ratio_candidate_over_llama":ca["ttft_ms"]/la["ttft_ms"],
                "decode_latency_ratio_candidate_over_llama":ca["decode_ms"]/la["decode_ms"],
                "decode_throughput_ratio_candidate_over_llama":ca["decode_tps"]/la["decode_tps"],
                "e2e_latency_ratio_candidate_over_llama":ca["e2e_ms"]/la["e2e_ms"],
                "candidate_generated_hash":ca.get("generated_hash_fnv1a64"),
                "llama_generated_hash":la.get("generated_hash_fnv1a64"),
            })
            if cres and lres:
                for key,outkey in [
                    ("working_set_peak_bytes","working_set_ratio_candidate_over_llama"),
                    ("private_bytes_peak","private_bytes_ratio_candidate_over_llama"),
                ]:
                    cv,lv=cres.get(key),lres.get(key)
                    row[outkey]=(cv/lv) if cv and lv else None
            recorded_pairs+=1
        pairs.append(row)

    valid=[x for x in pairs if x["valid"]]
    def vals(k):return [x[k] for x in valid if x.get(k) is not None]
    summary={
        "session":session,"workload":wl,
        "pairs_recorded":len(pairs),
        "valid_pairs":len(valid),
        "ttft_latency_ratio_candidate_over_llama":stats(vals("ttft_latency_ratio_candidate_over_llama")),
        "decode_latency_ratio_candidate_over_llama":stats(vals("decode_latency_ratio_candidate_over_llama")),
        "decode_throughput_ratio_candidate_over_llama":stats(vals("decode_throughput_ratio_candidate_over_llama")),
        "e2e_latency_ratio_candidate_over_llama":stats(vals("e2e_latency_ratio_candidate_over_llama")),
        "working_set_ratio_candidate_over_llama":stats(vals("working_set_ratio_candidate_over_llama")),
        "private_bytes_ratio_candidate_over_llama":stats(vals("private_bytes_ratio_candidate_over_llama")),
        "pairs":pairs
    }
    cells[cell]=summary
    if len(valid)>=3:
        all_decode_latency_ratios.append(summary["decode_latency_ratio_candidate_over_llama"]["median"])
        all_e2e_ratios.append(summary["e2e_latency_ratio_candidate_over_llama"]["median"])
        all_decode_tps_ratios.append(summary["decode_throughput_ratio_candidate_over_llama"]["median"])

def gmean(v):
    return math.exp(sum(math.log(x) for x in v)/len(v)) if v and all(x>0 for x in v) else None

complete=(
    not errors and
    all(c["pairs_recorded"]==5 and c["valid_pairs"]>=3 for c in cells.values()) and
    len(cells)==4
)
classification="I003_MATCHED_EXTERNAL_CHARACTERIZATION_COMPLETE" if complete else "I003_MEASUREMENT_INVALID"
if not errors and len(cells)==4 and any(c["valid_pairs"]<3 for c in cells.values()):
    classification="I003_RUNTIME_INCOMPLETE"

out={
  "schema":"arcllm.v1.i003.matched_external_summary.v0.1",
  "classification":classification,
  "decision_role":"MATCHED_EXTERNAL_GAP_CHARACTERIZATION_NO_WINNER",
  "errors":errors,
  "recorded_valid_pairs":recorded_pairs,
  "expected_pairs":20,
  "cells":cells,
  "global":{
    "geomean_cell_median_decode_latency_ratio_candidate_over_llama":gmean(all_decode_latency_ratios),
    "geomean_cell_median_e2e_latency_ratio_candidate_over_llama":gmean(all_e2e_ratios),
    "geomean_cell_median_decode_throughput_ratio_candidate_over_llama":gmean(all_decode_tps_ratios)
  },
  "historical_q2_context":{
    "not_used_for_fresh_validity_or_causal_estimation":True,
    "W-S":{"old_ttft_arc_over_llama":9.153516357434288,"old_decode_tps_arc_over_llama":0.024986093071674383,"old_e2e_arc_over_llama":38.7580309302438},
    "W-C":{"old_ttft_arc_over_llama":10.331303662427766,"old_decode_tps_arc_over_llama":0.03060936855882324,"old_e2e_arc_over_llama":23.93979614037086}
  },
  "winner_declared":False,
  "next_mechanism_selected":False
}
Path(a.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print(json.dumps({"classification":classification,"valid_pairs":recorded_pairs},indent=2))
raise SystemExit(0 if classification=="I003_MATCHED_EXTERNAL_CHARACTERIZATION_COMPLETE" else 2)
