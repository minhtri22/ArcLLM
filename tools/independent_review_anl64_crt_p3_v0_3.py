from pathlib import Path
import collections, hashlib, json, py_compile, subprocess

root=Path(__file__).resolve().parents[1]
schedule=json.loads((root/"config/anl64_crt_p3_blocked_randomized_schedule_v0.3.json").read_text())
pre=json.loads((root/"config/anl64_crt_p3_blocked_randomized_state_aware_replication_prelock_v0.3.json").read_text())
cov=json.loads((root/"artifacts/ANL64_CRT/ANL64_CRT_P3_COVARIATE_SURFACE_LOCK_v0.2.json").read_text())
p1=json.loads((root/"config/anl64_crt_p1r_prebound_plan_causal_separation_gate_v0.1.json").read_text())
runner=(root/"run_anl64_crt_p3.ps1").read_text()
collector=(root/"tools/anl64_crt_p3_precell_state_v0_2.ps1").read_text()
adjud=(root/"tools/adjudicate_anl64_crt_p3.py").read_text()

findings=[]
def check(cond, fid, msg):
    if not cond: findings.append({"id":fid,"finding":msg})

# Reproduce hash randomization independently.
seed=20260928
prefix="ANL64_CRT_P3"
conds=["A_CANONICAL/W-S","B_PLAN_PREBOUND/W-S","C_PLAN_PREBOUND_RESIDUAL/W-S",
       "A_CANONICAL/W-C","B_PLAN_PREBOUND/W-C","C_PLAN_PREBOUND_RESIDUAL/W-C"]
base=[[1,2,6,3,5,4],[2,3,1,4,6,5],[3,4,2,5,1,6],[4,5,3,6,2,1],[5,6,4,1,3,2],[6,1,5,2,4,3]]
def H(kind,item):
    return hashlib.sha256(f"{prefix}|{seed}|{kind}|{item}".encode()).hexdigest()
order=sorted(conds,key=lambda c:H("condition",c))
mapping={i+1:c for i,c in enumerate(order)}
row_order=sorted(range(6),key=lambda i:H("row",i))
regen=[[mapping[x] for x in base[i]] for i in row_order]
rows=[b["order"] for b in schedule["blocks"]]
check(rows==regen,"P3-V3-01","schedule does not reproduce from frozen hash algorithm")

# Exact block/position/within-block carryover balance.
flat=sum(rows,[])
check(len(rows)==6 and all(len(r)==6 and len(set(r))==6 for r in rows),"P3-V3-02","not six complete blocks")
pos={c:collections.Counter() for c in conds}
pairs=collections.Counter()
for r in rows:
    for i,c in enumerate(r,1): pos[c][i]+=1
    pairs.update(zip(r,r[1:]))
check(all(pos[c]==collections.Counter({1:1,2:1,3:1,4:1,5:1,6:1}) for c in conds),"P3-V3-03","ordinal-position balance failed")
check(len(pairs)==30 and set(pairs.values())=={1},"P3-V3-04","within-block ordered-pair balance failed")
check(schedule["balance_claims"]["cross_block_transitions"]==5 and schedule["balance_claims"]["cross_block_transitions_are_part_of_Williams_balance_claim"] is False,
      "P3-V3-05","cross-block carryover scope not explicit")

# Threshold provenance exact match to P1R.
t=pre["frozen_materiality_thresholds"]; p=p1["thresholds"]
check(t["decode_benefit_min"]==p["residual_decode_C_over_B_min"]==p["overall_decode_C_over_A_min"],"P3-V3-06","decode threshold drift")
check(t["e2e_latency_ratio_max"]==p["residual_e2e_C_over_B_max"]==p["overall_e2e_C_over_A_max"],"P3-V3-07","E2E threshold drift")
check(t["ttft_harm_ratio_max"]==p["ttft_harm_max"],"P3-V3-08","TTFT threshold drift")
check(t["plan_no_harm_decode_min"]==p["plan_prebound_decode_B_over_A_no_harm_min"],"P3-V3-09","plan decode no-harm threshold drift")
check(t["plan_no_harm_e2e_max"]==p["plan_prebound_e2e_B_over_A_no_harm_max"],"P3-V3-10","plan E2E no-harm threshold drift")

# Covariate surface and exact collector identity.
blob=lambda rel: subprocess.check_output(["git","-C",str(root),"rev-parse","HEAD:"+rel],text=True).strip()
check(blob("tools/anl64_crt_p3_precell_state_v0_2.ps1")==cov["collector"]["blob"],"P3-V3-11","collector blob mismatch")
for field in cov["required_fields"]:
    check(field in collector,"P3-V3-12-"+field,"required covariate absent from collector")
