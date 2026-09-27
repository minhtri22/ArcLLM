from pathlib import Path
import json, statistics, sys

d=Path(sys.argv[1])
m=json.loads((d/"MANIFEST.json").read_text())
assert m["status"]=="PASS_COMPLETE_FRESH_COLLECTION"
assert len(m["cells"])==12
assert m["measured_attempts"]==60
assert m["historical_p6_timing_reused"] is False
assert m["selective_rerun"] is False

expected={
"A":[("A_CANONICAL","W-S"),("B_PLAN_SHADOW","W-S"),("C_PLAN_ACTIVE_RESIDUAL","W-S"),("C_PLAN_ACTIVE_RESIDUAL","W-C"),("B_PLAN_SHADOW","W-C"),("A_CANONICAL","W-C")],
"B":[("C_PLAN_ACTIVE_RESIDUAL","W-S"),("B_PLAN_SHADOW","W-S"),("A_CANONICAL","W-S"),("A_CANONICAL","W-C"),("B_PLAN_SHADOW","W-C"),("C_PLAN_ACTIVE_RESIDUAL","W-C")]
}
cells={}
for row in m["cells"]:
    key=(row["session"],row["arm"],row["workload"])
    obj=json.loads((d/row["file"]).read_text())
    assert obj["status"]=="PASS_CELL"
    assert obj["arm"]==row["arm"] and obj["workload"]==row["workload"]
    assert len(obj["attempts"])==5
    for a in obj["attempts"]:
        assert a["success"] and a["finite"] and a["dispatch_pass"] and a["lifecycle_pass"]
        assert len(a["generated_token_ids"])==32
    if row["arm"]=="A_CANONICAL":
        assert obj["plan"]["enabled"] is False
    else:
        p=obj["plan"]
        assert p["enabled"] is True
        assert p["nodes"]==469 and p["fixed_q4_fast_nodes"]==140 and p["regions"]==24104
        assert p["metadata_bytes"]==593504 and p["hash_fnv1a64"]=="04f3f884c0fc4fcc"
    cells[key]=obj

for sess,seq in expected.items():
    rows=sorted([x for x in m["cells"] if x["session"]==sess],key=lambda x:x["order"])
    assert [(x["arm"],x["workload"]) for x in rows]==seq

# Exact cross-arm token equality at matched attempt index.
for sess in ["A","B"]:
  for w in ["W-S","W-C"]:
    objs=[cells[(sess,a,w)] for a in ["A_CANONICAL","B_PLAN_SHADOW","C_PLAN_ACTIVE_RESIDUAL"]]
    for i in range(5):
      ids=[tuple(o["attempts"][i]["generated_token_ids"]) for o in objs]
      assert ids[0]==ids[1]==ids[2]

def med(obj,k): return statistics.median(a[k] for a in obj["attempts"])
def ratio_num(a,b,k): return med(a,k)/med(b,k)

results={}
all_pass=True
for sess in ["A","B"]:
  results[sess]={}
  for w in ["W-S","W-C"]:
    A=cells[(sess,"A_CANONICAL",w)]
    B=cells[(sess,"B_PLAN_SHADOW",w)]
    C=cells[(sess,"C_PLAN_ACTIVE_RESIDUAL",w)]
    row={
      "A":{"ttft_ms":med(A,"ttft_ms"),"decode_tps":med(A,"decode_tps"),"e2e_ms":med(A,"e2e_ms")},
      "B":{"ttft_ms":med(B,"ttft_ms"),"decode_tps":med(B,"decode_tps"),"e2e_ms":med(B,"e2e_ms")},
      "C":{"ttft_ms":med(C,"ttft_ms"),"decode_tps":med(C,"decode_tps"),"e2e_ms":med(C,"e2e_ms")},
    }
    row["B_over_A_decode"]=row["B"]["decode_tps"]/row["A"]["decode_tps"]
    row["B_over_A_e2e"]=row["B"]["e2e_ms"]/row["A"]["e2e_ms"]
    row["B_over_A_ttft"]=row["B"]["ttft_ms"]/row["A"]["ttft_ms"]
    row["C_over_B_decode"]=row["C"]["decode_tps"]/row["B"]["decode_tps"]
    row["C_over_B_e2e"]=row["C"]["e2e_ms"]/row["B"]["e2e_ms"]
    row["C_over_B_ttft"]=row["C"]["ttft_ms"]/row["B"]["ttft_ms"]
    row["C_over_A_decode"]=row["C"]["decode_tps"]/row["A"]["decode_tps"]
    row["C_over_A_e2e"]=row["C"]["e2e_ms"]/row["A"]["e2e_ms"]
    row["C_over_A_ttft"]=row["C"]["ttft_ms"]/row["A"]["ttft_ms"]
    row["plan_shadow_no_harm"]=(row["B_over_A_decode"]>=0.9 and row["B_over_A_e2e"]<=1.1 and row["B_over_A_ttft"]<=1.1)
    row["residual_material"]=(row["C_over_B_decode"]>=1.1 and row["C_over_B_e2e"]<=0.9 and row["C_over_B_ttft"]<=1.1)
    row["overall_material"]=(row["C_over_A_decode"]>=1.1 and row["C_over_A_e2e"]<=0.9 and row["C_over_A_ttft"]<=1.1)
    all_pass=all_pass and row["plan_shadow_no_harm"] and row["residual_material"] and row["overall_material"]
    results[sess][w]=row

out={
 "schema":"arcllm.anl64_crt.p1.machine_adjudication.v0.1",
 "F0_freshness":True,
 "F1_structural":True,
 "F2_exact_token_semantics":True,
 "F3_session_completeness":True,
 "comparisons":results,
 "verdict":"PASS_PLAN_NEUTRAL_RESIDUAL_INCREMENTAL_VALUE_ESTABLISHED" if all_pass else "FAIL_OR_PARTIAL_INCREMENTAL_VALUE_NOT_ESTABLISHED",
 "thresholds":{"decode_gain_min":1.1,"e2e_ratio_max":0.9,"ttft_ratio_max":1.1,"plan_no_harm_decode_min":0.9,"plan_no_harm_e2e_max":1.1},
 "llama_cpp_claim":False
}
(d/"MACHINE_ADJUDICATION.json").write_text(json.dumps(out,indent=2)+"\n")
print(out["verdict"])
