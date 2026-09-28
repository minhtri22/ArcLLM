from pathlib import Path
import json,math,statistics,sys

root=Path(sys.argv[1])
pr=json.loads((root/"config/arcllm_v1_bounded_npu_ffn_down_transfer_prereg_v0.1.json").read_text())
ev=json.loads((root/"results/ARCLLM_V1_BOUNDED_NPU_FFN_DOWN_TRANSFER_EXECUTION_v0.1.json").read_text())

assert ev["status"]=="PASS_COMPLETE_BOUNDED_EXECUTION"
assert len(ev["layers"])==6
assert len(ev["cases"])==18
expected={(3,"Q4_K"),(14,"Q4_K"),(22,"Q4_K"),(0,"Q6_K"),(16,"Q6_K"),(27,"Q6_K")}
assert {(int(x["layer"]),x["qtype"]) for x in ev["layers"]}==expected
assert all(len(c["infer_ms"])==9 for c in ev["cases"])

max_abs_thr=float(pr["numerical_reference"]["primary_end_to_end_checks"]["max_abs_max"])
rmse_thr=float(pr["numerical_reference"]["primary_end_to_end_checks"]["rmse_max"])
num=[
    bool(c["finite"]) and
    float(c["end_to_end_error_vs_exact_quant_reference"]["max_abs"])<=max_abs_thr and
    float(c["end_to_end_error_vs_exact_quant_reference"]["rmse"])<=rmse_thr
    for c in ev["cases"]
]
assert all(bool(c["numerical_pass"])==v for c,v in zip(ev["cases"],num))

case_med=[]
for c in ev["cases"]:
    m=float(statistics.median(float(x) for x in c["infer_ms"]))
    assert abs(m-float(c["infer_median_ms"]))<1e-9
    case_med.append(m)
ys=sorted(case_med)
idx=max(0,min(len(ys)-1,math.ceil(0.95*len(ys))-1))
p95=ys[idx]
fam=28.0*p95
bud=pr["warm_transfer_metric"]["frozen_current_GPU_family_budget_ms_per_token"]
positive=fam<min(float(bud["W-S"]),float(bud["W-C"]))
material=fam<=0.90*min(float(bud["W-S"]),float(bud["W-C"]))

if not all(num): verdict="FAIL_NUMERICAL_SEMANTICS"
elif not positive: verdict="FAIL_WARM_TRANSFER_BUDGET"
elif material: verdict="PASS_REAL_WEIGHT_WARM_TRANSFER_MATERIAL"
else: verdict="PASS_REAL_WEIGHT_WARM_TRANSFER"

assert abs(p95-float(ev["aggregate"]["case_median_ms_p95_guard"]))<1e-9
assert abs(fam-float(ev["aggregate"]["family_28_layer_p95_ms_per_token"]))<1e-9
assert positive==bool(ev["aggregate"]["positive_budget_rule_pass"])
assert material==bool(ev["aggregate"]["material_10pct_rule_pass"])
assert verdict==ev["formal_execution_verdict"]

out={
 "schema":"arcllm.v1.bounded_npu_ffn_down_transfer.independent_adjudication.v0.1",
 "status":"PASS_INDEPENDENT_RECOMPUTATION",
 "complete_cases":18,
 "all_numerical_cases_pass":all(num),
 "max_end_to_end_max_abs":max(float(c["end_to_end_error_vs_exact_quant_reference"]["max_abs"]) for c in ev["cases"]),
 "max_end_to_end_rmse":max(float(c["end_to_end_error_vs_exact_quant_reference"]["rmse"]) for c in ev["cases"]),
 "case_median_ms_primary":float(statistics.median(case_med)),
 "case_median_ms_p95_guard":p95,
 "family_28_layer_p95_ms_per_token":fam,
 "GPU_family_budget_ms_per_token":bud,
 "positive_budget_rule_pass":positive,
 "material_10pct_rule_pass":material,
 "recomputed_verdict":verdict,
 "machine_independent_agreement":True
}
(root/"results/ARCLLM_V1_BOUNDED_NPU_FFN_DOWN_TRANSFER_INDEPENDENT_ADJUDICATION_v0.1.json").write_text(json.dumps(out,indent=2)+"\n")
print(json.dumps(out,indent=2))
