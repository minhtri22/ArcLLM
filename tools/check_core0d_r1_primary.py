#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import re
import statistics
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DATASET_NAME="core0d_measured_20260929T121751651960Z"
DATASET_COMMIT="630406103254b4358ab32a758b73c0fb4d4742f9"
CORE0B={"W-S":6.91387843464958,"W-C":8.63225451567552}
FORCED={
 "W-S":[136406,144325,181,8100,16019,23938,31857,39776,47695,55614,63533,71452,79371,87290,95209,103128,111047,118966,126885,134804,142723,150642,6498,14417,22336,30255,38174,46093,54012,61931,69850],
 "W-C":[3112,11031,18950,26869,34788,42707,50626,58545,66464,74383,82302,90221,98140,106059,113978,121897,129816,137735,145654,1510,9429,17348,25267,33186,41105,49024,56943,64862,72781,80700,88619],
}
MODES=["CONTROL_UNINSTRUMENTED","PHASE_ONLY","GPU_TRACE"]
TOTAL_RE=re.compile(r"(?m)^Total time:\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s+us\.\s*$")
Q_RE=re.compile(r"q\(128,([0-9]+),28,1\)")
HEADER="Vulkan Timings:"

ARC_STATS={
 "prefill_dispatches":441,"prefill_submits":1,"decode_dispatches_per_step":469,
 "decode_submits_per_step":1,"decode_steps":31,"route_a_steps":0,"route_b_steps":31,
 "acquire_events":1,"evict_events":1,"b_allocations":1,"b_materializations":1,
 "b_validations":1,"b_releases":1,"p1_calls":1,"p3_calls":0,"p0_calls":0,"finite":True,
}

def load(p:Path)->dict:
    return json.loads(p.read_text(encoding="utf-8-sig"))

def git(*args:str)->str:
    return subprocess.check_output(["git","-C",str(ROOT),*args],text=True).strip()

def med(xs): return statistics.median(xs)
def mad(xs):
    m=med(xs); return statistics.median(abs(x-m) for x in xs)
def stats(xs):
    return {"n":len(xs),"median":med(xs),"min":min(xs),"max":max(xs),"mad":mad(xs)}

def finite_pos(x):
    return isinstance(x,(int,float)) and math.isfinite(float(x)) and float(x)>0

def validate_arc_result(d,forced):
    e=[]
    if d.get("schema")!="arcllm.core0d.arc_teacher_forced.v0.1":e.append("schema")
    if d.get("system")!="ArcLLM" or d.get("teacher_forced") is not True or d.get("success") is not True:e.append("identity")
    if d.get("forced_decode_input_ids")!=forced:e.append("forced_ids")
    if len(d.get("predicted_token_ids",[]))!=32:e.append("predicted_count")
    s=d.get("stats",{})
    for k,v in ARC_STATS.items():
        if s.get(k)!=v:e.append(f"{k}={s.get(k)!r}")
    return e

def validate_llama_result(d,w,forced):
    e=[]
    if d.get("schema")!="arcllm.core0d.llama_teacher_forced.v0.1":e.append("schema")
    if d.get("system")!="llama.cpp" or d.get("teacher_forced") is not True or d.get("success") is not True:e.append("identity")
    if d.get("workload")!=w or d.get("forced_decode_input_ids")!=forced:e.append("trajectory")
    if len(d.get("predicted_token_ids",[]))!=32 or d.get("final_logits_finite") is not True:e.append("logits")
    rt=d.get("runtime",{})
    if rt.get("full_offload") is not True or rt.get("offloaded_layers")!=29 or rt.get("offloaded_layers_total")!=29:e.append("offload")
    perf=d.get("perf",{})
    if perf.get("n_eval")!=31 or perf.get("n_p_eval")!=(4 if w=="W-S" else 256):e.append("perf_counts")
    return e

