import argparse,json,hashlib
from pathlib import Path

def load(path):
    with open(path,"r",encoding="utf-8") as f:
        return json.load(f)

ALLOWED_ATTEMPT_FIELDS=[
    "success","final_logits_finite","dispatch_census_pass",
    "generated_token_ids","generated_hash_fnv1a64",
    "final_logits_hash_fnv1a64","final_hidden_hash_fnv1a64"
]

def attempt_view(a):
    return {k:a.get(k) for k in ALLOWED_ATTEMPT_FIELDS}

def cell_view(obj, system, workload):
    attempts=obj.get("attempts")
    if not isinstance(attempts,list):
        attempts=[]
    return {
        "system":system,
        "workload":workload,
        "schema":obj.get("schema"),
        "status":obj.get("status","OK" if attempts else "MISSING_ATTEMPTS"),
        "attempt_count":len(attempts),
        "attempts":[attempt_view(a) for a in attempts],
        "anl64_plan":obj.get("anl64_plan") if system=="ANL64_P4_LOCKED" else None,
    }

def stable(vals):
    return bool(vals) and all(v==vals[0] for v in vals)

def good_attempt(a):
    return a.get("success") is True and a.get("final_logits_finite") is True and a.get("dispatch_census_pass") is True

def sha256(path):
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for chunk in iter(lambda:f.read(1024*1024),b""):
            h.update(chunk)
    return h.hexdigest().upper()

ap=argparse.ArgumentParser()
ap.add_argument("--reference-ws",required=True)
ap.add_argument("--candidate-ws",required=True)
ap.add_argument("--reference-wc",required=True)
ap.add_argument("--candidate-wc",required=True)
ap.add_argument("--out",required=True)
args=ap.parse_args()

raw={
    ("ARC_SAFE_REFERENCE","W-S"):load(args.reference_ws),
    ("ANL64_P4_LOCKED","W-S"):load(args.candidate_ws),
    ("ARC_SAFE_REFERENCE","W-C"):load(args.reference_wc),
    ("ANL64_P4_LOCKED","W-C"):load(args.candidate_wc),
}
cells={f"{s}/{w}":cell_view(o,s,w) for (s,w),o in raw.items()}

f1=True
for c in cells.values():
    if c["attempt_count"]!=5 or not all(good_attempt(a) for a in c["attempts"]):
        f1=False

plans=[cells["ANL64_P4_LOCKED/W-S"]["anl64_plan"],cells["ANL64_P4_LOCKED/W-C"]["anl64_plan"]]
plan_checks=[]
for p in plans:
    ok=isinstance(p,dict)
    if ok:
        ok = (
            p.get("nodes")==469 and
            p.get("quant_linear_nodes")==215 and
            p.get("fixed_q4_fast_nodes")==140 and
            p.get("regions")==24104 and
            p.get("fixed_q4_regions")==19936 and
            isinstance(p.get("metadata_bytes"),int) and p.get("metadata_bytes")<=2097152 and
            isinstance(p.get("hash_fnv1a64"),str) and p.get("hash_fnv1a64") not in ("","0000000000000000")
        )
    plan_checks.append(ok)
plan_hash_equal=all(plan_checks) and plans[0]["hash_fnv1a64"]==plans[1]["hash_fnv1a64"]
f1=f1 and all(plan_checks) and plan_hash_equal

f2=True
f3=True
pair_details=[]
for w in ("W-S","W-C"):
    r=cells[f"ARC_SAFE_REFERENCE/{w}"]["attempts"]
    c=cells[f"ANL64_P4_LOCKED/{w}"]["attempts"]
    if len(r)!=5 or len(c)!=5:
        f2=f3=False
        pair_details.append({"workload":w,"pair_count":min(len(r),len(c)),"prefill_equal":False,"full_sequence_equal":False})
        continue
    pref=[]
    full=[]
    for i in range(5):
        rs=r[i].get("generated_token_ids")
        cs=c[i].get("generated_token_ids")
        p=isinstance(rs,list) and isinstance(cs,list) and len(rs)==32 and len(cs)==32 and rs[0]==cs[0]
        q=isinstance(rs,list) and isinstance(cs,list) and len(rs)==32 and len(cs)==32 and rs==cs
        pref.append(p);full.append(q)
    f2=f2 and all(pref)
    f3=f3 and all(full)
    pair_details.append({
        "workload":w,
        "pair_count":5,
        "prefill_equal_by_attempt":pref,
        "full_sequence_equal_by_attempt":full,
        "token_mismatch_attempt_indices":[i for i,x in enumerate(full) if not x],
    })

f4=True
repeatability={}
for key,c in cells.items():
    ats=c["attempts"]
    seqs=[a.get("generated_token_ids") for a in ats]
    lh=[a.get("final_logits_hash_fnv1a64") for a in ats]
    hh=[a.get("final_hidden_hash_fnv1a64") for a in ats]
    ok=len(ats)==5 and stable(seqs) and stable(lh) and stable(hh)
    repeatability[key]={
        "generated_sequence_stable":len(ats)==5 and stable(seqs),
        "final_logits_hash_stable":len(ats)==5 and stable(lh),
        "final_hidden_hash_stable":len(ats)==5 and stable(hh),
    }
    f4=f4 and ok

out={
    "schema":"arcllm.anl64.p5.semantic_evidence.v0.1",
    "status":"P5_EXECUTION_COMPLETE_AWAITING_INDEPENDENT_ADJUDICATION",
    "performance_fields_extracted":False,
    "raw_timing_decision_role":"NONE_SPENT_FOR_P6",
    "cells":cells,
    "gate_observations":{
        "F1_structure":f1,
        "F2_prefill_control":f2,
        "F3_decode_semantics":f3,
        "F4_repeatability":f4,
        "all_frozen_semantic_gates_observed_pass":f1 and f2 and f3 and f4,
        "pair_details":pair_details,
        "repeatability":repeatability,
        "candidate_plan_hash_equal_across_workloads":plan_hash_equal,
    },
    "raw_sha256":{
        "reference_ws":sha256(args.reference_ws),
        "candidate_ws":sha256(args.candidate_ws),
        "reference_wc":sha256(args.reference_wc),
        "candidate_wc":sha256(args.candidate_wc),
    }
}
Path(args.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print("ANL64 P5 semantic extraction complete; performance fields were not copied.")