check("environment_invariants_pass" in collector and "381b4222-f694-41f0-9685-ff5bb260df2e" in collector,"P3-V3-13","environment invariant implementation missing")
check("Token-XRay" not in collector and "VTune" not in collector,"P3-V3-14","intrusive profiler reference in collector")

# Stop rules and runner enforcement.
for key in ("before_cell_environment","completed_cell_semantic_or_structural_failure","infrastructure_interruption_before_valid_cell_artifact","valid_completed_cell","schedule_deviation","adaptive_behavior"):
    check(key in pre["stop_rules"],"P3-V3-15-"+key,"stop rule missing")
for needle in ("P3 execution not authorized","locked git blob mismatch","runtime executable SHA256 mismatch","locked shader SHA256 mismatch",
               "output directory is not empty; no resume/overwrite allowed","precell state collector failed",
               "environment invariant failed","valid_completed_cell_reruns=0","selective_reruns=0","adaptive_reordering=$false"):
    check(needle in runner,"P3-V3-16-"+needle,"runner does not enforce frozen rule")
for forbidden in ("Remove-Item","Start-Sleep","powercfg /S","Set-ItemProperty"):
    check(forbidden not in runner,"P3-V3-17-"+forbidden,"adaptive/mutating operation found in runner")

# Outcome classes mutually exclusive and no ANL64-specific attribution.
oc=pre["mutually_exclusive_outcome_classes"]
expected={"PASS_TOTAL_SPLIT_AND_DECOMPOSITION_IDENTIFIED","PASS_TOTAL_SPLIT_DECOMPOSITION_REMAINS_UNDERIDENTIFIED",
          "FAIL_TOTAL_SPLIT_NOT_REPLICATED","FAIL_TTFT_HARM",
          "STOP_SEMANTIC_OR_STRUCTURAL_GUARD_FAILURE_NO_PERFORMANCE_CLAIM","ENVIRONMENT_OR_INFRASTRUCTURE_STOP_NO_SCIENTIFIC_RESULT"}
check(set(oc)==expected,"P3-V3-18","outcome class set is not the frozen mutually-exclusive set")
check(pre["nuisance_analysis"]["ANL64_specific_instability_claim_licensed"] is False,"P3-V3-19","unsupported ANL64-specific attribution remains licensed")

# Adjudicator syntax and explicit verdict precedence.
py_compile.compile(str(root/"tools/adjudicate_anl64_crt_p3.py"),doraise=True)
for verdict in ("FAIL_TOTAL_SPLIT_NOT_REPLICATED","FAIL_TTFT_HARM","PASS_TOTAL_SPLIT_AND_DECOMPOSITION_IDENTIFIED","PASS_TOTAL_SPLIT_DECOMPOSITION_REMAINS_UNDERIDENTIFIED"):
    check(verdict in adjud,"P3-V3-20-"+verdict,"adjudicator missing outcome")
check(pre["execution"]["fresh_execution_authorized"] is False,"P3-V3-21","prelock prematurely authorizes execution")

out={
 "schema":"arcllm.anl64_crt.p3.independent_preoutcome_review.v0.3",
 "date":"2026-09-28",
 "fresh_P3_outcomes_observed":False,
 "reviewed":{"schedule_blob":blob("config/anl64_crt_p3_blocked_randomized_schedule_v0.3.json"),
             "prelock_blob":blob("config/anl64_crt_p3_blocked_randomized_state_aware_replication_prelock_v0.3.json"),
             "covariate_lock_blob":blob("artifacts/ANL64_CRT/ANL64_CRT_P3_COVARIATE_SURFACE_LOCK_v0.2.json"),
             "runner_blob":blob("run_anl64_crt_p3.ps1"),
             "adjudicator_blob":blob("tools/adjudicate_anl64_crt_p3.py")},
 "checks":{"randomization_reproduced":True,"position_balance":True,"within_block_carryover_balance":True,
           "cross_block_scope_explicit":True,"threshold_provenance_exact":True,"collector_identity_exact":True,
           "stop_rules_enforced":True,"shader_lock_enforced":True,"outcome_classes_mutually_exclusive":True,"adjudicator_syntax_valid":True} if not findings else {},
 "findings":findings,
 "open_finding_count":len(findings),
 "execution_lock_permitted":len(findings)==0
}
(root/"artifacts/ANL64_CRT/ANL64_CRT_P3_INDEPENDENT_PREOUTCOME_REVIEW_v0.3.json").write_text(json.dumps(out,indent=2)+"\n")
print(json.dumps(out,indent=2))
