import json, math
from pathlib import Path
D=Path(__file__).resolve().parent
raw=json.loads((D/"RAW_EVIDENCE.json").read_text())
pre=json.loads((D/"PREREGISTRATION.json").read_text())
complete=json.loads((D/"CAMPAIGN_COMPLETE.json").read_text())
recs=raw["records"]
warm=[r for r in recs if r["record"]["phase"]=="warmup"]
meas=[r for r in recs if r["record"]["phase"]=="measured"]
cells=[x["id"] for x in pre["workloads"]["cells"]]
def q(v,p):
 v=sorted(float(x) for x in v)
 h=(len(v)-1)*p
 lo,hi=math.floor(h),math.ceil(h)
 return v[lo]+(v[hi]-v[lo])*(h-lo)
def qnr(v,p):
 v=sorted(float(x) for x in v)
 return v[max(0,math.ceil(p*len(v))-1)]
def imp_low(a,b): return (a-b)/a*100 if a else 0
def imp_high(a,b): return (b-a)/a*100 if a else 0

expected=[]
for c in cells:
 for pair,o in enumerate(pre["workloads"]["measured_pair_order"]):
  arms=["direct","ring"] if o=="A_B" else ["ring","direct"]
  for pos,arm in enumerate(arms): expected.append((c,pair,pos,arm))
actual=[(r["record"]["cell"],r["record"]["pair"],r["record"]["pos"],r["record"]["arm"]) for r in meas]

sem_keys=["prefill_dispatches","prefill_submits","decode_dispatches_per_step","decode_submits_per_step","decode_steps","route_a_steps","route_b_steps","acquire_events","evict_events","b_allocations","b_materializations","b_validations","b_releases","p1_calls","p3_calls","p0_calls"]
mismatch=[]
for c in cells:
 for p in range(4):
  rs=[r for r in meas if r["record"]["cell"]==c and r["record"]["pair"]==p]
  A=next((r for r in rs if r["record"]["arm"]=="direct"),None)
  B=next((r for r in rs if r["record"]["arm"]=="ring"),None)
  issues=[]
  if not A or not B: issues.append("missing_arm")
  else:
   a,b=A["result"],B["result"]
   if a["generated_token_ids"]!=b["generated_token_ids"]: issues.append("tokens")
   if a["runtime_stats"]["finite"] is not True or b["runtime_stats"]["finite"] is not True: issues.append("finite")
   for k in sem_keys:
    if a["runtime_stats"].get(k)!=b["runtime_stats"].get(k): issues.append(k)
   if bool(a.get("error"))!=bool(b.get("error")): issues.append("asymmetric_error")
   if a.get("semantic_trace_consistent") is not True or b.get("semantic_trace_consistent") is not True: issues.append("trace")
  if issues: mismatch.append({"cell":c,"pair":p,"issues":issues})

missing=set()
itl_exact=True
max_tps_err=0.0
for r in meas:
 o=r["result"]
 for k in ["request_start_ns","request_end_ns","cpu_start_100ns","cpu_end_100ns"]:
  if k not in o: missing.add(k)
 t=o["token_ready_ns"]
 di=[t[i]-t[i-1] for i in range(1,len(t))]
 if di!=o["itl_ns"]: itl_exact=False
 rtps=(len(t)-1)/((t[-1]-t[0])/1e9) if len(t)>1 and t[-1]>t[0] else 0.0
 max_tps_err=max(max_tps_err,abs(rtps-float(o["metrics"]["decode_tokens_per_second"])))

metrics={}
for c in cells:
 rs=[r for r in meas if r["record"]["cell"]==c]
 A=[r for r in rs if r["record"]["arm"]=="direct"]
 B=[r for r in rs if r["record"]["arm"]=="ring"]
 cm={}
 for m in ["ttft_ns","e2e_ns","decode_tokens_per_second","cpu_utilization_percent","allocation_count_request","allocation_count_decode_window"]:
  av=[r["result"]["metrics"][m] for r in A]; bv=[r["result"]["metrics"][m] for r in B]
  a50,b50,a95,b95=q(av,.5),q(bv,.5),q(av,.95),q(bv,.95)
  high=m=="decode_tokens_per_second"
  cm[m]={"A_p50":a50,"B_p50":b50,"A_p95":a95,"B_p95":b95,
         "p50_improvement_percent":imp_high(a50,b50) if high else imp_low(a50,b50),
         "p95_improvement_percent":imp_high(a95,b95) if high else imp_low(a95,b95)}
 ia=[x for r in A for x in r["result"]["itl_ns"]]; ib=[x for r in B for x in r["result"]["itl_ns"]]
 cm["itl_ns"]={"A_p50":q(ia,.5),"B_p50":q(ib,.5),"A_p95":q(ia,.95),"B_p95":q(ib,.95),
               "p50_improvement_percent":imp_low(q(ia,.5),q(ib,.5)),
               "p95_improvement_percent":imp_low(q(ia,.95),q(ib,.95)),
               "nearest_rank_p95_improvement_percent":imp_low(qnr(ia,.95),qnr(ib,.95))}
 metrics[c]=cm

