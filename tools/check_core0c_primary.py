#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import statistics
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATASET_NAME = "core0c_measured_20260929T064130264001Z"
DATASET_COMMIT = "73fd1064495578bf2aeb9d90d108df3385c46edc"
TX_HEAD = "35f86ac68f98ffe60fc441a790274cd1f1269dfe"
CORE0B_REFS = {
    "W-S": ROOT/"results"/"core0b_primary_20260929T033358387271Z"/"A_WS_p0_arc.json",
    "W-C": ROOT/"results"/"core0b_primary_20260929T033358387271Z"/"A_WC_p0_arc.json",
}
ORDER = {
    0:["CONTROL_UNINSTRUMENTED_EXTERNAL_WALL","TOKEN_XRAY_TRACE"],
    1:["TOKEN_XRAY_TRACE","CONTROL_UNINSTRUMENTED_EXTERNAL_WALL"],
    2:["CONTROL_UNINSTRUMENTED_EXTERNAL_WALL","TOKEN_XRAY_TRACE"],
}
ARC_STATS = {
    "prefill_dispatches":441,
    "prefill_submits":1,
    "decode_dispatches_per_step":469,
    "decode_submits_per_step":1,
    "decode_steps":31,
    "route_a_steps":0,
    "route_b_steps":31,
    "acquire_events":1,
    "evict_events":1,
    "b_allocations":1,
    "b_materializations":1,
    "b_validations":1,
    "b_releases":1,
    "p1_calls":1,
    "p3_calls":0,
    "p0_calls":0,
    "finite":True,
}

def load(p:Path)->dict:
    return json.loads(p.read_text(encoding="utf-8-sig"))

def sha256(p:Path)->str:
    h=hashlib.sha256()
    with p.open("rb") as f:
        for c in iter(lambda:f.read(8*1024*1024),b""):
            h.update(c)
    return h.hexdigest().upper()

def git(*args:str)->str:
    return subprocess.check_output(["git","-C",str(ROOT),*args],text=True).strip()

def stat(xs:list[float])->dict:
    med=statistics.median(xs)
    return {"n":len(xs),"median":med,"min":min(xs),"max":max(xs),
            "mad":statistics.median(abs(x-med) for x in xs)}

def family(name:str)->str:
    m=re.match(r"^L\d{2}\.(.+)$",name)
    return m.group(1) if m else name

def layer_id(name:str)->str|None:
    m=re.match(r"^(L\d{2})\.",name)
    return m.group(1) if m else None

