from pathlib import Path
import json, hashlib, statistics, sys

root=Path(sys.argv[1])
manifest=json.loads((root/"MANIFEST.json").read_text(encoding="utf-8"))
assert manifest["status"]=="PASS_COMPLETE_FRESH_P1R_COLLECTION"
assert manifest["measured_attempts"]==60
assert manifest["invalid_p1_partial_reused"] is False
assert manifest["historical_p6_timing_reused"] is False
assert manifest["selective_rerun"] is False
assert len(manifest["cells"])==12

expected_orders={
"A":[("A_CANONICAL","W-S"),("B_PLAN_PREBOUND","W-S"),("C_PLAN_PREBOUND_RESIDUAL","W-S"),
     ("C_PLAN_PREBOUND_RESIDUAL","W-C"),("B_PLAN_PREBOUND","W-C"),("A_CANONICAL","W-C")],
"B":[("C_PLAN_PREBOUND_RESIDUAL","W-S"),("B_PLAN_PREBOUND","W-S"),("A_CANONICAL","W-S"),
     ("A_CANONICAL","W-C"),("B_PLAN_PREBOUND","W-C"),("C_PLAN_PREBOUND_RESIDUAL","W-C")]
}

cells={}
for row in manifest["cells"]:
    p=root/row["file"]
    raw=p.read_bytes()
    sha=hashlib.sha256(raw).hexdigest().upper()
    assert sha==row["sha256"], (row["file"],sha,row["sha256"])
    obj=json.loads(raw)
    assert obj["status"]=="PASS_CELL"
    assert obj["arm"]==row["arm"] and obj["workload"]==row["workload"]
    assert obj["implementation_commit"]=="72b917c44b580e752a89a7e3c5231e8dbfd21249"
    assert len(obj["attempts"])==5
    assert obj["runtime"]["prefill_dispatches"]==441
    assert obj["runtime"]["decode_dispatches_per_step"]==469
    assert obj["runtime"]["decode_steps"]==31
    assert obj["runtime"]["I002_gate_up_unchanged"] is True
    assert obj["runtime"]["Q4V4_down_unchanged"] is True
    if row["arm"]=="A_CANONICAL":
        assert obj["plan"]["enabled"] is False
    else:
        plan=obj["plan"]
        assert plan["enabled"] is True
        assert plan["prebound_before_timing"] is True
        assert plan["per_token_plan_lookup"] is False
        assert plan["nodes"]==469
        assert plan["quant_linear_nodes"]==215
        assert plan["fixed_q4_fast_nodes"]==140
        assert plan["regions"]==24104
        assert plan["metadata_bytes"]==593504
        assert plan["hash_fnv1a64"]=="04f3f884c0fc4fcc"
    for a in obj["attempts"]:
        assert a["success"] is True
        assert a["finite"] is True
        assert a["dispatch_pass"] is True
        assert a["lifecycle_pass"] is True
        assert len(a["generated_token_ids"])==32
    cells[(row["session"],row["arm"],row["workload"])]=obj

for sess,expected in expected_orders.items():
    rows=sorted((r for r in manifest["cells"] if r["session"]==sess),key=lambda x:x["order"])
    assert [(r["arm"],r["workload"]) for r in rows]==expected

for sess in ("A","B"):
    for work in ("W-S","W-C"):
        trio=[cells[(sess,arm,work)] for arm in ("A_CANONICAL","B_PLAN_PREBOUND","C_PLAN_PREBOUND_RESIDUAL")]
        for i in range(5):
            toks=[tuple(o["attempts"][i]["generated_token_ids"]) for o in trio]
            assert toks[0]==toks[1]==toks[2]

def med(obj,key):
    return statistics.median(float(a[key]) for a in obj["attempts"])

