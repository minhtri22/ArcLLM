import argparse,json,math,random,statistics
from pathlib import Path
ORDER=[
["0","A","B","AB"],["A","B","AB","0"],["B","AB","0","A"],["AB","0","A","B"],
["AB","B","A","0"],["0","AB","B","A"],["A","0","AB","B"],["B","A","0","AB"]]
EXPECTED={"W-S":"f31d4bb9fe5eb9c3","W-C":"471519ddc45b232e"}
BOOT=10000;SEED=1980
def req(c,m):
    if not c: raise SystemExit("Q4 4-arm timing adjudication STOP: "+m)
def pct(v,p):
    x=sorted(v);q=(len(x)-1)*p;lo=int(math.floor(q));hi=int(math.ceil(q))
    return x[lo] if lo==hi else x[lo]*(hi-q)+x[hi]*(q-lo)
def med(v): return statistics.median(v)
def load(path,w):
    d=json.loads(Path(path).read_text(encoding="utf-8"))
    req(d.get("schema")=="arcllm.v1.q4_down.4arm.primary_timing.collection.v0.1",w+" schema")
    req(d.get("status")=="PASS_PRIMARY_TIMING_COLLECTION",w+" status")
    req(d.get("workload")==w,w+" identity")
    m=d.get("measurement",{})
    req(m.get("primary_metric")=="Q4_DOWN_COMPONENT_LATENCY_MS_PER_TOKEN",w+" metric")
    req(m.get("target_dispatches_per_decode")==14 and m.get("decode_samples_per_attempt")==31,w+" sample census")
    req(m.get("hardware_counters_executed") is False and m.get("token_xray_executed") is False,w+" forbidden instrumentation")
    c=d.get("correctness_recheck",{})
    req(c.get("expected_generated_hash_fnv1a64")==EXPECTED[w],w+" expected hash")
    ats=d.get("attempts",[]);req(len(ats)==32,w+" attempt count")
    idx={}
    for a in ats:
        key=(int(a["block"]),int(a["ordinal"]))
        req(key not in idx,w+" duplicate block/ordinal")
        idx[key]=a
    for b in range(8):
        for o,arm in enumerate(ORDER[b]):
            a=idx.get((b,o));req(a is not None,w+f" missing {b}/{o}")
            req(a.get("arm")==arm,w+f" arm order {b}/{o}")
            req(a.get("profiled") is True and a.get("success") is True and a.get("dispatch_census_pass") is True,w+" attempt invariant")
            req(a.get("generated_hash_fnv1a64")==EXPECTED[w],w+" semantic drift")
            q=a.get("q4_down_ms_by_decode",[]);req(len(q)==31 and all(float(x)>0 and math.isfinite(float(x)) for x in q),w+" raw timing")
            v=float(a["component_latency_ms_per_token"]);req(v>0 and math.isfinite(v),w+" primary timing")
            req(abs(v-med([float(x) for x in q]))<=max(1e-12,abs(v)*1e-10),w+" median recompute")
    return d,idx
def cells(idx,blocks):
    out={a:[] for a in ["0","A","B","AB"]}
    for b in blocks:
        for o,a in enumerate(ORDER[b]): out[a].append(float(idx[(b,o)]["component_latency_ms_per_token"]))
    return {a:med(v) for a,v in out.items()}
def derive(c):
    return {
      "A_over_0":c["A"]/c["0"],"B_over_0":c["B"]/c["0"],
      "AB_over_A":c["AB"]/c["A"],"AB_over_B":c["AB"]/c["B"],
      "G_A_ms":c["0"]-c["A"],"G_B_ms":c["0"]-c["B"],
      "G_INT_ms":c["A"]+c["B"]-c["0"]-c["AB"],
      "g_A":math.log(c["0"]/c["A"]),"g_B":math.log(c["0"]/c["B"]),
      "g_INT":math.log((c["A"]*c["B"])/(c["0"]*c["AB"]))
    }
def bootstrap(idx,seed):
    rng=random.Random(seed);rows=[]
    for _ in range(BOOT):
        bs=[rng.randrange(8) for __ in range(8)]
        rows.append(derive(cells(idx,bs)))
    keys=rows[0].keys()
    return {k:[pct([r[k] for r in rows],.025),pct([r[k] for r in rows],.975)] for k in keys}
ap=argparse.ArgumentParser();ap.add_argument("--w-s",required=True);ap.add_argument("--w-c",required=True);ap.add_argument("--out",required=True)
a=ap.parse_args();data={};idxs={}
for w,p in [("W-S",a.w_s),("W-C",a.w_c)]: data[w],idxs[w]=load(p,w)
res={}
for wi,w in enumerate(["W-S","W-C"]):
    c=cells(idxs[w],list(range(8)));res[w]={"cell_medians_ms":c,"effects":derive(c),"ci95":bootstrap(idxs[w],SEED+wi*100)}
tmat=float(data["W-S"]["architecture_cost"]["materialization_time_ms"])
vmat=float(data["W-S"]["architecture_cost"]["validation_time_ms"])
req(data["W-S"]["architecture_cost"]["role"]=="CANONICAL_REFERENCE","W-S architecture-cost role")
req(data["W-C"]["architecture_cost"]["role"]=="INTEGRITY_REPLICATE_ONLY","W-C architecture-cost role")
for w in ["W-S","W-C"]:
    c=res[w]["cell_medians_ms"]
    be={}
    be["B_vs_0_tokens"]=tmat/(c["0"]-c["B"]) if c["B"]<c["0"] else None
    be["AB_vs_A_tokens"]=tmat/(c["A"]-c["AB"]) if c["AB"]<c["A"] else None
    res[w]["architecture_cost_break_even"]=be
out={
 "schema":"arcllm.v1.q4_down.4arm.primary_timing.adjudication.v0.1",
 "status":"PASS_PRIMARY_TIMING_RECOMPUTE",
 "bootstrap":{"resamples":BOOT,"seed":SEED,"paired_unit":"block","ci":"95% percentile"},
 "workloads":res,
 "architecture_cost":{"canonical_materialization_time_ms":tmat,"canonical_validation_time_ms":vmat,"extra_ram_bytes":549527552},
 "claim_boundary":{"timing_only":True,"mechanism_supported_A_B_claims_allowed":False,"native_counter_campaign_executed":False,
                   "cross_workload_pooling":"DESCRIPTIVE_ONLY","winner_forced":False},
 "next":"FREEZE_PRIMARY_TIMING_EVIDENCE_THEN_SEPARATELY_AUTHORIZE_PREREGISTERED_NATIVE_COUNTER_CAMPAIGN"
}
Path(a.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print("Q4_DOWN_4ARM_PRIMARY_TIMING_RECOMPUTE=PASS")
