from pathlib import Path
import hashlib, json, statistics, subprocess, sys

root=Path(sys.argv[1])
repo=Path(sys.argv[2]) if len(sys.argv)>2 else Path(".")
manifest=json.loads((root/"MANIFEST.json").read_text())
schedule=json.loads((repo/"config/anl64_crt_p3_blocked_randomized_schedule_v0.3.json").read_text())
pre=json.loads((repo/"config/anl64_crt_p3_blocked_randomized_state_aware_replication_prelock_v0.3.json").read_text())
lock=json.loads((repo/"artifacts/ANL64_CRT/ANL64_CRT_P3_EXECUTION_LOCK_v0.1.json").read_text())

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def med(xs): return statistics.median(float(x) for x in xs)
def cm(o,k): return med([a[k] for a in o["attempts"]])
def vals(o): return {"decode":cm(o,"decode_tps"),"e2e":cm(o,"e2e_ms"),"ttft":cm(o,"ttft_ms")}
def ratios(x,y): return {"decode":x["decode"]/y["decode"],"e2e":x["e2e"]/y["e2e"],"ttft":x["ttft"]/y["ttft"]}
def plan_ok(r): return r["decode"]>=0.9 and r["e2e"]<=1.1 and r["ttft"]<=1.1
def material(r): return r["decode"]>=1.1 and r["e2e"]<=0.9
def ttft_safe(r): return r["ttft"]<=1.1

assert manifest["status"]=="PASS_COMPLETE_FRESH_P3_COLLECTION"
assert manifest["repository_head"]=="6c9d206ddb5ba79babe7b6050740203bd6728f7e"
assert manifest["execution_lock_blob"]=="6cc9358ffe5be594b72039ccef8f5151e22fed21"
assert manifest["schedule_blob"]=="54bad896502b78ef6ca1223b8bf100ef87bfcef9"
assert manifest["collector_blob"]=="67a6f0bd24f6523aac0b78609e5b22c500da2b47"
assert manifest["runtime_source_blob"]=="b755618be6c662dc35d7c22ac31b6ea51d41bf0b"
assert manifest["runtime_exe_sha256"]==lock["runtime_exe_sha256"]
assert manifest["model_sha256"]==lock["model_sha256"]
assert len(manifest["cells"])==36 and manifest["measured_attempts"]==180
assert manifest["valid_completed_cell_reruns"]==0
assert manifest["selective_reruns"]==0
assert manifest["adaptive_reordering"] is False

expected=[]
g=0
for b in schedule["blocks"]:
    for pos,condition in enumerate(b["order"],1):
        g+=1
        expected.append((int(b["block"]),pos,g,condition))
observed=[(int(r["block"]),int(r["position"]),int(r["global_ordinal"]),r["condition"]) for r in manifest["cells"]]
assert observed==expected

cells={}
prestates={}
for r in manifest["cells"]:
    cp=root/r["cell_file"]; sp=root/r["prestate_file"]
    assert cp.exists() and sp.exists()
    assert sha(cp)==r["cell_sha256"]
    assert sha(sp)==r["prestate_sha256"]
    o=json.loads(cp.read_text()); st=json.loads(sp.read_text())
    assert o["status"]=="PASS_CELL"
    assert o["arm"]==r["arm"] and o["workload"]==r["workload"]
    assert len(o["attempts"])==5
    assert all(a["success"] and a["finite"] and a["dispatch_pass"] and a["lifecycle_pass"] for a in o["attempts"])
    assert st["environment_invariants_pass"] is True
    assert st["global_cell_ordinal"]==r["global_ordinal"]
    assert st["block_id"]==r["block"]
    assert st["ordinal_position_within_block"]==r["position"]
    assert st["previous_condition"]==r["previous_condition"]
    cells[(r["block"],r["arm"],r["workload"])]=o
    prestates[(r["block"],r["arm"],r["workload"])]=st