rows={}
plan_pass=True
residual_pass=True
overall_pass=True
ttft_pass=True
for sess in ("A","B"):
    rows[sess]={}
    for work in ("W-S","W-C"):
        A=cells[(sess,"A_CANONICAL",work)]
        B=cells[(sess,"B_PLAN_PREBOUND",work)]
        C=cells[(sess,"C_PLAN_PREBOUND_RESIDUAL",work)]
        x={
          "A":{"ttft_ms":med(A,"ttft_ms"),"decode_tps":med(A,"decode_tps"),"e2e_ms":med(A,"e2e_ms")},
          "B":{"ttft_ms":med(B,"ttft_ms"),"decode_tps":med(B,"decode_tps"),"e2e_ms":med(B,"e2e_ms")},
          "C":{"ttft_ms":med(C,"ttft_ms"),"decode_tps":med(C,"decode_tps"),"e2e_ms":med(C,"e2e_ms")}
        }
        x["B_over_A_decode"]=x["B"]["decode_tps"]/x["A"]["decode_tps"]
        x["B_over_A_e2e"]=x["B"]["e2e_ms"]/x["A"]["e2e_ms"]
        x["B_over_A_ttft"]=x["B"]["ttft_ms"]/x["A"]["ttft_ms"]
        x["C_over_B_decode"]=x["C"]["decode_tps"]/x["B"]["decode_tps"]
        x["C_over_B_e2e"]=x["C"]["e2e_ms"]/x["B"]["e2e_ms"]
        x["C_over_B_ttft"]=x["C"]["ttft_ms"]/x["B"]["ttft_ms"]
        x["C_over_A_decode"]=x["C"]["decode_tps"]/x["A"]["decode_tps"]
        x["C_over_A_e2e"]=x["C"]["e2e_ms"]/x["A"]["e2e_ms"]
        x["C_over_A_ttft"]=x["C"]["ttft_ms"]/x["A"]["ttft_ms"]
        x["plan_no_harm"]=x["B_over_A_decode"]>=0.9 and x["B_over_A_e2e"]<=1.1 and x["B_over_A_ttft"]<=1.1
        x["residual_material"]=x["C_over_B_decode"]>=1.1 and x["C_over_B_e2e"]<=0.9
        x["overall_material"]=x["C_over_A_decode"]>=1.1 and x["C_over_A_e2e"]<=0.9
        x["ttft_guard"]=x["C_over_A_ttft"]<=1.1
        plan_pass &= x["plan_no_harm"]
        residual_pass &= x["residual_material"]
        overall_pass &= x["overall_material"]
        ttft_pass &= x["ttft_guard"]
        rows[sess][work]=x

if plan_pass and residual_pass and overall_pass and ttft_pass:
    verdict="PASS_PREBOUND_PLAN_AND_RESIDUAL_INCREMENTAL_VALUE"
elif not plan_pass:
    verdict="VALID_NEGATIVE_PREBOUND_PLAN_HARM_BLOCKS_TRANSFER"
elif not residual_pass:
    verdict="VALID_NEGATIVE_RESIDUAL_ANL64_VALUE_ABSORBED_OR_INSUFFICIENT"
else:
    verdict="VALID_NEGATIVE_OVERALL_TRANSFER_NOT_ESTABLISHED"

out={
  "schema":"arcllm.anl64_crt.p1r.independent_adjudication.v0.1",
  "evidence_source":"fresh_collection_only",
  "manifest_status":manifest["status"],
  "manifest_cell_count":len(manifest["cells"]),
  "measured_attempts":manifest["measured_attempts"],
  "cell_sha256_all_match_manifest":True,
  "F0_freshness":True,
  "F1_structural_prebinding":True,
  "F2_exact_token_semantics":True,
  "F3_session_completeness":True,
  "F4_plan_no_harm":plan_pass,
  "F5_residual_incremental":residual_pass,
  "F6_overall_transfer":overall_pass,
  "F7_ttft_guard":ttft_pass,
  "comparisons":rows,
  "verdict":verdict,
  "criteria_rederived_from_frozen_gate":{
    "plan_decode_min":0.9,
    "plan_e2e_max":1.1,
    "plan_ttft_max":1.1,
    "residual_decode_min":1.1,
    "residual_e2e_max":0.9,
    "overall_decode_min":1.1,
    "overall_e2e_max":0.9,
    "overall_ttft_max":1.1
  },
  "machine_adjudication_file_read":False,
  "invalid_P1_partial_reused":False,
  "historical_P6_timing_reused":False
}
(root/"INDEPENDENT_ADJUDICATION.json").write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print(verdict)
