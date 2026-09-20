#!/usr/bin/env python3
import argparse, hashlib, json, math, statistics
from pathlib import Path

EXPECTED_ORDERS={
 "A":["arcllm_ws","baseline_ws","baseline_wc","arcllm_wc"],
 "B":["baseline_ws","arcllm_ws","arcllm_wc","baseline_wc"],
}
CELLS={
 "arcllm_ws":("ArcLLM","W-S"),
 "baseline_ws":("llama.cpp","W-S"),
 "arcllm_wc":("ArcLLM","W-C"),
 "baseline_wc":("llama.cpp","W-C"),
}
PROMPT_HASH={"W-S":"93833ffb49890aba","W-C":"5973d0cfd8ad6313"}

def load(p): return json.loads(Path(p).read_text(encoding="utf-8-sig"))
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()
def med(xs): return statistics.median(xs) if xs else None
def finite(x): return isinstance(x,(int,float)) and math.isfinite(x)
def ratio(a,b): return (a/b) if finite(a) and finite(b) and b!=0 else None

def inspect_session(label, root, pf_hash, auth_hash, design_hash):
    root=Path(root); errors=[]
    meta=load(root/"q3_session_meta.json"); env=load(root/"q3_environment.json"); complete=load(root/"q3_session_complete.json")
    if meta.get("schema")!="arcllm.q3.session_meta.v1" or meta.get("session")!=label: errors.append("session_meta_identity")
    if meta.get("execution_order")!=EXPECTED_ORDERS[label]: errors.append("session_order")
    if int(meta.get("expected_measured_attempts",-1))!=20: errors.append("session_attempt_contract")
    if str(meta.get("preflight_lock_sha256","")).upper()!=pf_hash: errors.append("preflight_binding")
    if str(meta.get("design_sha256","")).upper()!=design_hash: errors.append("design_binding")
    if env.get("session")!=label or str(env.get("preflight_lock_sha256","")).upper()!=pf_hash: errors.append("environment_binding")
    if str(env.get("design_sha256","")).upper()!=design_hash: errors.append("environment_design_binding")
    if not bool(env.get("system_power",{}).get("ac_line_status")==1): errors.append("ac_power")
    cells={}
    for key,(system,workload) in CELLS.items():
        rp=root/f"{key}_result.json"; tp=root/f"{key}_resources.json"
        if not rp.is_file() or not tp.is_file():
            errors.append(f"{key}:missing_result_or_trace"); continue
        r=load(rp); t=load(tp)
        if r.get("system")!=system or r.get("workload")!=workload: errors.append(f"{key}:identity")
        if str(r.get("prompt_hash_fnv1a64","")).lower()!=PROMPT_HASH[workload]: errors.append(f"{key}:prompt_hash")
        attempts=r.get("attempts",[])
        if len(attempts)!=5: errors.append(f"{key}:attempt_count")
        if int(r.get("output_tokens",-1))!=32: errors.append(f"{key}:output_tokens")
        summaries=t.get("attempt_summaries",[])
        if len(summaries)!=5: errors.append(f"{key}:resource_attempt_count")
        if int(t.get("process_exit_code",-999))!=int(meta.get("cell_exit_codes",{}).get(key,-998)): errors.append(f"{key}:exit_code_binding")
        byidx={int(x.get("index",-1)):x for x in summaries}
        success=[a for a in attempts if a.get("success") is True]
        for a in attempts:
            i=int(a.get("index",-1))
            if i not in byidx: errors.append(f"{key}:resource_index_{i}"); continue
            s=byidx[i]
            for fld in ("working_set_peak_bytes","private_bytes_peak","cpu_mean_percent_total_capacity","cpu_peak_percent_total_capacity"):
                if not finite(s.get(fld)): errors.append(f"{key}:{fld}:{i}")
            if a.get("success") is True:
                for fld in ("ttft_ms","decode_tps","e2e_ms"):
                    if not finite(a.get(fld)): errors.append(f"{key}:{fld}:{i}")
                if a.get("final_logits_finite") is not True: errors.append(f"{key}:nonfinite_logits:{i}")
                if system=="ArcLLM" and a.get("dispatch_census_pass") is not True: errors.append(f"{key}:dispatch_census:{i}")
        metrics={
          "attempts":len(attempts),"successes":len(success),"stability_5of5":len(success)==5,
          "ttft_ms":med([a["ttft_ms"] for a in success if finite(a.get("ttft_ms"))]),
          "decode_tps":med([a["decode_tps"] for a in success if finite(a.get("decode_tps"))]),
          "e2e_ms":med([a["e2e_ms"] for a in success if finite(a.get("e2e_ms"))]),
          "working_set_peak_bytes":med([byidx[int(a["index"])]["working_set_peak_bytes"] for a in success if int(a["index"]) in byidx]),
          "private_bytes_peak":med([byidx[int(a["index"])]["private_bytes_peak"] for a in success if int(a["index"]) in byidx]),
          "cpu_mean":med([byidx[int(a["index"])]["cpu_mean_percent_total_capacity"] for a in success if int(a["index"]) in byidx]),
          "cpu_peak":med([byidx[int(a["index"])]["cpu_peak_percent_total_capacity"] for a in success if int(a["index"]) in byidx]),
        }
        cells[key]=metrics
    return {"label":label,"root":str(root),"runner_process_id":meta.get("runner_process_id"),"environment":env,"cells":cells,"errors":errors}

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--session-a",required=True);ap.add_argument("--session-b",required=True)
    ap.add_argument("--design",required=True);ap.add_argument("--preflight-lock",required=True);ap.add_argument("--authorization",required=True);ap.add_argument("--out",required=True)
    a=ap.parse_args()
    design=load(a.design);pf=load(a.preflight_lock);auth=load(a.authorization)
    dh=sha(a.design);ph=sha(a.preflight_lock);ah=sha(a.authorization)
    errors=[]
    if design.get("schema")!="arcllm.q3.no_practical_advantage_confirmatory_design.v1": errors.append("design_schema")
    if pf.get("schema")!="arcllm.q3.preflight_lock.v1": errors.append("preflight_schema")
    if auth.get("schema")!="arcllm.q3.execution_authorization.v1" or auth.get("authorized") is not True: errors.append("authorization_schema_or_state")
    if str(auth.get("preflight_lock_sha256","")).upper()!=ph: errors.append("authorization_preflight_binding")
    if str(auth.get("design_sha256","")).upper()!=dh: errors.append("authorization_design_binding")
    A=inspect_session("A",a.session_a,ph,ah,dh);B=inspect_session("B",a.session_b,ph,ah,dh)
    errors += ["A:"+x for x in A["errors"]]+["B:"+x for x in B["errors"]]
    if A["runner_process_id"]==B["runner_process_id"]: errors.append("sessions_not_separate_runner_processes")
    for fld in ("version","build_number"):
        if A["environment"].get("os",{}).get(fld)!=B["environment"].get("os",{}).get(fld): errors.append("environment_os_drift_"+fld)
    if A["environment"].get("power_scheme")!=B["environment"].get("power_scheme"): errors.append("environment_power_scheme_drift")
    ag=[x.get("DriverVersion") for x in A["environment"].get("gpu",[]) if "Arc" in str(x.get("Name",""))]
    bg=[x.get("DriverVersion") for x in B["environment"].get("gpu",[]) if "Arc" in str(x.get("Name",""))]
    if ag!=bg: errors.append("environment_gpu_driver_drift")

    speed=float(design["practical_effect_thresholds"]["speed_or_latency_minimum_relative_improvement"])
    mem=float(design["practical_effect_thresholds"]["working_set_minimum_relative_improvement"])
    harm=float(design["practical_effect_thresholds"]["maximum_allowed_blocking_harm"])
    thresholds={"ttft_max":1-speed,"decode_min":1+speed,"e2e_max":1-speed,"working_set_max":1-mem,
                "harm_ttft_max":1+harm,"harm_decode_min":1-harm,"harm_e2e_max":1+harm,"harm_working_set_max":1+harm}
    session_eval={}
    for S in (A,B):
        se={}
        for wl,suffix in (("W-S","ws"),("W-C","wc")):
            arc=S["cells"].get("arcllm_"+suffix,{});base=S["cells"].get("baseline_"+suffix,{})
            ratios={
              "TTFT":ratio(arc.get("ttft_ms"),base.get("ttft_ms")),
              "DECODE":ratio(arc.get("decode_tps"),base.get("decode_tps")),
              "E2E":ratio(arc.get("e2e_ms"),base.get("e2e_ms")),
              "WORKING_SET":ratio(arc.get("working_set_peak_bytes"),base.get("working_set_peak_bytes")),
            }
            stability=bool(arc.get("stability_5of5") and base.get("stability_5of5"))
            harm_ok=stability and all([
              finite(ratios["TTFT"]) and ratios["TTFT"]<=thresholds["harm_ttft_max"],
              finite(ratios["DECODE"]) and ratios["DECODE"]>=thresholds["harm_decode_min"],
              finite(ratios["E2E"]) and ratios["E2E"]<=thresholds["harm_e2e_max"],
              finite(ratios["WORKING_SET"]) and ratios["WORKING_SET"]<=thresholds["harm_working_set_max"],
            ])
            benefit={
              "TTFT":finite(ratios["TTFT"]) and ratios["TTFT"]<=thresholds["ttft_max"],
              "DECODE":finite(ratios["DECODE"]) and ratios["DECODE"]>=thresholds["decode_min"],
              "E2E":finite(ratios["E2E"]) and ratios["E2E"]<=thresholds["e2e_max"],
              "WORKING_SET":finite(ratios["WORKING_SET"]) and ratios["WORKING_SET"]<=thresholds["working_set_max"],
            }
            dimension_pass={k:bool(v and harm_ok) for k,v in benefit.items()}
            se[wl]={"ratios_arc_over_baseline":ratios,"stability_5of5":stability,"blocking_harm_guard":harm_ok,"primary_benefit":benefit,"dimension_pass":dimension_pass,
                    "supporting_only":{"private_bytes_arc_over_baseline":ratio(arc.get("private_bytes_peak"),base.get("private_bytes_peak")),
                                       "cpu_mean_arc_over_baseline":ratio(arc.get("cpu_mean"),base.get("cpu_mean"))}}
        session_eval[S["label"]]=se

    reproduced=[]
    for wl in ("W-S","W-C"):
        for dim in ("TTFT","DECODE","E2E","WORKING_SET"):
            if session_eval["A"][wl]["dimension_pass"][dim] and session_eval["B"][wl]["dimension_pass"][dim]:
                reproduced.append({"workload":wl,"primary_benefit_dimension":dim})
    if errors:
        candidate="UNRESOLVED"
    elif reproduced:
        candidate="REGIME_ADVANTAGE_SUPPORTED"
    else:
        candidate="FEASIBLE_NO_DEMONSTRATED_ADVANTAGE"
    out={
      "schema":"arcllm.q3.adjudication_candidate.v1","provisional":True,"final_independent_adjudication_required":True,
      "candidate_verdict":candidate,"H_NPA_falsified":bool(reproduced) if not errors else None,
      "design_sha256":dh,"preflight_lock_sha256":ph,"execution_authorization_sha256":ah,
      "expected_fresh_measured_attempts":40,"thresholds":thresholds,
      "sessions":{"A":A,"B":B},"session_evaluation":session_eval,"reproduced_primary_advantages":reproduced,
      "evidence_errors":errors,
      "governance":{"private_bytes_can_establish_advantage":False,"cpu_utilization_can_establish_advantage":False,"gpu_counter_can_establish_advantage":False,
                    "nexus_used":False,"architecture_tuned":False}
    }
    Path(a.out).write_text(json.dumps(out,indent=2)+"\n",encoding="utf-8")
    print(f"Q3 candidate verdict: {candidate}")
    if errors: print("Evidence errors:",len(errors))
if __name__=="__main__": main()
