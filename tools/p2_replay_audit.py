from pathlib import Path
import json, statistics, math, sys

d=Path(sys.argv[1])
m=json.loads((d/"MANIFEST.json").read_text())
cells=[]
for row in sorted(m["cells"], key=lambda r:(r["session"],r["order"])):
    obj=json.loads((d/row["file"]).read_text())
    cells.append({"session":row["session"],"order":row["order"],"arm":row["arm"],"workload":row["workload"],"obj":obj})

def med(vals): return statistics.median(vals)
def mean(vals): return sum(vals)/len(vals)
def slope(xs,ys):
    xm,ym=mean(xs),mean(ys)
    den=sum((x-xm)**2 for x in xs)
    return 0.0 if den==0 else sum((x-xm)*(y-ym) for x,y in zip(xs,ys))/den
def ranks(xs):
    pairs=sorted((v,i) for i,v in enumerate(xs)); r=[0.0]*len(xs); j=0
    while j<len(pairs):
        k=j
        while k+1<len(pairs) and pairs[k+1][0]==pairs[j][0]: k+=1
        rr=(j+k+2)/2.0
        for q in range(j,k+1): r[pairs[q][1]]=rr
        j=k+1
    return r
def corr(xs,ys):
    if len(xs)<2:return 0.0
    xm,ym=mean(xs),mean(ys)
    a=sum((x-xm)*(y-ym) for x,y in zip(xs,ys))
    b=sum((x-xm)**2 for x in xs); c=sum((y-ym)**2 for y in ys)
    return 0.0 if b==0 or c==0 else a/math.sqrt(b*c)
summary={}
for c in cells:
    at=c["obj"]["attempts"]
    key=(c["session"],c["arm"],c["workload"])
    summary[key]={
      "ttft_ms":med([a["ttft_ms"] for a in at]),
      "decode_tps":med([a["decode_tps"] for a in at]),
      "e2e_ms":med([a["e2e_ms"] for a in at]),
      "attempt_decode_slope":slope(list(range(5)),[a["decode_tps"] for a in at]),
      "attempt_e2e_slope":slope(list(range(5)),[a["e2e_ms"] for a in at]),
      "attempt_ttft_slope":slope(list(range(5)),[a["ttft_ms"] for a in at]),
      "attempt_decode_first_last_ratio":at[-1]["decode_tps"]/at[0]["decode_tps"],
      "attempt_e2e_last_first_ratio":at[-1]["e2e_ms"]/at[0]["e2e_ms"],
    }

session_shift={}
for arm in ["A_CANONICAL","B_PLAN_PREBOUND","C_PLAN_PREBOUND_RESIDUAL"]:
  session_shift[arm]={}
  for w in ["W-S","W-C"]:
    A=summary[("A",arm,w)]; B=summary[("B",arm,w)]
    session_shift[arm][w]={
      "B_over_A_decode":B["decode_tps"]/A["decode_tps"],
      "B_over_A_e2e":B["e2e_ms"]/A["e2e_ms"],
      "B_over_A_ttft":B["ttft_ms"]/A["ttft_ms"]
    }
# Cell-order/global-order association using cell medians.
order_rows=[]
global_order=0
for sess in ["A","B"]:
  for c in [x for x in cells if x["session"]==sess]:
    global_order+=1
    s=summary[(sess,c["arm"],c["workload"])]
    order_rows.append((sess,c["order"],global_order,c["arm"],c["workload"],s))

order_assoc={}
for w in ["W-S","W-C"]:
    rows=[r for r in order_rows if r[4]==w]
    order_assoc[w]={
      "spearman_cell_order_decode":corr(ranks([r[1] for r in rows]),ranks([r[5]["decode_tps"] for r in rows])),
      "spearman_cell_order_e2e":corr(ranks([r[1] for r in rows]),ranks([r[5]["e2e_ms"] for r in rows])),
      "spearman_global_order_decode":corr(ranks([r[2] for r in rows]),ranks([r[5]["decode_tps"] for r in rows])),
      "spearman_global_order_e2e":corr(ranks([r[2] for r in rows]),ranks([r[5]["e2e_ms"] for r in rows]))
    }

within=[]
for key,s in summary.items():
    within.append({"session":key[0],"arm":key[1],"workload":key[2],**{k:v for k,v in s.items() if k.startswith("attempt_")}})
decode_session_ratios=[session_shift[a][w]["B_over_A_decode"] for a in session_shift for w in ["W-S","W-C"]]
e2e_session_ratios=[session_shift[a][w]["B_over_A_e2e"] for a in session_shift for w in ["W-S","W-C"]]
ttft_session_ratios=[session_shift[a][w]["B_over_A_ttft"] for a in session_shift for w in ["W-S","W-C"]]
out={
 "schema":"arcllm.anl64_crt.p2.replay_analysis.v0.1",
 "collection_status":m["status"],
 "cells":len(m["cells"]),
 "measured_attempts":m["measured_attempts"],
 "session_shift":session_shift,
 "session_shift_summary":{
   "decode_B_over_A_all_positive":all(x>1 for x in decode_session_ratios),
   "decode_B_over_A_min":min(decode_session_ratios),
   "decode_B_over_A_max":max(decode_session_ratios),
   "decode_B_over_A_median":med(decode_session_ratios),
   "e2e_B_over_A_all_improve":all(x<1 for x in e2e_session_ratios),
   "e2e_B_over_A_min":min(e2e_session_ratios),
   "e2e_B_over_A_max":max(e2e_session_ratios),
   "e2e_B_over_A_median":med(e2e_session_ratios),
   "ttft_B_over_A_min":min(ttft_session_ratios),
   "ttft_B_over_A_max":max(ttft_session_ratios)
 },
 "order_association":order_assoc,
 "within_cell_attempt_drift":within
}
(d/"P2_REPLAY_ANALYSIS.json").write_text(json.dumps(out,indent=2)+"\n")
print(json.dumps(out["session_shift_summary"],indent=2))
print(json.dumps(order_assoc,indent=2))
