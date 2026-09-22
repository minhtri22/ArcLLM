import argparse, json, math, statistics
from pathlib import Path

SCHEMA="arcllm.v1.i001r.exact_7b_decode_device_profile.v0.1"
CELLS={
    "A/W-S":"i001r_A_WS.json",
    "A/W-C":"i001r_A_WC.json",
    "B/W-C":"i001r_B_WC.json",
    "B/W-S":"i001r_B_WS.json",
}
FAMILIES=["embedding","rmsnorm","attn_qkv","rope","kv_store","attention","attn_output",
          "residual_add","ffn_gate_up","swiglu","ffn_down","lm_head","other"]
QUANT_LINEAR={"attn_qkv","attn_output","ffn_gate_up","ffn_down","lm_head"}

def median(xs):
    return statistics.median(xs) if xs else None

def q(xs,p):
    if not xs: return None
    a=sorted(xs)
    if len(a)==1:return a[0]
    x=(len(a)-1)*p
    lo=int(math.floor(x)); hi=int(math.ceil(x))
    if lo==hi:return a[lo]
    return a[lo]*(hi-x)+a[hi]*(x-lo)

def family_of(name):
    if name=="token_embedding": return "embedding"
    if name=="lm_head": return "lm_head"
    if name=="output_norm" or "rmsnorm" in name: return "rmsnorm"
    if "q_proj" in name or "k_proj" in name or "v_proj" in name: return "attn_qkv"
    if "rope" in name: return "rope"
    if "kv_store" in name: return "kv_store"
    if "cached_gqa" in name: return "attention"
    if "o_proj" in name: return "attn_output"
    if "attn_residual" in name or "ffn_residual" in name: return "residual_add"
    if "ffn_gate" in name or "ffn_up" in name: return "ffn_gate_up"
    if "swiglu" in name: return "swiglu"
    if "ffn_down" in name: return "ffn_down"
    return "other"

ap=argparse.ArgumentParser()
ap.add_argument("--results-dir",required=True)
ap.add_argument("--out",required=True)
args=ap.parse_args()
root=Path(args.results_dir)
errors=[]
data={}
for key,fn in CELLS.items():
    p=root/fn
    if not p.exists():
        errors.append(f"missing cell {key}: {fn}")
        continue
    try:
        d=json.loads(p.read_text(encoding="utf-8-sig"))
    except Exception as e:
        errors.append(f"parse {key}: {e}")
        continue
    data[key]=d
    if d.get("schema")!=SCHEMA: errors.append(f"{key}: schema mismatch")
    if d.get("production",{}).get("decode_dispatches_per_step")!=469: errors.append(f"{key}: decode census")
    if d.get("production",{}).get("profile_probe_decode_indices")!=[0,15,30]: errors.append(f"{key}: probe indices")
    if len(d.get("decode_op_names",[]))!=469: errors.append(f"{key}: op-name census")
    ats=d.get("attempts",[])
    if len(ats)!=5: errors.append(f"{key}: measured attempts !=5")
    for ai,a in enumerate(ats):
        if not a.get("success"): errors.append(f"{key}/{ai}: attempt fail")
        if not a.get("final_logits_finite"): errors.append(f"{key}/{ai}: nonfinite")
        if not a.get("dispatch_census_pass"): errors.append(f"{key}/{ai}: dispatch census")
        ps=a.get("profile_summary",{})
        if ps.get("profiled_steps")!=3: errors.append(f"{key}/{ai}: profiled steps")
        if ps.get("lifecycle_steps")!=28: errors.append(f"{key}/{ai}: lifecycle steps")
        probes=a.get("probes",[])
        if len(probes)!=3: errors.append(f"{key}/{ai}: probes")
        for pi,pr in enumerate(probes):
            if pr.get("decode_index") not in [0,15,30]: errors.append(f"{key}/{ai}/{pi}: bad probe")
            if len(pr.get("op_ticks",[]))!=469: errors.append(f"{key}/{ai}/{pi}: op ticks")
            if not (pr.get("timestamp_valid_bits",0)>0): errors.append(f"{key}/{ai}/{pi}: timestamp bits")
            if pr.get("chain_ticks",0)<=0 or pr.get("dispatch_tick_sum",0)<=0: errors.append(f"{key}/{ai}/{pi}: zero ticks")

# Cross-cell graph identity.
op_lists=[tuple(d.get("decode_op_names",[])) for d in data.values()]
if op_lists and any(x!=op_lists[0] for x in op_lists[1:]): errors.append("decode op-name graph differs across cells")
weight_maps=[d.get("model_weight_bytes_per_decode_step",{}) for d in data.values()]
if weight_maps and any(x!=weight_maps[0] for x in weight_maps[1:]): errors.append("weight-byte map differs across cells")

# Semantic equality: every measured run for the same workload must generate the same 32 tokens.
for workload in ["W-S","W-C"]:
    seqs=[]
    for key,d in data.items():
        if key.endswith(workload):
            for a in d.get("attempts",[]): seqs.append(tuple(a.get("generated_token_ids",[])))
    if seqs:
        if any(len(x)!=32 for x in seqs): errors.append(f"{workload}: generated length")
        if any(x!=seqs[0] for x in seqs[1:]): errors.append(f"{workload}: generated sequence mismatch")

