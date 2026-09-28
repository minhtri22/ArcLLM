#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, statistics, sys

root=Path(sys.argv[1])
repo=Path(sys.argv[2]) if len(sys.argv)>2 else Path(".")
m=json.loads((root/"MANIFEST.json").read_text())
s=json.loads((repo/"config/anl64_crt_p3_blocked_randomized_schedule_v0.3.json").read_text())

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def med(xs): return statistics.median(float(x) for x in xs)
def cm(o,k): return med([a[k] for a in o["attempts"]])

assert m["status"]=="PASS_COMPLETE_FRESH_P3_COLLECTION"
assert len(m["cells"])==36 and m["measured_attempts"]==180
assert m["valid_completed_cell_reruns"]==0 and m["selective_reruns"]==0 and m["adaptive_reordering"] is False

expected=[]
g=0
for b in s["blocks"]:
    for p,c in enumerate(b["order"],1):
        g+=1
        expected.append((int(b["block"]),p,g,c))
observed=[(int(r["block"]),int(r["position"]),int(r["global_ordinal"]),r["condition"]) for r in m["cells"]]
assert observed==expected

rows={}
states={}
for r in m["cells"]:
    cp=root/r["cell_file"]
    sp=root/r["prestate_file"]
    assert sha(cp)==r["cell_sha256"] and sha(sp)==r["prestate_sha256"]
    o=json.loads(cp.read_text())
    st=json.loads(sp.read_text())
    assert o["status"]=="PASS_CELL" and o["arm"]==r["arm"] and o["workload"]==r["workload"]
    assert len(o["attempts"])==5
    assert all(a["success"] and a["finite"] and a["dispatch_pass"] and a["lifecycle_pass"] for a in o["attempts"])
    assert st["environment_invariants_pass"] is True
    assert st["global_cell_ordinal"]==r["global_ordinal"]
    assert st["block_id"]==r["block"]
    assert st["ordinal_position_within_block"]==r["position"]
    assert st["previous_condition"]==r["previous_condition"]
    key=(r["block"],r["arm"],r["workload"])
    rows[key]=o
    states[key]=st

for b in range(1,7):
    for w in ("W-S","W-C"):
        trio=[rows[(b,a,w)] for a in ("A_CANONICAL","B_PLAN_PREBOUND","C_PLAN_PREBOUND_RESIDUAL")]
        for i in range(5):
            toks=[tuple(o["attempts"][i]["generated_token_ids"]) for o in trio]
            assert toks[0]==toks[1]==toks[2]

def vals(o):
    return {"decode":cm(o,"decode_tps"),"e2e":cm(o,"e2e_ms"),"ttft":cm(o,"ttft_ms")}
def ratios(x,y):
    return {"decode":x["decode"]/y["decode"],"e2e":x["e2e"]/y["e2e"],"ttft":x["ttft"]/y["ttft"]}
def plan_ok(r): return r["decode"]>=0.9 and r["e2e"]<=1.1 and r["ttft"]<=1.1
def material(r): return r["decode"]>=1.1 and r["e2e"]<=0.9
def ttft_safe(r): return r["ttft"]<=1.1

blocks={}
for b in range(1,7):
    blocks[str(b)]={}
    for w in ("W-S","W-C"):
        A=vals(rows[(b,"A_CANONICAL",w)])
        B=vals(rows[(b,"B_PLAN_PREBOUND",w)])
        C=vals(rows[(b,"C_PLAN_PREBOUND_RESIDUAL",w)])
        ba=ratios(B,A); cb=ratios(C,B); ca=ratios(C,A)
        blocks[str(b)][w]={
          "A":A,"B":B,"C":C,
          "B_over_A":ba,"C_over_B":cb,"C_over_A":ca,
          "plan_no_harm":plan_ok(ba),
          "residual_material":material(cb),
          "total_material":material(ca),
          "total_ttft_safe":ttft_safe(ca)
        }

pooled={}
G5=True
G6=True
for w in ("W-S","W-C"):
    pooled[w]={}
    for name in ("B_over_A","C_over_B","C_over_A"):
        pooled[w][name]={k:med([blocks[str(b)][w][name][k] for b in range(1,7)]) for k in ("decode","e2e","ttft")}
    pplan=plan_ok(pooled[w]["B_over_A"])
    pres=material(pooled[w]["C_over_B"])
    ptotal=material(pooled[w]["C_over_A"])
    plan_agree=sum(blocks[str(b)][w]["plan_no_harm"]==pplan for b in range(1,7))
    res_agree=sum(blocks[str(b)][w]["residual_material"]==pres for b in range(1,7))
    total_agree=sum(blocks[str(b)][w]["total_material"]==ptotal for b in range(1,7))
    ttft_agree=sum(blocks[str(b)][w]["total_ttft_safe"] for b in range(1,7))
    pooled[w].update({
      "plan_no_harm":pplan,"plan_agree_blocks":plan_agree,
      "residual_material":pres,"residual_agree_blocks":res_agree,
      "total_material":ptotal,"total_agree_blocks":total_agree,
      "total_ttft_safe":ttft_safe(pooled[w]["C_over_A"]),"ttft_safe_blocks":ttft_agree
    })
    G5 = G5 and plan_agree>=5
    G6 = G6 and res_agree>=5

G3=(not pooled["W-S"]["total_material"] and pooled["W-S"]["total_agree_blocks"]>=5 and
    pooled["W-C"]["total_material"] and pooled["W-C"]["total_agree_blocks"]>=5)
G4=all(pooled[w]["total_ttft_safe"] and pooled[w]["ttft_safe_blocks"]>=5 for w in ("W-S","W-C"))

if not G3:
    verdict="FAIL_TOTAL_SPLIT_NOT_REPLICATED"
elif not G4:
    verdict="FAIL_TTFT_HARM"
elif G5 and G6:
    verdict="PASS_TOTAL_SPLIT_AND_DECOMPOSITION_IDENTIFIED"
else:
    verdict="PASS_TOTAL_SPLIT_DECOMPOSITION_REMAINS_UNDERIDENTIFIED"

cov_fields=[
  "system_CPU_utilization_percent",
  "available_physical_memory_bytes",
  "CPU_processor_performance_percent",
  "GPU_engine_nonzero_counter_count",
  "GPU_engine_max_utilization_percent",
  "GPU_engine_sum_utilization_percent"
]
cov_summary={}
for f in cov_fields:
    xs=[float(st[f]) for st in states.values()]
    cov_summary[f]={"min":min(xs),"median":med(xs),"max":max(xs)}

out={
  "schema":"arcllm.anl64_crt.p3.machine_adjudication.v0.1",
  "G0_provenance_completeness":True,
  "G1_semantic_structural":True,
  "G2_schedule_balance":True,
  "G3_total_workload_split_replication":G3,
  "G4_total_TTFT_guard":G4,
  "G5_plan_identifiability":G5,
  "G6_residual_identifiability":G6,
  "blocks":blocks,
  "pooled":pooled,
  "secondary_covariate_summary":cov_summary,
  "specific_nuisance_mechanism_claimed":False,
  "verdict":verdict
}
(root/"MACHINE_ADJUDICATION.json").write_text(json.dumps(out,indent=2)+"\n")
print(verdict)