for b in range(1,7):
    for w in ("W-S","W-C"):
        trio=[cells[(b,a,w)] for a in ("A_CANONICAL","B_PLAN_PREBOUND","C_PLAN_PREBOUND_RESIDUAL")]
        for i in range(5):
            toks=[tuple(o["attempts"][i]["generated_token_ids"]) for o in trio]
            assert toks[0]==toks[1]==toks[2]

blocks={}
for b in range(1,7):
    blocks[str(b)]={}
    for w in ("W-S","W-C"):
        A=vals(cells[(b,"A_CANONICAL",w)])
        B=vals(cells[(b,"B_PLAN_PREBOUND",w)])
        C=vals(cells[(b,"C_PLAN_PREBOUND_RESIDUAL",w)])
        ba=ratios(B,A); cb=ratios(C,B); ca=ratios(C,A)
        blocks[str(b)][w]={
          "B_over_A":ba,"C_over_B":cb,"C_over_A":ca,
          "plan_no_harm":plan_ok(ba),
          "residual_material":material(cb),
          "total_material":material(ca),
          "total_ttft_safe":ttft_safe(ca)
        }

pooled={}
G5=True; G6=True
for w in ("W-S","W-C"):
    pooled[w]={}
    for name in ("B_over_A","C_over_B","C_over_A"):
        pooled[w][name]={k:med([blocks[str(b)][w][name][k] for b in range(1,7)]) for k in ("decode","e2e","ttft")}
    pplan=plan_ok(pooled[w]["B_over_A"])
    pres=material(pooled[w]["C_over_B"])
    ptotal=material(pooled[w]["C_over_A"])
    pa=sum(blocks[str(b)][w]["plan_no_harm"]==pplan for b in range(1,7))
    ra=sum(blocks[str(b)][w]["residual_material"]==pres for b in range(1,7))
    ta=sum(blocks[str(b)][w]["total_material"]==ptotal for b in range(1,7))
    tsa=sum(blocks[str(b)][w]["total_ttft_safe"] for b in range(1,7))
    pooled[w].update({
      "plan_no_harm":pplan,"plan_agree_blocks":pa,
      "residual_material":pres,"residual_agree_blocks":ra,
      "total_material":ptotal,"total_agree_blocks":ta,
      "total_ttft_safe":ttft_safe(pooled[w]["C_over_A"]),"ttft_safe_blocks":tsa
    })
    G5 &= pa>=5
    G6 &= ra>=5

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

out={
  "schema":"arcllm.anl64_crt.p3.independent_postoutcome_review.v0.1",
  "evidence_source":"raw P3 manifest + 36 cell artifacts + 36 prestate artifacts only",
  "machine_adjudication_file_read":False,
  "integrity":{
    "manifest_complete":True,
    "cell_count":36,
    "prestate_count":36,
    "measured_attempts":180,
    "all_manifest_sha256_match":True,
    "exact_schedule_match":True,
    "exact_token_semantics":True,
    "all_environment_invariants_pass":True,
    "valid_completed_cell_reruns":0,
    "selective_reruns":0,
    "adaptive_reordering":False
  },
  "gates":{
    "G0_provenance_completeness":True,
    "G1_semantic_structural":True,
    "G2_schedule_balance":True,
    "G3_total_workload_split_replication":G3,
    "G4_total_TTFT_guard":G4,
    "G5_plan_identifiability":G5,
    "G6_residual_identifiability":G6
  },
  "pooled":pooled,
  "block_classifications":{
    w:{
      "plan_no_harm":[blocks[str(b)][w]["plan_no_harm"] for b in range(1,7)],
      "residual_material":[blocks[str(b)][w]["residual_material"] for b in range(1,7)],
      "total_material":[blocks[str(b)][w]["total_material"] for b in range(1,7)],
      "total_ttft_safe":[blocks[str(b)][w]["total_ttft_safe"] for b in range(1,7)]
    } for w in ("W-S","W-C")
  },
  "verdict":verdict,
  "evidence_integrity_pass":True,
  "formal_adjudication_permitted":True
}
(root/"INDEPENDENT_POSTOUTCOME_REVIEW.json").write_text(json.dumps(out,indent=2)+"\n")
print(json.dumps(out,indent=2))
