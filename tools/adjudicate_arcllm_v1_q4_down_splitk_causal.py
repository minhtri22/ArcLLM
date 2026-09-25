import argparse,json,math,random,statistics
from pathlib import Path

WORKLOADS=["W-S","W-C"]
BLOCKS=8
BOOTSTRAPS=10000
SEED=1980
EXPECTED_HASH={"W-S":"f31d4bb9fe5eb9c3","W-C":"471519ddc45b232e"}

def req(c,msg):
    if not c:
        raise SystemExit("Q4-down causal adjudication STOP: "+msg)

def gmean(xs):
    req(bool(xs),"empty ratio vector")
    req(all(isinstance(x,(int,float)) and math.isfinite(float(x)) and float(x)>0 for x in xs),"non-positive/non-finite ratio")
    return math.exp(sum(math.log(float(x)) for x in xs)/len(xs))

def percentile(xs,p):
    ys=sorted(xs)
    if not ys:
        return None
    x=(len(ys)-1)*p
    lo=int(math.floor(x)); hi=int(math.ceil(x))
    if lo==hi:
        return ys[lo]
    f=x-lo
    return ys[lo]*(1-f)+ys[hi]*f

def paired_bootstrap(ratios,seed):
    rng=random.Random(seed)
    n=len(ratios)
    samples=[]
    for _ in range(BOOTSTRAPS):
        draw=[ratios[rng.randrange(n)] for __ in range(n)]
        samples.append(gmean(draw))
    return {
        "estimate":gmean(ratios),
        "ci95":[percentile(samples,0.025),percentile(samples,0.975)],
        "block_ratios":ratios,
    }

def index_attempts(d):
    out={}
    for a in d.get("attempts",[]):
        key=(int(a["block"]),str(a["arm"]),str(a["phase"]))
        req(key not in out,"duplicate attempt "+repr(key))
        out[key]=a
    return out

def metric_ratios(d,phase,metric):
    idx=index_attempts(d)
    ratios=[]
    for b in range(BLOCKS):
        a0=idx.get((b,"0",phase)); aa=idx.get((b,"A",phase))
        req(a0 is not None and aa is not None,f"missing paired {phase} block {b}")
        req(bool(a0.get("success")) and bool(aa.get("success")),f"failed paired {phase} block {b}")
        req(a0.get("generated_hash_fnv1a64")==EXPECTED_HASH[d["workload"]],f"baseline semantic hash block {b}")
        req(aa.get("generated_hash_fnv1a64")==EXPECTED_HASH[d["workload"]],f"arm A semantic hash block {b}")
        if phase=="carry":
            req(a0.get("carry_timing_authoritative") is True and aa.get("carry_timing_authoritative") is True,"carry authority flag")
        if phase=="component":
            req(a0.get("component_ticks_authoritative") is True and aa.get("component_ticks_authoritative") is True,"component authority flag")
            req(int(a0.get("q4_down_dispatches",0))==14 and int(aa.get("q4_down_dispatches",0))==14,"component dispatch count")
            req(int(a0.get("timestamp_valid_bits",0))>0 and int(aa.get("timestamp_valid_bits",0))>0,"timestamp validity")
        v0=float(a0[metric]); va=float(aa[metric])
        req(math.isfinite(v0) and math.isfinite(va) and v0>0 and va>0,f"invalid metric {metric} block {b}")
        ratios.append(va/v0)
    return ratios

ap=argparse.ArgumentParser()
ap.add_argument("--w-s",required=True)
ap.add_argument("--w-c",required=True)
ap.add_argument("--out",required=True)
a=ap.parse_args()