def parse_arc_trace(path:Path,phase:str):
    d=load(path); e=[]
    if d.get("artifact_type")!="TOKEN_TRACE":e.append("artifact_type")
    if d.get("token",{}).get("mode")!=phase:e.append("phase")
    mc=d.get("measurement_context",{})
    if mc.get("measurement_mode")!="TOKEN_XRAY_TRACE" or mc.get("timing_authority")!="DIAGNOSTIC_ONLY":e.append("authority")
    dispatches=d.get("dispatches",[])
    expected=441 if phase=="prefill" else 469
    if len(dispatches)!=expected:e.append(f"dispatch_count={len(dispatches)}")
    ids={sid for x in dispatches for sid in x.get("semantic_node_ids",[])}
    if len(ids)!=451:e.append(f"semantic_nodes={len(ids)}")
    ts=[]
    for x in dispatches:
        v=x.get("timestamp",{}).get("duration_ns")
        if not finite_pos(v):e.append(f"bad_timestamp:{x.get('dispatch_id')}")
        else:ts.append(float(v))
    timing=d.get("timing",{})
    span=timing.get("device_span_ns")
    if not finite_pos(span):e.append("device_span")
    if len(ts)!=expected:e.append("timestamp_coverage")
    return e,float(span) if finite_pos(span) else math.nan