def validate_arc(d:dict,ref:dict)->list[str]:
    e=[]
    if d.get("generated_token_ids") != ref.get("generated_token_ids"):
        e.append("generated_token_identity")
    s=d.get("stats",{})
    for k,v in ARC_STATS.items():
        if s.get(k)!=v:
            e.append(f"{k}={s.get(k)!r}")
    return e

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--token-xray-root",required=True)
    ap.add_argument("--dataset",default=str(ROOT/"results"/DATASET_NAME))
    ap.add_argument("--out",default=str(ROOT/"results"/"CORE0C_PRIMARY_INDEPENDENT_ADJUDICATION.json"))
    args=ap.parse_args()

    tx=Path(args.token_xray_root).resolve()
    ds=Path(args.dataset).resolve()
    out=Path(args.out).resolve()
    findings=[]
    notes=[]
    def add(fid,detail): findings.append({"id":fid,"detail":detail})

    head=git("rev-parse","HEAD")
    if subprocess.run(["git","-C",str(ROOT),"merge-base","--is-ancestor",DATASET_COMMIT,head]).returncode!=0:
        add("DATASET_COMMIT_ANCESTRY",f"{DATASET_COMMIT} !<= {head}")

    measured_dirs=sorted(p.name for p in (ROOT/"results").glob("core0c_measured_*") if p.is_dir())
    if measured_dirs != [DATASET_NAME]:
        add("FIRST_COMPLETE_COLLECTION_RULE",repr(measured_dirs))

    tx_head=subprocess.check_output(["git","-C",str(tx),"rev-parse","HEAD"],text=True).strip()
    if tx_head!=TX_HEAD: add("TOKEN_XRAY_HEAD",tx_head)
    if subprocess.check_output(["git","-C",str(tx),"status","--porcelain"],text=True).strip():
        add("TOKEN_XRAY_DIRTY",str(tx))

    summary_path=ds/"CORE0C_MEASURED_SUMMARY.json"
    if not summary_path.exists():
        add("SUMMARY_MISSING",str(summary_path)); summary={}
    else:
        summary=load(summary_path)

    expected_top={
        "schema":"arcllm.core0c.measured_summary.v0.1",
        "classification":"CORE0C_COLLECTION_COMPLETE_PENDING_ADJUDICATION",
        "measurement_mode":"TOKEN_XRAY_TRACE_WITH_ADJACENT_CONTROL",
        "timing_authority":"DIAGNOSTIC_ONLY",
        "core0b_performance_authority_preserved":True,
        "measured_requests_expected":12,
        "measured_requests_observed":12,
        "hardware_counters_used":False,
        "profiler_used":False,
        "mechanism_selected":False,
    }
    for k,v in expected_top.items():
        if summary.get(k)!=v: add("SUMMARY_FIELD",f"{k}={summary.get(k)!r} expected {v!r}")

    rows=summary.get("rows",[])
    if len(rows)!=6: add("PAIR_BLOCK_COUNT",str(len(rows)))

    sys.path.insert(0,str(tx/"src"))
    from token_xray.runtime_trace import load_json as tx_load, validate_trace_semantics
    from token_xray.runtime_lifecycle import validate_lifecycle_trace

    refs={w:load(p) for w,p in CORE0B_REFS.items()}
    observer={}
    trace_runs=[]
    physical_by_workload={w:{"prefill":[],"decode":[]} for w in CORE0B_REFS}
    lifecycle_by_workload={w:[] for w in CORE0B_REFS}

    for workload in ["W-S","W-C"]:
        wr=[r for r in rows if r.get("workload")==workload]
        if len(wr)!=3: add("WORKLOAD_BLOCK_COUNT",f"{workload}:{len(wr)}")
        ratios=[]
        for block in range(3):
            rr=next((r for r in wr if r.get("block")==block),None)
            if rr is None:
                add("BLOCK_MISSING",f"{workload}/{block}"); continue
            if rr.get("order")!=ORDER[block]:
                add("ORDER",f"{workload}/{block}:{rr.get('order')}")
            if rr.get("valid") is not True:
                add("ROW_VALID",f"{workload}/{block}")
            arms=rr.get("arms",{})
            for mode in ORDER[block]:
                a=arms.get(mode,{})
                if a.get("returncode")!=0 or a.get("valid") is not True or a.get("errors")!=[]:
                    add("ARM_VALID",f"{workload}/{block}/{mode}:{a}")
                rp=ds/a.get("result_file","")
                if not rp.exists():
                    add("RESULT_MISSING",str(rp)); continue
                errs=validate_arc(load(rp),refs[workload])
                if errs: add("ARC_RESULT_INVARIANT",f"{rp.name}:{errs}")
            control=float(arms["CONTROL_UNINSTRUMENTED_EXTERNAL_WALL"]["wall_ms"])
            trace=float(arms["TOKEN_XRAY_TRACE"]["wall_ms"])
            if control<=0 or trace<=0:
                add("WALL_TIME",f"{workload}/{block}:{control}/{trace}")
            ratio=trace/control
            ratios.append(ratio)

            tdir=ds/arms["TOKEN_XRAY_TRACE"]["trace_dir"]
            prefill=sorted(tdir.glob("*_prefill_*.json"))
            decode=sorted(tdir.glob("*_decode_*.json"))
            life=sorted(tdir.glob("*_runtime_lifecycle.json"))
            if len(prefill)!=1 or len(decode)!=31 or len(life)!=1:
                add("TRACE_CARDINALITY",f"{workload}/{block}: {len(prefill)}/{len(decode)}/{len(life)}")
                continue

            runagg={
                "workload":workload,"block":block,
                "prefill":{"dispatches":0,"sum_dispatch_ns":0,"device_span_ns":0,"unattributed_ns":0,
                           "family_ns":defaultdict(int),"layer_ns":defaultdict(int)},
                "decode":{"dispatches":0,"sum_dispatch_ns":0,"device_span_ns":0,"unattributed_ns":0,
                          "family_ns":defaultdict(int),"layer_ns":defaultdict(int)},
            }
            for p in prefill+decode:
                t=tx_load(p)
                s=validate_trace_semantics(t)
                phase=t["token"]["mode"]
                exp=441 if phase=="prefill" else 469
                if s["dispatch_count"]!=exp or s["timestamped_dispatch_count"]!=exp:
                    add("TRACE_SEMANTICS",f"{p.name}:{s}")
                ctx=t.get("measurement_context",{})
                if ctx.get("measurement_mode")!="TOKEN_XRAY_TRACE" or ctx.get("timing_authority")!="DIAGNOSTIC_ONLY":
                    add("TRACE_AUTHORITY",p.name)
                if ctx.get("instrumentation_overhead") is not None:
                    add("TRACE_OVERHEAD_NOT_NULL",p.name)

                ids={sid for d in t["dispatches"] for sid in d["semantic_node_ids"]}
                if len(ids)!=451: add("SEMANTIC_NODE_COUNT",f"{p.name}:{len(ids)}")

                tm=t["timing"]
                runagg[phase]["device_span_ns"] += int(tm["device_span_ns"])
                runagg[phase]["sum_dispatch_ns"] += int(tm["sum_dispatch_duration_ns"])
                runagg[phase]["unattributed_ns"] += int(tm["unattributed_device_time_ns"])
                for d in t["dispatches"]:
                    dur=d["timestamp"].get("duration_ns")
                    if dur is None:
                        add("NULL_DISPATCH_TIMING",f"{p.name}:{d['dispatch_id']}")
                        continue
                    dur=int(dur)
                    runagg[phase]["dispatches"] += 1
                    fam=family(d["runtime_name"])
                    runagg[phase]["family_ns"][fam] += dur
                    lid=layer_id(d["runtime_name"])
                    if lid: runagg[phase]["layer_ns"][lid] += dur

                    if d["runtime_name"].endswith("ffn_gate_up_fused"):
                        if len(d.get("semantic_node_ids",[]))!=2:
                            add("FUSED_SEMANTIC_MAPPING",f"{p.name}:{d['runtime_name']}")

            lt=tx_load(life[0])
            ls=validate_lifecycle_trace(lt)
            types=[e["event_type"] for e in lt["events"]]
            if ls["event_count"]!=49 or ls["materialize_event_count"]!=14 or ls["related_dispatch_count"]!=14:
                add("LIFECYCLE_COUNTS",f"{life[0].name}:{ls}")
            required={"representation_acquire","representation_materialize","representation_validate",
                      "representation_resident","representation_reuse","representation_evict","representation_release"}
            if not required.issubset(set(types)): add("LIFECYCLE_TYPES",f"{life[0].name}:{sorted(set(types))}")
            mats=[e for e in lt["events"] if e["event_type"]=="representation_materialize"]
            if any(e.get("timestamp",{}).get("duration_ns") is None for e in mats):
                add("LIFECYCLE_MATERIALIZE_TIMING",life[0].name)
            mat_ns=sum(
                int(e["timestamp"]["duration_ns"])
                for e in mats
                if e.get("timestamp",{}).get("duration_ns") is not None
            )
            lifecycle_by_workload[workload].append({"block":block,"materialize_ns":mat_ns,"summary":ls})

            for phase in ["prefill","decode"]:
                if runagg[phase]["dispatches"] != (441 if phase=="prefill" else 469*31):
                    add("AGG_DISPATCH_COUNT",f"{workload}/{block}/{phase}:{runagg[phase]['dispatches']}")
                fam=dict(runagg[phase]["family_ns"]); lay=dict(runagg[phase]["layer_ns"])
                runagg[phase]["family_ns"]=fam; runagg[phase]["layer_ns"]=lay
                physical_by_workload[workload][phase].append(runagg[phase])
            trace_runs.append(runagg)

        observer[workload]={
            "pairs":[{"block":i,"ratio_trace_over_control":ratios[i],
                      "overhead_fraction":ratios[i]-1.0,
                      "overhead_percent":(ratios[i]-1.0)*100.0} for i in range(len(ratios))],
            "ratio_stats":stat(ratios),
            "overhead_fraction_stats":stat([x-1.0 for x in ratios]),
        }

    # Cross-check observer summary emitted by runner.
    reported=summary.get("observer_effect",{})
    for workload in ["W-S","W-C"]:
        if workload not in reported: add("OBSERVER_REPORTED_MISSING",workload); continue
        for key in ["median","min","max","mad"]:
            a=observer[workload]["ratio_stats"][key]
            b=reported[workload]["ratio_stats"][key]
            if not math.isclose(a,b,rel_tol=1e-12,abs_tol=1e-12):
                add("OBSERVER_MATH",f"{workload}/{key}:{a}!={b}")

    localization={}
    for workload in ["W-S","W-C"]:
        localization[workload]={}
        for phase in ["prefill","decode"]:
            runs=physical_by_workload[workload][phase]
            families=sorted(set().union(*(r["family_ns"].keys() for r in runs)))
            fam_rows=[]
            for fam in families:
                shares=[]; durs=[]
                for r in runs:
                    dur=float(r["family_ns"].get(fam,0)); total=float(r["sum_dispatch_ns"])
                    durs.append(dur); shares.append(dur/total if total>0 else 0.0)
                fam_rows.append({
                    "family":fam,
                    "median_share_of_dispatch_time":statistics.median(shares),
                    "share_range":[min(shares),max(shares)],
                    "median_duration_ns":statistics.median(durs),
                })
            fam_rows.sort(key=lambda x:x["median_share_of_dispatch_time"],reverse=True)
            coverage=[]
            for r in runs:
                span=float(r["device_span_ns"]); disp=float(r["sum_dispatch_ns"])
                coverage.append(disp/span if span>0 else 0.0)
            localization[workload][phase]={
                "dispatch_timing_coverage":"100%",
                "dispatch_time_fraction_of_device_span":stat(coverage),
                "top_runtime_families":fam_rows[:10],
                "all_runtime_families":fam_rows,
                "device_span_ns":stat([float(r["device_span_ns"]) for r in runs]),
                "sum_dispatch_ns":stat([float(r["sum_dispatch_ns"]) for r in runs]),
                "unattributed_ns":stat([float(r["unattributed_ns"]) for r in runs]),
            }

        matvals=[float(x["materialize_ns"]) for x in lifecycle_by_workload[workload]]
        localization[workload]["q4v4_lifecycle"]={
            "materialize_events_per_trace":14,
            "materialize_duration_ns":stat(matvals),
            "non_materialize_event_timing":"unknown/null by contract",
        }

    # Observer-effect interpretation is descriptive, not a gate.
    any_negative=any(p["overhead_fraction"]<0 for w in observer.values() for p in w["pairs"])
    max_ratio=max(p["ratio_trace_over_control"] for w in observer.values() for p in w["pairs"])
    min_ratio=min(p["ratio_trace_over_control"] for w in observer.values() for p in w["pairs"])
    observer_interpretation=(
        "HOST_NONSTATIONARITY_PREVENTS_SIMPLE_CAUSAL_OVERHEAD_ESTIMATE"
        if any_negative else "TRACE_OVERHEAD_DIRECTIONALLY_POSITIVE_IN_ALL_PAIRS"
    )
    notes.append({"id":"OBSERVER_EFFECT_INTERPRETATION","value":observer_interpretation,
                  "ratio_range":[min_ratio,max_ratio],
                  "gate":False})

    # Dataset manifest.
    manifest={}
    for p in sorted(ds.rglob("*.json")):
        manifest[str(p.relative_to(ds)).replace("\\","/")]=sha256(p)

    verdict="PASS_CORE0C_DIAGNOSTIC_LOCALIZATION_COMPLETE" if not findings else "STOP_CORE0C_PRIMARY_ADJUDICATION"
    result={
        "schema":"arcllm.core0c.primary_independent_adjudication.v0.1",
        "verdict":verdict,
        "open_findings":len(findings),
        "findings":findings,
        "notes":notes,
        "dataset_name":DATASET_NAME,
        "dataset_commit":DATASET_COMMIT,
        "checked_head":head,
        "measured_requests":12,
        "trace_requests":6,
        "control_requests":6,
        "token_trace_files":192,
        "lifecycle_trace_files":6,
        "observer_effect":observer,
        "observer_effect_interpretation":observer_interpretation,
        "localization":localization,
        "core0b_performance_authority_preserved":True,
        "timing_authority":"DIAGNOSTIC_ONLY",
        "hardware_counters_used":False,
        "profiler_used":False,
        "mechanism_selected":False,
        "dataset_sha256_manifest":manifest,
        "next_if_pass":"FORMAL_CLOSE_CORE0C_AND_OPEN_CORE0D_PREREGISTRATION_ONLY",
    }
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(f"CORE0C_PRIMARY_ADJUDICATION={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    for w in ["W-S","W-C"]:
        print(f"{w}_OBSERVER_MEDIAN={observer[w]['ratio_stats']['median']:.12f}")
        for phase in ["prefill","decode"]:
            top=localization[w][phase]["top_runtime_families"][:5]
            print(f"{w}_{phase.upper()}_TOP5="+",".join(f"{x['family']}:{x['median_share_of_dispatch_time']:.6f}" for x in top))
        print(f"{w}_P1_MATERIALIZE_MEDIAN_NS={localization[w]['q4v4_lifecycle']['materialize_duration_ns']['median']:.0f}")
    for f in findings:
        print(f"FINDING {f['id']}: {f['detail']}")
    return 0 if not findings else 3

if __name__=="__main__":
    raise SystemExit(main())
