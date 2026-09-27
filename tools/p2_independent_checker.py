from pathlib import Path
import json, statistics, sys

d=Path(sys.argv[1])
m=json.loads((d/"MANIFEST.json").read_text())
cells={}
for row in m["cells"]:
    o=json.loads((d/row["file"]).read_text())
    cells[(row["session"],row["arm"],row["workload"])]=o

def med(o,k): return statistics.median(float(a[k]) for a in o["attempts"])
def vals(sess,arm,w):
    o=cells[(sess,arm,w)]
    return {"d":med(o,"decode_tps"),"e":med(o,"e2e_ms"),"t":med(o,"ttft_ms")}

shared=[]
for arm in ["A_CANONICAL","B_PLAN_PREBOUND","C_PLAN_PREBOUND_RESIDUAL"]:
  for w in ["W-S","W-C"]:
    a,b=vals("A",arm,w),vals("B",arm,w)
    shared.append({"arm":arm,"workload":w,"decode":b["d"]/a["d"],"e2e":b["e"]/a["e"],"ttft":b["t"]/a["t"]})
assert all(x["decode"]>1 for x in shared)
assert all(x["e2e"]<1 for x in shared)
total={}
for sess in ["A","B"]:
  total[sess]={}
  for w in ["W-S","W-C"]:
    a=vals(sess,"A_CANONICAL",w); c=vals(sess,"C_PLAN_PREBOUND_RESIDUAL",w)
    rd=c["d"]/a["d"]; re=c["e"]/a["e"]; rt=c["t"]/a["t"]
    total[sess][w]={"decode":rd,"e2e":re,"ttft":rt,"material":rd>=1.1 and re<=0.9,"ttft_guard":rt<=1.1}

assert total["A"]["W-S"]["material"] is False
assert total["B"]["W-S"]["material"] is False
assert total["A"]["W-C"]["material"] is True
assert total["B"]["W-C"]["material"] is True
assert all(total[s][w]["ttft_guard"] for s in total for w in total[s])

out={
 "schema":"arcllm.anl64_crt.p2.independent_replay_check.v0.1",
 "shared_session_direction_6_of_6":True,
 "canonical_A_session_shift":{
   "W-S":shared[0],
   "W-C":shared[1]
 },
 "total_C_over_A":total,
 "conclusion":{
   "global_nonstationarity_present_independent_of_ANL64":True,
   "ANL64_specific_instability_established":False,
   "plan_vs_residual_decomposition_identifiable":False,
   "total_transfer_workload_classification_replicates":True
 }
}
(d/"P2_INDEPENDENT_CHECK.json").write_text(json.dumps(out,indent=2)+"\n")
print(json.dumps(out["conclusion"],indent=2))