def parse_llama_raw(path:Path,w:str):
    txt=path.read_text(encoding="utf-8",errors="replace")
    blocks=txt.split(HEADER)[1:]
    e=[]
    if len(blocks)!=32:e.append(f"headers={len(blocks)}")
    totals=[]; qvals=[]
    for i,b in enumerate(blocks):
        q=Q_RE.search(b);qvals.append(int(q.group(1)) if q else None)
        ms=TOTAL_RE.findall(b)
        if len(ms)!=1:e.append(f"total_count[{i}]={len(ms)}");continue
        x=float(ms[0])
        if not finite_pos(x):e.append(f"total[{i}]")
        totals.append(x)
    exp=4 if w=="W-S" else 256
    if qvals[:1]!=[exp]:e.append(f"prefill_q={qvals[:1]}")
    if len(qvals)!=32 or any(x!=1 for x in qvals[1:]):e.append("decode_q")
    return e,totals,qvals

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--dataset",default=str(ROOT/"results"/DATASET_NAME))
    ap.add_argument("--out",default=str(ROOT/"results"/"CORE0D_R1_PRIMARY_INDEPENDENT_ADJUDICATION.json"))
    args=ap.parse_args()
    ds=Path(args.dataset).resolve(); out=Path(args.out).resolve()
    findings=[]; gate_findings={f"G{i}":[] for i in range(5)}
    def add(g,i,d):
        rec={"id":i,"detail":d}; findings.append({"gate":g,**rec}); gate_findings[g].append(rec)

    head=git("rev-parse","HEAD")
    if subprocess.run(["git","-C",str(ROOT),"merge-base","--is-ancestor",DATASET_COMMIT,head]).returncode!=0:
        add("G0","DATASET_ANCESTRY",f"{DATASET_COMMIT} !<= {head}")

    summary=load(ds/"CORE0D_MEASURED_SUMMARY.json")
    if summary.get("classification")!="CORE0D_COLLECTION_COMPLETE_PENDING_ADJUDICATION":add("G0","SUMMARY_CLASS",repr(summary.get("classification")))
    if summary.get("measured_requests_expected")!=36 or summary.get("measured_requests_observed")!=36:add("G0","REQUEST_COUNT",repr((summary.get("measured_requests_expected"),summary.get("measured_requests_observed"))))
    rows=summary.get("rows",[])
    if len(rows)!=18:add("G0","ROW_COUNT",str(len(rows)))

    rowmap={(r.get("workload"),int(r.get("block")) if r.get("block") is not None else -1,r.get("mode")):r for r in rows}
    expected={(w,b,m) for w in ["W-S","W-C"] for b in range(3) for m in MODES}
    if set(rowmap)!=expected:add("G0","ROW_KEYS",repr(sorted(set(rowmap)^expected)))

    # G0 exact common-trajectory/result contract on all 36 arms.
    for key in sorted(expected):
        w,b,m=key; r=rowmap.get(key)
        if not r:continue
        if r.get("valid") is not True:add("G0","RUNNER_ROW_INVALID",repr(key))
        arms=r.get("arms",{})
        for system in ["ArcLLM","llama.cpp"]:
            a=arms.get(system)
            if not a:
                add("G0","ARM_MISSING",f"{key}/{system}");continue
            if a.get("returncode")!=0:add("G0","ARM_RC",f"{key}/{system}:{a.get('returncode')}")
            p=ds/a.get("result_file","")
            if not p.exists():add("G0","RESULT_MISSING",str(p));continue
            d=load(p)
            errs=validate_arc_result(d,FORCED[w]) if system=="ArcLLM" else validate_llama_result(d,w,FORCED[w])
            for e in errs:add("G0","RESULT_CONTRACT",f"{p.name}:{e}")

    # G1 CONTROL transfer.
    control={}
    for w in ["W-S","W-C"]:
        vals=[]
        for b in range(3):
            r=rowmap[(w,b,"CONTROL_UNINSTRUMENTED")]
            aw=float(r["arms"]["ArcLLM"]["wall_ms"]); lw=float(r["arms"]["llama.cpp"]["wall_ms"])
            ratio=aw/lw; vals.append({"block":b,"arc_wall_ms":aw,"llama_wall_ms":lw,"ratio":ratio})
            if not (ratio>1):add("G1","CONTROL_RATIO_NOT_GT1",f"{w}/b{b}:{ratio}")
        ratios=[x["ratio"] for x in vals]; m=med(ratios); ref=CORE0B[w]
        factor=max(m/ref,ref/m)
        spread=max(ratios)/min(ratios)
        if factor>1.5:add("G1","CONTROL_TRANSFER_FACTOR",f"{w}:{factor}")
        if spread>2.5:add("G1","CONTROL_SPREAD",f"{w}:{spread}")
        control[w]={"pairs":vals,"ratio_stats":stats(ratios),"core0b_reference":ref,"transfer_factor":factor,"max_over_min":spread}

    # G2 TRACE transfer.
    trace_transfer={}
    for w in ["W-S","W-C"]:
        vals=[]
        for b in range(3):
            r=rowmap[(w,b,"GPU_TRACE")]
            aw=float(r["arms"]["ArcLLM"]["wall_ms"]); lw=float(r["arms"]["llama.cpp"]["wall_ms"])
            ratio=aw/lw; de=aw-lw
            vals.append({"block":b,"arc_wall_ms":aw,"llama_wall_ms":lw,"ratio":ratio,"total_excess_ms":de})
            if not(de>0):add("G2","TRACE_EXCESS_NOT_POSITIVE",f"{w}/b{b}:{de}")
        ratios=[x["ratio"] for x in vals]; tm=med(ratios); cm=control[w]["ratio_stats"]["median"]; factor=max(tm/cm,cm/tm)
        if factor>1.5:add("G2","TRACE_CONTROL_TRANSFER_FACTOR",f"{w}:{factor}")
        trace_transfer[w]={"pairs":vals,"ratio_stats":stats(ratios),"control_median_ratio":cm,"trace_control_factor":factor}

    # G3 raw GPU measurement qualification + G4 decomposition.
    decomposed={"W-S":[],"W-C":[]}
    g3_details={"W-S":[],"W-C":[]}
    for w in ["W-S","W-C"]:
        for b in range(3):
            r=rowmap[(w,b,"GPU_TRACE")]
            aa=r["arms"]["ArcLLM"]; la=r["arms"]["llama.cpp"]
            tdir=ds/aa.get("trace_dir","")
            pre=sorted(tdir.glob("*_prefill_*.json")); dec=sorted(tdir.glob("*_decode_*.json"))
            if len(pre)!=1:add("G3","ARC_PREFILL_TRACE_COUNT",f"{w}/b{b}:{len(pre)}")
            if len(dec)!=31:add("G3","ARC_DECODE_TRACE_COUNT",f"{w}/b{b}:{len(dec)}")
            arc_spans=[]
            for p in pre:
                es,span=parse_arc_trace(p,"prefill")
                for e in es:add("G3","ARC_TRACE",f"{p.name}:{e}")
                if math.isfinite(span):arc_spans.append(span)
            for p in dec:
                es,span=parse_arc_trace(p,"decode")
                for e in es:add("G3","ARC_TRACE",f"{p.name}:{e}")
                if math.isfinite(span):arc_spans.append(span)
            G_arc_ms=sum(arc_spans)/1e6

            raw=ds/la.get("vkperf_raw","")
            parsed=ds/la.get("vkperf_file","")
            if not raw.exists():add("G3","LLAMA_RAW_MISSING",str(raw)); raw_tot=[]
            else:
                es,raw_tot,qvals=parse_llama_raw(raw,w)
                for e in es:add("G3","LLAMA_RAW",f"{raw.name}:{e}")
            if not parsed.exists():add("G3","LLAMA_PARSED_MISSING",str(parsed)); pd={}
            else:pd=load(parsed)
            if pd:
                if pd.get("schema")!="arcllm.core0d_r1.llama_vk_perf_parse.v0.1":add("G3","LLAMA_SCHEMA",parsed.name)
                if pd.get("timing_authority")!="DIAGNOSTIC_ONLY" or pd.get("block_count")!=32 or pd.get("decode_step_count")!=31:add("G3","LLAMA_CONTRACT",parsed.name)
                if pd.get("prefill_q_sequence_length")!=(4 if w=="W-S" else 256):add("G3","LLAMA_PREFILL_SHAPE",parsed.name)
                vals=[pd.get("prefill_gpu_us")]+list(pd.get("decode_gpu_us",[]))
                if len(vals)!=32 or any(not finite_pos(x) for x in vals):add("G3","LLAMA_TIMINGS",parsed.name)
                if len(raw_tot)==32 and len(vals)==32:
                    for i,(x,y) in enumerate(zip(raw_tot,vals)):
                        if not math.isclose(float(x),float(y),rel_tol=1e-12,abs_tol=1e-9):add("G3","LLAMA_PARSE_MISMATCH",f"{parsed.name}[{i}]")
                token_total=float(pd.get("token_gpu_total_us",math.nan))
                if not finite_pos(token_total):add("G3","LLAMA_TOKEN_TOTAL",parsed.name)
                elif len(vals)==32 and not math.isclose(token_total,sum(float(x) for x in vals),rel_tol=1e-12,abs_tol=1e-6):add("G3","LLAMA_SUM_MISMATCH",parsed.name)
            G_llama_ms=float(pd.get("token_gpu_total_us",math.nan))/1000.0 if pd else math.nan

            W_arc=float(aa["wall_ms"]);W_llama=float(la["wall_ms"])
            H_arc=W_arc-G_arc_ms;H_llama=W_llama-G_llama_ms
            if not finite_pos(G_arc_ms):add("G4","ARC_G_NONPOSITIVE",f"{w}/b{b}:{G_arc_ms}")
            if not finite_pos(G_llama_ms):add("G4","LLAMA_G_NONPOSITIVE",f"{w}/b{b}:{G_llama_ms}")
            if H_arc<0:add("G4","ARC_H_NEGATIVE",f"{w}/b{b}:{H_arc}")
            if H_llama<0:add("G4","LLAMA_H_NEGATIVE",f"{w}/b{b}:{H_llama}")
            de=W_arc-W_llama;dg=G_arc_ms-G_llama_ms;dh=H_arc-H_llama
            closure=abs(de-(dg+dh))
            if not(de>0):add("G4","DELTA_E_NOT_POSITIVE",f"{w}/b{b}:{de}")
            if closure>0.001:add("G4","CLOSURE",f"{w}/b{b}:{closure}ms")
            fg=dg/de;fh=dh/de
            decomposed[w].append({
                "block":b,"W_arc_ms":W_arc,"W_llama_ms":W_llama,
                "G_arc_ms":G_arc_ms,"G_llama_ms":G_llama_ms,
                "H_arc_ms":H_arc,"H_llama_ms":H_llama,
                "DeltaE_ms":de,"DeltaG_ms":dg,"DeltaH_ms":dh,
                "closure_error_ms":closure,"f_G":fg,"f_H":fh
            })
            g3_details[w].append({"block":b,"arc_trace_files":len(pre)+len(dec),"arc_token_gpu_ms":G_arc_ms,"llama_groups":len(raw_tot),"llama_token_gpu_ms":G_llama_ms})

    gate_pass={g:len(gate_findings[g])==0 for g in gate_findings}
    all_pass=all(gate_pass.values())

    attribution={}
    amdahl={"opened":all_pass}
    if all_pass:
        for w in ["W-S","W-C"]:
            attribution[w]={
                "DeltaE_ms":stats([x["DeltaE_ms"] for x in decomposed[w]]),
                "DeltaG_ms":stats([x["DeltaG_ms"] for x in decomposed[w]]),
                "DeltaH_ms":stats([x["DeltaH_ms"] for x in decomposed[w]]),
                "f_G":stats([x["f_G"] for x in decomposed[w]]),
                "f_H":stats([x["f_H"] for x in decomposed[w]]),
            }
        regions={}
        for region,key in [("G","f_G"),("H","f_H")]:
            q={}
            for w in ["W-S","W-C"]:
                mf=attribution[w][key]["median"];R=CORE0B[w]
                q[w]=mf*(R-1.0)/R
            eligible=(attribution["W-S"][key]["median"]>0 and attribution["W-C"][key]["median"]>0 and q["W-S"]>=0.10 and q["W-C"]>=0.10 and max(q.values())>=0.20)
            score=min(q.values())
            speed={}
            for w,v in q.items():
                speed[w]=(1.0/(1.0-v)) if v<1.0 else None
            regions[region]={"q":q,"ideal_full_elimination_speedup":speed,"eligible":eligible,"robust_score":score}
        g=regions["G"];h=regions["H"]
        winner=None
        candidates=[(x,regions[x]) for x in ["G","H"] if regions[x]["eligible"] and regions[x]["robust_score"]>=0.15]
        if candidates:
            candidates.sort(key=lambda z:z[1]["robust_score"],reverse=True)
            top=candidates[0]
            other=regions["H" if top[0]=="G" else "G"]
            if top[1]["robust_score"]-other["robust_score"]>=0.05:winner=top[0]
        decision="G_TOKEN_GPU" if winner=="G" else ("H_OUTSIDE_TOKEN_GPU" if winner=="H" else "NO_SINGLE_REGION_PRIORITY")
        successor="CORE-0E_GPU_EXCESS_SUBFAMILY_ATTRIBUTION_PREREGISTRATION" if winner=="G" else ("CORE-0E_OUTSIDE_GPU_PHASE_ATTRIBUTION_PREREGISTRATION" if winner=="H" else "CORE-0E_SPLIT_REGION_BOUNDARY_REPLICATION_PREREGISTRATION")
        amdahl.update({"regions":regions,"decision":decision,"successor":successor})

    # Descriptive PHASE_ONLY evidence, non-gating.
    phase={}
    for w in ["W-S","W-C"]:
        arc_fields={};llama_fields={}
        for b in range(3):
            r=rowmap[(w,b,"PHASE_ONLY")]
            ap=ds/r["arms"]["ArcLLM"].get("phase_file","")
            if ap.exists():
                d=load(ap)
                for k,v in d.items():
                    if k.endswith("_ns") and isinstance(v,(int,float)):arc_fields.setdefault(k,[]).append(float(v)/1e6)
            lp=ds/r["arms"]["llama.cpp"]["result_file"]
            if lp.exists():
                d=load(lp).get("perf",{})
                for k in ["t_load_ms","t_p_eval_ms","t_eval_ms"]:
                    if isinstance(d.get(k),(int,float)):llama_fields.setdefault(k,[]).append(float(d[k]))
        phase[w]={
            "arcllm_ms":{k:stats(v) for k,v in arc_fields.items()},
            "llama_ms":{k:stats(v) for k,v in llama_fields.items()},
            "authority":"DIAGNOSTIC_ONLY"
        }

    if all_pass:
        verdict="PASS_CORE0D_R1_EXCESS_COST_ATTRIBUTION_COMPLETE"
    elif not gate_pass["G0"]:verdict="STOP_CORE0D_R1_COMMON_TRAJECTORY_INVALID"
    elif not gate_pass["G1"]:verdict="STOP_CORE0D_R1_TOTAL_TRANSFER_NOT_QUALIFIED"
    elif not gate_pass["G2"]:verdict="STOP_CORE0D_R1_TRACE_TRANSFER_NOT_QUALIFIED"
    elif not gate_pass["G3"]:verdict="STOP_CORE0D_R1_GPU_MEASUREMENT_NOT_QUALIFIED"
    else:verdict="STOP_CORE0D_R1_RESIDUAL_CLOSURE_NOT_QUALIFIED"

    result={
        "schema":"arcllm.core0d_r1.primary_independent_adjudication.v0.1",
        "verdict":verdict,"open_findings":len(findings),"findings":findings,
        "dataset":DATASET_NAME,"dataset_commit":DATASET_COMMIT,"checked_head":head,
        "measured_requests":36,"matched_rows":18,
        "gate_pass":gate_pass,"gate_findings":gate_findings,
        "G1_control_transfer":control,"G2_trace_transfer":trace_transfer,
        "G3_gpu_measurement":g3_details,
        "G4_decomposition_pairs":decomposed,
        "primary_attribution":attribution,
        "amdahl_gate":amdahl,
        "secondary_phase_only":phase,
        "boundaries":{
            "core0b_performance_authority_preserved":True,
            "core0d_parent_dataset_reused_for_primary":False,
            "optimization_authorized":False,
            "mechanism_selection_authorized":False,
            "npu_authorized":False
        }
    }
    out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(f"CORE0D_R1_PRIMARY_ADJUDICATION={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    print("GATES="+",".join(f"{g}:{'PASS' if gate_pass[g] else 'FAIL'}" for g in ["G0","G1","G2","G3","G4"]))
    if all_pass:
        for w in ["W-S","W-C"]:
            print(f"{w}_fG_MEDIAN={attribution[w]['f_G']['median']:.12f}")
            print(f"{w}_fH_MEDIAN={attribution[w]['f_H']['median']:.12f}")
        print("AMDAHL_DECISION="+amdahl["decision"])
        print("SUCCESSOR="+amdahl["successor"])
        for reg in ["G","H"]:
            print(f"{reg}_Q_WS={amdahl['regions'][reg]['q']['W-S']:.12f}")
            print(f"{reg}_Q_WC={amdahl['regions'][reg]['q']['W-C']:.12f}")
            print(f"{reg}_ROBUST={amdahl['regions'][reg]['robust_score']:.12f}")
            print(f"{reg}_ELIGIBLE={amdahl['regions'][reg]['eligible']}")
    for f in findings:print(f"FINDING {f['gate']} {f['id']}: {f['detail']}")
    return 0 if not findings else 3

if __name__=="__main__":
    raise SystemExit(main())