cands=[]; guards={"ttft_p95_gt3":[],"itl_p95_gt3":[],"decode_tps_gt3":[],"cpu_gt5":[]}; regl=[]; regnr=[]
for c,cm in metrics.items():
 for m in ["ttft_ns","e2e_ns","decode_tokens_per_second"]:
  if cm[m]["p50_improvement_percent"]>=5 and cm[m]["p95_improvement_percent"]>=5: cands.append({"cell":c,"endpoint":m})
 tr=-cm["ttft_ns"]["p95_improvement_percent"]; ir=-cm["itl_ns"]["p95_improvement_percent"]; tpr=-cm["decode_tokens_per_second"]["p50_improvement_percent"]
 cr50=-cm["cpu_utilization_percent"]["p50_improvement_percent"]; cr95=-cm["cpu_utilization_percent"]["p95_improvement_percent"]
 if tr>3: guards["ttft_p95_gt3"].append({"cell":c,"regression_percent":tr})
 if ir>3: guards["itl_p95_gt3"].append({"cell":c,"regression_percent":ir})
 if tpr>3: guards["decode_tps_gt3"].append({"cell":c,"regression_percent":tpr})
 if max(cr50,cr95)>5: guards["cpu_gt5"].append({"cell":c,"p50_regression_percent":cr50,"p95_regression_percent":cr95})
 if tr>10 or ir>10 or tpr>10: regl.append(c)
 irnr=-cm["itl_ns"]["nearest_rank_p95_improvement_percent"]
 if tr>10 or irnr>10 or tpr>10: regnr.append(c)

e2e=bool(cands) and not any(guards.values())
regression=len(set(regl))>=4
control=[r for r in recs if r["record"].get("phase")=="control_path_subtest"]
instrument_complete=not missing
formal_pass=(len(warm)==4 and len(meas)==48 and actual==expected and not mismatch and instrument_complete and bool(control))
diag="SEMANTICS_FAIL" if mismatch else ("REGRESSION" if regression else ("E2E_SUPPORTED" if e2e else "NO_SUPPORTED_BENEFIT"))
result={
 "schema":"arcllm.lmax_arch_p0.independent_qa.v0.1",
 "source":{"raw_schema":raw.get("schema"),"campaign_status":complete.get("status"),"records":len(recs),"warmups":len(warm),"measured":len(meas)},
 "execution_structure":{"pair_order_exact":actual==expected,"all_rc_zero":all(r["record"].get("rc")==0 for r in recs),"all_success_true":all(r["result"].get("success") is True for r in recs)},
 "semantic_gate":{"pass":not mismatch,"pairs_checked":24,"mismatch_count":len(mismatch),"mismatches":mismatch},
 "primitive_recomputation":{"itl_exact":itl_exact,"decode_tps_max_abs_error":max_tps_err,"missing_required_serialized_primitives":sorted(missing),"ttft_recomputable_from_primitives":False,"e2e_recomputable_from_primitives":False,"cpu_recomputable_from_primitives":False,"instrumentation_record_contract_complete":instrument_complete},
 "cell_metrics_linear_quantile":metrics,
 "decision_tree":{"e2e_candidates_before_guards":cands,"e2e_guard_violations":guards,"e2e_supported_if_stored_derived_metrics_accepted":e2e,"regression_cells_linear":sorted(set(regl)),"regression_cells_nearest_rank":sorted(set(regnr)),"regression_reached":regression,"control_path_subtest_records":len(control),"control_path_only_evaluable":bool(control),"diagnostic_category_if_stored_derived_metrics_accepted":diag},
 "formal_qa":{"pass":formal_pass,"blocking_findings":[]},
 "formal_adjudication":{"status":"UNRESOLVED","reason":"Frozen evidence omits preregistered request_start/request_end and CPU start/end primitives, preventing independent primitive recomputation of TTFT/E2E/CPU; the preregistered 1,000,000-event control-path subtest is also absent. No rerun or post-hoc subtest was performed.","diagnostic_only_category":diag,"lineage_eligible":True}
}
if not instrument_complete: result["formal_qa"]["blocking_findings"].append("PREREGISTERED_PER_REQUEST_PRIMITIVES_NOT_SERIALIZED")
if not control: result["formal_qa"]["blocking_findings"].append("PREREGISTERED_CONTROL_PATH_SUBTEST_NOT_EXECUTED")
(D/"INDEPENDENT_QA_RESULT.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print("QA_COMPLETE")
print("SEMANTIC_PASS="+str(not mismatch))
print("FORMAL_QA_PASS="+str(formal_pass))
print("FORMAL_VERDICT=UNRESOLVED")
print("DIAGNOSTIC_CATEGORY="+diag)
print("E2E_CANDIDATES="+json.dumps(cands))
print("CPU_GUARD_CELLS="+json.dumps([x["cell"] for x in guards["cpu_gt5"]]))
print("REGRESSION_LINEAR="+json.dumps(sorted(set(regl))))
print("REGRESSION_NEAREST="+json.dumps(sorted(set(regnr))))
print("BLOCKERS="+json.dumps(result["formal_qa"]["blocking_findings"]))