out={
 "schema":"arcllm.v1.i001r.profile_summary.v0.1",
 "profile_validity":"FAIL" if errors else "PASS",
 "errors":errors,
 "measured_attempts_present":sum(len(d.get("attempts",[])) for d in data.values()),
 "profiled_decode_steps_present":sum(len(a.get("probes",[])) for d in data.values() for a in d.get("attempts",[])),
 "normal_lifecycle_steps_present":sum(a.get("profile_summary",{}).get("lifecycle_steps",0) for d in data.values() for a in d.get("attempts",[])),
}

if not errors:
    family_shares={f:[] for f in FAMILIES}
    barrier_shares=[]
    lifecycle_outside=[]
    dispatch_ticks={f:[] for f in FAMILIES}
    cell_summary={}
    total_family_ticks={f:0.0 for f in FAMILIES}
    total_probes=0

    for key,d in data.items():
        names=d["decode_op_names"]
        cell_family={f:[] for f in FAMILIES}
        cell_barrier=[]; cell_life=[]
        for a in d["attempts"]:
            ps=a["profile_summary"]
            chain=ps["profiled_chain_ticks"]
            ft=ps["family_ticks"]
            for f in FAMILIES:
                share=(ft.get(f,0)/chain) if chain else 0.0
                family_shares[f].append(share);cell_family[f].append(share)
            barrier=(ps["profiled_barrier_unattributed_ticks"]/chain) if chain else 0.0
            barrier_shares.append(barrier);cell_barrier.append(barrier)
            rec=ps["lifecycle_record_submit_wait_ms"]; sub=ps["lifecycle_submit_wait_ms"]
            outside=((rec-sub)/rec) if rec>0 else 0.0
            lifecycle_outside.append(outside);cell_life.append(outside)
            for pr in a["probes"]:
                total_probes+=1
                for name,ticks in zip(names,pr["op_ticks"]):
                    f=family_of(name)
                    tv=float(ticks)
                    dispatch_ticks[f].append(tv)
                    total_family_ticks[f]+=tv
        cell_summary[key]={
            "family_share_median":{f:median(cell_family[f]) for f in FAMILIES},
            "barrier_unattributed_share_median":median(cell_barrier),
            "lifecycle_outside_submit_share_median":median(cell_life),
            "decode_tps_median":median([a["decode_tps"] for a in d["attempts"]]),
            "ttft_ms_median":median([a["ttft_ms"] for a in d["attempts"]]),
        }

    family_medians={f:median(v) for f,v in family_shares.items()}
    ranked=sorted(family_medians.items(),key=lambda kv:kv[1],reverse=True)
    quant_linear_share=sum(family_medians.get(f,0.0) for f in QUANT_LINEAR)
    top_family,top_share=ranked[0]
    top2_share=ranked[0][1]+ranked[1][1]
    life_med=median(lifecycle_outside)
    barrier_med=median(barrier_shares)

    # Pre-registered decision rules.
    if life_med>=0.10:
        decision="REOPEN_EXECUTION_LIFECYCLE_MECHANISM"
    elif barrier_med>=0.10:
        decision="REOPEN_SYNC_MECHANISM"
    elif top_share>=0.20:
        decision="DOMINANT_DEVICE_FAMILY_IDENTIFIED:"+top_family
    elif top2_share>=0.40:
        decision="MIXED_DEVICE_FAMILIES_DOMINANT"
    else:
        decision="DIFFUSE_DEVICE_WORK_NEEDS_DEEPER_BOUND_MODEL"

    weight_map=weight_maps[0] if weight_maps else {}
    logical_bytes_per_tick={}
    for f,b in weight_map.items():
        ns=total_family_ticks.get(f,0.0)
        if ns>0 and total_probes>0:
            logical_bytes=float(b)*total_probes
            logical_bytes_per_tick[f]=(logical_bytes/ns) # bytes/ns == GB/s decimal

    out.update({
      "cell_summary":cell_summary,
      "global":{
        "family_share_median":family_medians,
        "family_rank":ranked,
        "quant_linear_family_share_sum_of_medians":quant_linear_share,
        "barrier_unattributed_share_median":barrier_med,
        "lifecycle_outside_submit_share_median":life_med,
        "top_family":top_family,
        "top_family_share":top_share,
        "top2_family_share":top2_share,
        "dispatch_duration_ticks":{
            f:{"p50":q(v,0.50),"p90":q(v,0.90),"p99":q(v,0.99),"max":max(v) if v else None,"n":len(v)}
            for f,v in dispatch_ticks.items()
        },
        "logical_model_weight_bytes_per_tick":logical_bytes_per_tick,
        "logical_weight_tick_semantics":"exact model tensor payload bytes per profiled step divided by family timestamp ticks; this is not GB/s, not measured DRAM bandwidth, and may include cache reuse",
      },
      "decision":decision,
      "replacement_implementation_selected":False,
      "next_rule":"Use the dominant exact-7B device family to open a family-specific compute-vs-memory/locality lower-bound study before implementation."
    })

Path(args.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
print(out["profile_validity"])
print(out.get("decision","INVALID"))
raise SystemExit(0 if not errors else 2)
