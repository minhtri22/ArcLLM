import argparse,json,statistics,hashlib
from pathlib import Path

EXPECTED_ORDERS={
 "A":["ARC_SAFE_REFERENCE/W-S","ANL64_P4_LOCKED/W-S","ANL64_P4_LOCKED/W-C","ARC_SAFE_REFERENCE/W-C"],
 "B":["ANL64_P4_LOCKED/W-S","ARC_SAFE_REFERENCE/W-S","ARC_SAFE_REFERENCE/W-C","ANL64_P4_LOCKED/W-C"],
}

def load(p):
    with open(p,"r",encoding="utf-8") as f: return json.load(f)

def sha256(p):
    h=hashlib.sha256()
    with open(p,"rb") as f:
        for b in iter(lambda:f.read(1024*1024),b""): h.update(b)
    return h.hexdigest().upper()

def median_attempt(cell,key):
    vals=[float(a[key]) for a in cell["attempts"]]
    if len(vals)!=5: raise ValueError(f"{key}: expected 5 measured attempts")
    return statistics.median(vals)

def semantic_ok(ref,cand):
    if len(ref.get("attempts",[]))!=5 or len(cand.get("attempts",[]))!=5: return False
    plan=cand.get("anl64_plan")
    if not isinstance(plan,dict): return False
    if not (
        plan.get("nodes")==469 and
        plan.get("quant_linear_nodes")==215 and
        plan.get("fixed_q4_fast_nodes")==140 and
        plan.get("regions")==24104 and
        plan.get("fixed_q4_regions")==19936 and
        int(plan.get("metadata_bytes",999999999))<=2097152 and
        plan.get("hash_fnv1a64")=="04f3f884c0fc4fcc"
    ): return False
    for r,c in zip(ref["attempts"],cand["attempts"]):
        if not (r.get("success") is True and c.get("success") is True): return False
        if not (r.get("final_logits_finite") is True and c.get("final_logits_finite") is True): return False
        if not (r.get("dispatch_census_pass") is True and c.get("dispatch_census_pass") is True): return False
        rs,cs=r.get("generated_token_ids"),c.get("generated_token_ids")
        if not (isinstance(rs,list) and isinstance(cs,list) and len(rs)==32 and len(cs)==32 and rs==cs): return False
    return True

ap=argparse.ArgumentParser()
ap.add_argument("--root",required=True)
ap.add_argument("--out",required=True)
args=ap.parse_args()
root=Path(args.root)

sessions={}
for s in ("A","B"):
    sd=root/f"session_{s}"
    meta=load(sd/"P6_SESSION_META.json")
    env=load(sd/"P6_SESSION_ENVIRONMENT.json")
    if meta.get("session")!=s or meta.get("execution_order")!=EXPECTED_ORDERS[s]:
        raise SystemExit(f"session {s} order/meta mismatch")
    cells={}
    for syskey,work,filekey in [
        ("ARC_SAFE_REFERENCE","W-S","reference_W_S.json"),
        ("ANL64_P4_LOCKED","W-S","candidate_W_S.json"),
        ("ARC_SAFE_REFERENCE","W-C","reference_W_C.json"),
        ("ANL64_P4_LOCKED","W-C","candidate_W_C.json"),
    ]:
        cells[(syskey,work)]=load(sd/filekey)
    sessions[s]={"meta":meta,"env":env,"cells":cells}

pid_distinct=sessions["A"]["meta"]["runner_process_id"]!=sessions["B"]["meta"]["runner_process_id"]
comparisons={}
semantic_all=True
decode_all=True
e2e_all=True
ttft_all=True
for s in ("A","B"):
    for w in ("W-S","W-C"):
        ref=sessions[s]["cells"][("ARC_SAFE_REFERENCE",w)]
        cand=sessions[s]["cells"][("ANL64_P4_LOCKED",w)]
        sem=semantic_ok(ref,cand)
        semantic_all &= sem
        rdec=median_attempt(ref,"decode_tps")
        cdec=median_attempt(cand,"decode_tps")
        re2e=median_attempt(ref,"e2e_ms")
        ce2e=median_attempt(cand,"e2e_ms")
        rttft=median_attempt(ref,"ttft_ms")
        cttft=median_attempt(cand,"ttft_ms")
        dr=cdec/rdec
        er=ce2e/re2e
        tr=cttft/rttft
        dp=dr>=1.10
        ep=er<=0.90
        tp=tr<=1.10
        decode_all &= dp
        e2e_all &= ep
        ttft_all &= tp
        comparisons[f"{s}/{w}"]={
            "semantic_guard":sem,
            "reference":{"decode_tps_median":rdec,"e2e_ms_median":re2e,"ttft_ms_median":rttft},
            "candidate":{"decode_tps_median":cdec,"e2e_ms_median":ce2e,"ttft_ms_median":cttft},
            "ratios":{"decode_candidate_over_reference":dr,"e2e_candidate_over_reference":er,"ttft_candidate_over_reference":tr},
            "threshold_pass":{"decode":dp,"e2e":ep,"ttft_blocking":tp},
        }

valid_sessions=pid_distinct and all(
    sessions[s]["meta"].get("measured_attempts_expected")==20 and
    sessions[s]["meta"].get("warmups_per_cell")==1 and
    sessions[s]["meta"].get("measured_attempts_per_cell")==5
    for s in ("A","B")
)

if not semantic_all:
    candidate_classification="P6_SEMANTIC_OR_STRUCTURAL_FAIL"
elif not valid_sessions:
    candidate_classification="P6_INVALID_F0"
elif decode_all and e2e_all and ttft_all:
    candidate_classification="P6_MATCHED_E2E_PASS"
else:
    candidate_classification="P6_NO_MATERIAL_E2E_BENEFIT"

out={
 "schema":"arcllm.anl64.p6.candidate_result.v0.1",
 "status":"P6_EXECUTION_COMPLETE_AWAITING_INDEPENDENT_ADJUDICATION",
 "candidate_classification":candidate_classification,
 "session_processes_distinct":pid_distinct,
 "semantic_structural_guard_all":semantic_all,
 "decode_material_benefit_all_four":decode_all,
 "e2e_material_benefit_all_four":e2e_all,
 "ttft_blocking_guard_all_four":ttft_all,
 "comparisons":comparisons,
 "session_meta_sha256":{s:sha256(root/f"session_{s}"/"P6_SESSION_META.json") for s in ("A","B")},
 "session_environment_sha256":{s:sha256(root/f"session_{s}"/"P6_SESSION_ENVIRONMENT.json") for s in ("A","B")},
 "raw_result_sha256":{
   s:{
     n:sha256(root/f"session_{s}"/n) for n in [
       "reference_W_S.json","candidate_W_S.json","reference_W_C.json","candidate_W_C.json"
     ]
   } for s in ("A","B")
 },
 "claim_boundary":{
   "successor_vs_safe_reference_only":True,
   "llama_cpp_advantage_claimed":False,
   "p5_timings_used":False
 }
}
Path(args.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print(candidate_classification)
