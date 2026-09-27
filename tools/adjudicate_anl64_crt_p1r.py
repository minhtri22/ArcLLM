from pathlib import Path
import json, statistics, sys

d=Path(sys.argv[1])
m=json.loads((d/"MANIFEST.json").read_text())
assert m["status"]=="PASS_COMPLETE_FRESH_P1R_COLLECTION"
assert len(m["cells"])==12 and m["measured_attempts"]==60
assert m["invalid_p1_partial_reused"] is False
assert m["historical_p6_timing_reused"] is False
assert m["selective_rerun"] is False

expected={
"A":[("A_CANONICAL","W-S"),("B_PLAN_PREBOUND","W-S"),("C_PLAN_PREBOUND_RESIDUAL","W-S"),("C_PLAN_PREBOUND_RESIDUAL","W-C"),("B_PLAN_PREBOUND","W-C"),("A_CANONICAL","W-C")],
"B":[("C_PLAN_PREBOUND_RESIDUAL","W-S"),("B_PLAN_PREBOUND","W-S"),("A_CANONICAL","W-S"),("A_CANONICAL","W-C"),("B_PLAN_PREBOUND","W-C"),("C_PLAN_PREBOUND_RESIDUAL","W-C")]
}
cells={}
for row in m["cells"]:
    key=(row["session"],row["arm"],row["workload"])
    obj=json.loads((d/row["file"]).read_text())
    assert obj["status"]=="PASS_CELL"
    assert obj["arm"]==row["arm"] and obj["workload"]==row["workload"]
    assert len(obj["attempts"])==5
    assert obj["runtime"]["prefill_dispatches"]==441
    assert obj["runtime"]["decode_dispatches_per_step"]==469
    assert obj["runtime"]["decode_steps"]==31
    assert obj["runtime"]["I002_gate_up_unchanged"] is True
    assert obj["runtime"]["Q4V4_down_unchanged"] is True
    for a in obj["attempts"]:
        assert a["success"] and a["finite"] and a["dispatch_pass"] and a["lifecycle_pass"]
        assert len(a["generated_token_ids"])==32
    if row["arm"]=="A_CANONICAL":
        assert obj["plan"]["enabled"] is False
    else:
        p=obj["plan"]
        assert p["enabled"] is True and p["prebound_before_timing"] is True and p["per_token_plan_lookup"] is False
        assert p["nodes"]==469 and p["quant_linear_nodes"]==215 and p["fixed_q4_fast_nodes"]==140
        assert p["regions"]==24104 and p["metadata_bytes"]==593504 and p["hash_fnv1a64"]=="04f3f884c0fc4fcc"
    cells[key]=obj

for sess,seq in expected.items():
    rows=sorted([x for x in m["cells"] if x["session"]==sess],key=lambda x:x["order"])
    assert [(x["arm"],x["workload"]) for x in rows]==seq

for sess in ["A","B"]:
  for w in ["W-S","W-C"]:
    objs=[cells[(sess,a,w)] for a in ["A_CANONICAL","B_PLAN_PREBOUND","C_PLAN_PREBOUND_RESIDUAL"]]
    for i in range(5):
      ids=[tuple(o["attempts"][i]["generated_token_ids"]) for o in objs]
      assert ids[0]==ids[1]==ids[2]

def med(obj,k): return statistics.median(a[k] for a in obj["attempts"])

results={}
plan_ok=residual_ok=overall_ok=ttft_ok=True
for sess in ["A","B"]:
  results[sess]={}
  for w in ["W-S","W-C"]:
    A=cells[(sess,"A_CANONICAL",w)]
    B=cells[(sess,"B_PLAN_PREBOUND",w)]
    C=cells[(sess,"C_PLAN_PREBOUND_RESIDUAL",w)]
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
    row["plan_no_harm"]=(row["B_over_A_decode"]>=0.9 and row["B_over_A_e2e"]<=1.1 and row["B_over_A_ttft"]<=1.1)
    row["residual_material"]=(row["C_over_B_decode"]>=1.1 and row["C_over_B_e2e"]<=0.9)
    row["overall_material"]=(row["C_over_A_decode"]>=1.1 and row["C_over_A_e2e"]<=0.9)
    row["ttft_guard"]=(row["C_over_A_ttft"]<=1.1)
    plan_ok &= row["plan_no_harm"]
    residual_ok &= row["residual_material"]
    overall_ok &= row["overall_material"]
    ttft_ok &= row["ttft_guard"]
    results[sess][w]=row

if plan_ok and residual_ok and overall_ok and ttft_ok:
    verdict="PASS_PREBOUND_PLAN_AND_RESIDUAL_INCREMENTAL_VALUE"
elif plan_ok and not residual_ok:
    verdict="VALID_NEGATIVE_RESIDUAL_ANL64_VALUE_ABSORBED_OR_INSUFFICIENT"
elif not plan_ok:
    verdict="VALID_NEGATIVE_PREBOUND_PLAN_HARM_BLOCKS_TRANSFER"
else:
    verdict="VALID_NEGATIVE_OVERALL_TRANSFER_NOT_ESTABLISHED"

out={
 "schema":"arcllm.anl64_crt.p1r.machine_adjudication.v0.1",
 "F0_freshness":True,
 "F1_structural_prebinding":True,
 "F2_exact_token_semantics":True,
 "F3_session_completeness":True,
 "F4_plan_no_harm":plan_ok,
 "F5_residual_incremental":residual_ok,
 "F6_overall_transfer":overall_ok,
 "F7_ttft_guard":ttft_ok,
 "comparisons":results,
 "verdict":verdict,
 "thresholds":{"decode_gain_min":1.1,"e2e_ratio_max":0.9,"ttft_ratio_max":1.1,"plan_no_harm_decode_min":0.9,"plan_no_harm_e2e_max":1.1},
 "invalid_P1_partial_reused":False,
 "llama_cpp_claim":False
}
(d/"MACHINE_ADJUDICATION.json").write_text(json.dumps(out,indent=2)+"\n")
print(verdict)
