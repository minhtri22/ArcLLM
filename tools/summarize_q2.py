#!/usr/bin/env python3
import argparse, hashlib, json, statistics
from pathlib import Path

CELLS=[
    ("arcllm_ws","ArcLLM","W-S"),
    ("baseline_ws","llama.cpp","W-S"),
    ("baseline_wc","llama.cpp","W-C"),
    ("arcllm_wc","ArcLLM","W-C"),
]

def sha256(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()

def stats(vals):
    v=[float(x) for x in vals if x is not None]
    if not v:return None
    med=statistics.median(v)
    return {"median":med,"min":min(v),"max":max(v),"mad":statistics.median([abs(x-med) for x in v])}

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--results-dir",required=True)
    ap.add_argument("--contract",required=True)
    ap.add_argument("--workloads",required=True)
    ap.add_argument("--environment",required=True)
    ap.add_argument("--baseline-qualification",required=True)
    ap.add_argument("--implementation-commit",required=True)
    ap.add_argument("--model-sha256",required=True)
    a=ap.parse_args()
    rd=Path(a.results_dir)

    cell_data={}
    cell_summaries={}
    all_attempts=[]
    mandatory_valid=True
    runtime_cells_complete=True
    for key,system,workload in CELLS:
        result_path=rd/f"q2_{key}_result.json"
        resource_path=rd/f"q2_{key}_resources.json"
        result=json.loads(result_path.read_text(encoding="utf-8"))
        resource=json.loads(resource_path.read_text(encoding="utf-8"))
        attempts=result.get("attempts",[])
        rsum={x["index"]:x for x in resource.get("attempt_summaries",[])}
        merged=[]
        for x in attempts:
            idx=x["index"]; rr=rsum.get(idx,{})
            row=dict(x);row["resource"]=rr
            merged.append(row);all_attempts.append({"cell":key,**row})
            if rr.get("working_set_peak_bytes") is None or rr.get("private_bytes_peak") is None or rr.get("cpu_mean_percent_total_capacity") is None:
                mandatory_valid=False
        if len(attempts)!=5:runtime_cells_complete=False
        success=[x for x in merged if x.get("success")]
        if len(success)<3:runtime_cells_complete=False
        cell_data[key]={"system":system,"workload":workload,"result":result,"resource":resource,"merged_attempts":merged}
        cell_summaries[key]={
            "system":system,"workload":workload,
            "attempts_recorded":len(attempts),
            "successful_attempts":len(success),
            "failed_attempts":len(attempts)-len(success),
            "error_rate":(len(attempts)-len(success))/len(attempts) if attempts else 1.0,
            "ttft_ms":stats([x.get("ttft_ms") for x in success]),
            "decode_tps":stats([x.get("decode_tps") for x in success]),
            "e2e_ms":stats([x.get("e2e_ms") for x in success]),
            "working_set_peak_bytes":stats([x["resource"].get("working_set_peak_bytes") for x in success]),
            "private_bytes_peak":stats([x["resource"].get("private_bytes_peak") for x in success]),
            "cpu_mean_percent_total_capacity":stats([x["resource"].get("cpu_mean_percent_total_capacity") for x in success]),
            "cpu_peak_percent_total_capacity":stats([x["resource"].get("cpu_peak_percent_total_capacity") for x in success]),
            "gpu_compute_mean_percent":stats([x["resource"].get("gpu_compute_mean_percent") for x in success]),
            "gpu_compute_peak_percent":stats([x["resource"].get("gpu_compute_peak_percent") for x in success]),
            "gpu_dedicated_peak_bytes":stats([x["resource"].get("gpu_dedicated_peak_bytes") for x in success]),
            "gpu_shared_peak_bytes":stats([x["resource"].get("gpu_shared_peak_bytes") for x in success]),
            "gpu_counter_available":bool(resource.get("gpu_sampler",{}).get("available")),
            "gpu_counter_error":resource.get("gpu_sampler",{}).get("error"),
        }

    def ratio(arc_key,base_key,metric,invert=False):
        aa=cell_summaries[arc_key].get(metric);bb=cell_summaries[base_key].get(metric)
        if not aa or not bb or not aa.get("median") or not bb.get("median"):return None
        # Always explicit ArcLLM / baseline ratio. No winner semantics.
        return aa["median"]/bb["median"]

    comparison={
        "schema":"arcllm.q2.descriptive_comparison.v1",
        "decision_role":"descriptive_only_no_winner",
        "W-S":{
            "ttft_arc_over_baseline":ratio("arcllm_ws","baseline_ws","ttft_ms"),
            "decode_tps_arc_over_baseline":ratio("arcllm_ws","baseline_ws","decode_tps"),
            "e2e_arc_over_baseline":ratio("arcllm_ws","baseline_ws","e2e_ms"),
            "working_set_arc_over_baseline":ratio("arcllm_ws","baseline_ws","working_set_peak_bytes"),
            "private_bytes_arc_over_baseline":ratio("arcllm_ws","baseline_ws","private_bytes_peak"),
        },
        "W-C":{
            "ttft_arc_over_baseline":ratio("arcllm_wc","baseline_wc","ttft_ms"),
            "decode_tps_arc_over_baseline":ratio("arcllm_wc","baseline_wc","decode_tps"),
            "e2e_arc_over_baseline":ratio("arcllm_wc","baseline_wc","e2e_ms"),
            "working_set_arc_over_baseline":ratio("arcllm_wc","baseline_wc","working_set_peak_bytes"),
            "private_bytes_arc_over_baseline":ratio("arcllm_wc","baseline_wc","private_bytes_peak"),
        }
    }

    qual=json.loads(Path(a.baseline_qualification).read_text(encoding="utf-8"))
    baseline_build_qualified=(
        qual.get("commit")=="391fac16460f15233a7740550d858ac96df3419d" and
        qual.get("release")=="v0.4.1" and qual.get("qualification")=="BUILD_API_QUALIFIED" and
        qual.get("raw_token_adapter") is True and qual.get("target_model_executed") is False
    )
    candidate="Q2_EVIDENCE_READY_FOR_ADJUDICATION"
    if not mandatory_valid:candidate="Q2_MEASUREMENT_INVALID"
    elif not runtime_cells_complete:candidate="Q2_RUNTIME_INCOMPLETE"
    elif not baseline_build_qualified:candidate="Q2_BASELINE_NOT_MATCHED"

    summary={
        "schema":"arcllm.q2.summary.v1",
        "implementation_commit":a.implementation_commit,
        "model_sha256":a.model_sha256,
        "cells":cell_summaries,
        "mandatory_resource_measurements_valid":mandatory_valid,
        "runtime_cells_complete":runtime_cells_complete,
        "baseline_build_api_qualified":baseline_build_qualified,
        "baseline_runtime_match_requires_log_adjudication":True,
        "candidate_classification":candidate,
        "advantage_adjudicated":False,
        "winner_declared":False,
        "q3_started":False
    }
    comparison_path=rd/"q2_descriptive_comparison.json"
    summary_path=rd/"q2_summary.json"
    comparison_path.write_text(json.dumps(comparison,indent=2),encoding="utf-8")
    summary_path.write_text(json.dumps(summary,indent=2),encoding="utf-8")

    artifact_paths=[
        Path(a.contract),Path(a.workloads),Path(a.environment),Path(a.baseline_qualification),
        summary_path,comparison_path
    ]
    for key,_,_ in CELLS:
        artifact_paths += [rd/f"q2_{key}_result.json",rd/f"q2_{key}_resources.json",rd/f"q2_{key}.log"]
    manifest={
        "schema":"arcllm.q2.evidence_manifest.v1",
        "implementation_commit":a.implementation_commit,
        "model_sha256":a.model_sha256,
        "expected_measured_attempts":20,
        "measured_attempts_recorded":sum(x["attempts_recorded"] for x in cell_summaries.values()),
        "artifacts":[{"path":str(p.resolve()),"bytes":p.stat().st_size,"sha256":sha256(p)} for p in artifact_paths if p.exists()],
        "q2_characterization_only":True,
        "advantage_claimed":False,
        "q3_started":False
    }
    (rd/"q2_evidence_manifest.json").write_text(json.dumps(manifest,indent=2),encoding="utf-8")
    print(json.dumps({"candidate_classification":candidate,"measured_attempts":manifest["measured_attempts_recorded"]},indent=2))

if __name__=="__main__":
    main()