data={}
for w,p in [("W-S",a.w_s),("W-C",a.w_c)]:
    d=json.loads(Path(p).read_text(encoding="utf-8"))
    req(d.get("schema")=="arcllm.v1.q4_down_splitk_causal.collection.v0.1",w+" schema")
    req(d.get("status")=="PASS_COLLECTION",w+" status")
    req(d.get("workload")==w,w+" identity")
    req(d.get("expected_generated_hash_fnv1a64")==EXPECTED_HASH[w],w+" expected hash")
    cc=d.get("component_correctness",{})
    req(cc.get("pass") is True and float(cc.get("max_abs",999))<=0.02 and float(cc.get("rmse",999))<=0.005,w+" component correctness")
    iso=d.get("isolation",{})
    req(iso.get("q4_down_layers")==[3,4,6,7,8,11,12,14,15,17,18,19,21,22],w+" target layers")
    req(int(iso.get("q4_down_dispatches_per_decode",0))==14,w+" q4 dispatch count")
    req(iso.get("q6_down_unchanged") is True and iso.get("prefill_unchanged") is True and int(iso.get("extra_resident_bytes",-1))==0,w+" isolation")
    req(len(d.get("attempts",[]))==32,w+" attempt census")
    data[w]=d

result_by_workload={}
for wi,w in enumerate(WORKLOADS):
    component=paired_bootstrap(metric_ratios(data[w],"component","q4_down_ticks"),SEED+wi*100)
    decode=paired_bootstrap(metric_ratios(data[w],"carry","decode_ms"),SEED+wi*100+1)
    e2e=paired_bootstrap(metric_ratios(data[w],"carry","e2e_ms"),SEED+wi*100+2)
    result_by_workload[w]={
        "component_A_over_0":component,
        "decode_A_over_0":decode,
        "e2e_A_over_0":e2e,
        "component_supported":component["ci95"][1]<1.0,
        "decode_carry_supported":decode["ci95"][1]<1.0,
        "e2e_carry_supported":e2e["ci95"][1]<1.0,
    }

component_all=all(result_by_workload[w]["component_supported"] for w in WORKLOADS)
decode_all=all(result_by_workload[w]["decode_carry_supported"] for w in WORKLOADS)
e2e_all=all(result_by_workload[w]["e2e_carry_supported"] for w in WORKLOADS)
if component_all and e2e_all:
    verdict="PASS_CAUSAL_Q4_DOWN_WORK_DECOMPOSITION_WITH_E2E_CARRY"
elif component_all:
    verdict="PASS_CAUSAL_Q4_DOWN_WORK_DECOMPOSITION_NO_E2E_CARRY"
else:
    verdict="NEGATIVE_Q4_DOWN_WORK_DECOMPOSITION"

out={
    "schema":"arcllm.v1.q4_down_splitk_causal.adjudication.v0.1",
    "status":"PASS_ADJUDICATION",
    "verdict":verdict,
    "question":"Does changing only the 14 Q4_K FFN-down decode nodes from generic serial-K to native subgroup32 split-K causally reduce component latency, and does it carry through?",
    "estimator":"geometric mean of 8 paired within-workload A/0 block ratios",
    "bootstrap":{"resamples":BOOTSTRAPS,"base_seed":SEED,"ci":"95% percentile paired bootstrap"},
    "workloads":result_by_workload,
    "cross_workload":{
        "component_supported_both":component_all,
        "decode_carry_supported_both":decode_all,
        "e2e_carry_supported_both":e2e_all,
    },
    "interpretation":{
        "component_causal_claim_allowed":component_all,
        "decode_carry_claim_allowed":decode_all,
        "e2e_carry_claim_allowed":e2e_all,
        "hardware_counter_dependency":False,
        "m3c_directional_signals_used_as_thresholds":False,
        "posthoc_threshold_change_allowed":False,
    },
    "next":"IF_COMPONENT_SUPPORTED_UPDATE_ARCLLM_VALIDATED_PRIMITIVE_ELSE_CLOSE_NEGATIVE"
}
Path(a.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print("Q4_DOWN_SPLITK_CAUSAL_ADJUDICATION=PASS")
print("VERDICT="+verdict)
