#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import os
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
MODEL_SHA="60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
MODEL_BYTES=4683074048
TX_HEAD="35f86ac68f98ffe60fc441a790274cd1f1269dfe"
LLAMA_HEAD="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"

FORCED={
 "W-S":[136406,144325,181,8100,16019,23938,31857,39776,47695,55614,63533,71452,79371,87290,95209,103128,111047,118966,126885,134804,142723,150642,6498,14417,22336,30255,38174,46093,54012,61931,69850],
 "W-C":[3112,11031,18950,26869,34788,42707,50626,58545,66464,74383,82302,90221,98140,106059,113978,121897,129816,137735,145654,1510,9429,17348,25267,33186,41105,49024,56943,64862,72781,80700,88619],
}
MODE_ORDER={
 0:["CONTROL_UNINSTRUMENTED","PHASE_ONLY","GPU_TRACE"],
 1:["GPU_TRACE","CONTROL_UNINSTRUMENTED","PHASE_ONLY"],
 2:["PHASE_ONLY","GPU_TRACE","CONTROL_UNINSTRUMENTED"],
}
SYSTEM_ORDER={0:["ArcLLM","llama.cpp"],1:["llama.cpp","ArcLLM"],2:["ArcLLM","llama.cpp"]}


def prompt(workload:str)->list[int]:
    if workload=="W-S": return [1,133151,133152,152062]
    v=[0]*256
    v[:4]=[1,133151,133152,152062]
    for i in range(4,256):v[i]=1+((104729+7919*i)%152063)
    return v


def sha256(p:Path)->str:
    h=hashlib.sha256()
    with p.open("rb") as f:
        for c in iter(lambda:f.read(8*1024*1024),b""): h.update(c)
    return h.hexdigest().upper()


def load(p:Path)->dict:
    return json.loads(p.read_text(encoding="utf-8-sig"))


def child(cmd:list[str],env:dict[str,str]|None=None,capture_stderr:Path|None=None)->tuple[int,float]:
    t0=time.perf_counter_ns()
    if capture_stderr is None:
        cp=subprocess.run(cmd,cwd=ROOT,env=env)
    else:
        with capture_stderr.open("w",encoding="utf-8") as ef:
            cp=subprocess.run(cmd,cwd=ROOT,env=env,stderr=ef)
    return cp.returncode,(time.perf_counter_ns()-t0)/1e6


def validate_arc(d:dict,forced:list[int])->list[str]:
    e=[]
    if d.get("schema")!="arcllm.core0d.arc_teacher_forced.v0.1":e.append("schema")
    if d.get("success") is not True or d.get("teacher_forced") is not True:e.append("success")
    if d.get("forced_decode_input_ids")!=forced:e.append("forced_ids")
    if len(d.get("predicted_token_ids",[]))!=32:e.append("predicted_count")
    s=d.get("stats",{})
    exp={"prefill_dispatches":441,"prefill_submits":1,"decode_dispatches_per_step":469,"decode_submits_per_step":1,"decode_steps":31,"route_a_steps":0,"route_b_steps":31,"acquire_events":1,"evict_events":1,"b_allocations":1,"b_materializations":1,"b_validations":1,"b_releases":1,"p1_calls":1,"p3_calls":0,"p0_calls":0,"finite":True}
    for k,v in exp.items():
        if s.get(k)!=v:e.append(f"{k}={s.get(k)!r}")
    return e


def validate_llama(d:dict,workload:str,forced:list[int])->list[str]:
    e=[]
    if d.get("schema")!="arcllm.core0d.llama_teacher_forced.v0.1":e.append("schema")
    if d.get("success") is not True or d.get("teacher_forced") is not True:e.append("success")
    if d.get("workload")!=workload:e.append("workload")
    if d.get("forced_decode_input_ids")!=forced:e.append("forced_ids")
    if len(d.get("predicted_token_ids",[]))!=32:e.append("predicted_count")
    if d.get("final_logits_finite") is not True:e.append("finite")
    r=d.get("runtime",{})
    if r.get("full_offload") is not True or r.get("offloaded_layers")!=29 or r.get("offloaded_layers_total")!=29:e.append("offload")
    p=d.get("perf",{})
    if p.get("n_eval")!=31 or p.get("n_p_eval")!=(4 if workload=="W-S" else 256):e.append("perf_counts")
    return e


def load_parser():
    path=ROOT/"tools"/"parse_core0d_llama_vk_perf.py"
    spec=importlib.util.spec_from_file_location("core0d_vk_parser",path)
    mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
    return mod


def fixture_self_test()->int:
    assert sum(len(v) for v in FORCED.values())==62
    assert [len(prompt("W-S")),len(prompt("W-C"))]==[4,256]
    count=0
    for w in ["W-S","W-C"]:
        for b in range(3):
            for m in MODE_ORDER[b]:
                for s in SYSTEM_ORDER[b]:
                    count+=1
    if count!=36:raise SystemExit("CORE0D fixture schedule count")
    print("CORE0D_MEASURED_RUNNER_FIXTURE_SELF_TEST=PASS")
    return 0


def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--model")
    ap.add_argument("--token-xray-root")
    ap.add_argument("--llama-root")
    ap.add_argument("--authorization",default=str(ROOT/"config"/"core0d_science_authorization.json"))
    ap.add_argument("--fixture-self-test",action="store_true")
    args=ap.parse_args()
    if args.fixture_self_test:return fixture_self_test()

    authp=Path(args.authorization)
    if not authp.exists():raise SystemExit("STOP: CORE0D science authorization artifact is absent")
    a=load(authp)
    if a.get("authorized") is not True or a.get("measured_requests_authorized")!=36:
        raise SystemExit("STOP: CORE0D 36-request execution is not authorized")
    if a.get("core0b_performance_authority_preserved") is not True:
        raise SystemExit("STOP: CORE0B authority preservation missing")

    model=Path(args.model).resolve()
    tx=Path(args.token_xray_root).resolve()
    llama_root=Path(args.llama_root).resolve()
    if not model.exists() or model.stat().st_size!=MODEL_BYTES or sha256(model)!=MODEL_SHA:
        raise SystemExit("STOP: CORE0D exact model mismatch")
    if subprocess.check_output(["git","-C",str(tx),"rev-parse","HEAD"],text=True).strip()!=TX_HEAD:
        raise SystemExit("STOP: Token-XRay pin drift")
    if subprocess.check_output(["git","-C",str(llama_root),"rev-parse","HEAD"],text=True).strip()!=LLAMA_HEAD:
        raise SystemExit("STOP: llama pin drift")

    arc_control=ROOT/"artifacts"/"core0d"/"core0d_arcllm_control_phase.exe"
    arc_trace=ROOT/"artifacts"/"core0d"/"core0d_arcllm_trace.exe"
    llama_exe=ROOT/"artifacts"/"core0d"/"core0d_llama_teacher_forced_adapter.exe"
    for k,p in [("arc_control",arc_control),("arc_trace",arc_trace),("llama",llama_exe)]:
        if sha256(p)!=a[f"{k}_exe_sha256"]:raise SystemExit(f"STOP: {k} exe drift")

    head=subprocess.check_output(["git","-C",str(ROOT),"rev-parse","HEAD"],text=True).strip()
    for path,blob in a.get("critical_git_blobs",{}).items():
        got=subprocess.check_output(["git","-C",str(ROOT),"rev-parse",f"HEAD:{path}"],text=True).strip()
        if got!=blob:raise SystemExit(f"STOP: critical blob drift {path}")
    lock_blob=subprocess.check_output(
        ["git","-C",str(ROOT),"rev-parse","HEAD:config/core0d_execution_lock_v0.1.json"],
        text=True,
    ).strip()
    if lock_blob!=a.get("execution_lock_blob"):
        raise SystemExit("STOP: CORE0D execution lock blob drift")
    shader_dir=ROOT/"compiled_shaders"
    for name,expected in a.get("active_shader_sha256",{}).items():
        p=shader_dir/name
        if not p.exists() or sha256(p)!=expected:
            raise SystemExit(f"STOP: CORE0D shader drift: {name}")
    inst=a.get("instrumentation",{})
    if any(inst.get(k) is not False for k in ("hardware_counters","external_profiler","resource_sampler")):
        raise SystemExit("STOP: CORE0D forbidden instrumentation authorization drift")
    if a.get("npu_authorized") is not False:
        raise SystemExit("STOP: CORE0D NPU must remain unauthorized")
    if a.get("optimization_authorized") is not False or a.get("mechanism_selection_authorized") is not False:
        raise SystemExit("STOP: CORE0D optimization/mechanism selection must remain unauthorized")

    existing=list((ROOT/"results").glob("core0d_measured_*"))
    if existing:raise SystemExit("STOP: CORE0D measured dataset already exists")
    stamp=datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    outdir=ROOT/"results"/f"core0d_measured_{stamp}"
    outdir.mkdir(parents=True,exist_ok=False)
    parser=load_parser()
    rows=[]

    for workload in ["W-S","W-C"]:
        toks=prompt(workload);forced=FORCED[workload]
        profile="0" if workload=="W-S" else "1"
        for block in range(3):
            for mode in MODE_ORDER[block]:
                pair={"workload":workload,"block":block,"mode":mode,"system_order":SYSTEM_ORDER[block],"arms":{}}
                for system in SYSTEM_ORDER[block]:
                    tag=f"{workload.replace('-','_')}_b{block}_{mode}_{'arc' if system=='ArcLLM' else 'llama'}"
                    result_path=outdir/f"{tag}.json"
                    env=os.environ.copy()
                    wall=None;rc=None;extra={}
                    if system=="ArcLLM":
                        exe=arc_trace if mode=="GPU_TRACE" else arc_control
                        cmd=[str(exe),"--model",str(model),"--shader-dir",str(ROOT/"compiled_shaders"),"--tokens",",".join(map(str,toks)),"--forced-decode-tokens",",".join(map(str,forced)),"--profile",profile,"--mode",mode,"--out",str(result_path)]
                        if mode=="PHASE_ONLY":
                            phase_path=outdir/f"{tag}_phase.json";cmd+=["--phase-out",str(phase_path)];extra["phase_file"]=phase_path.name
                        if mode=="GPU_TRACE":
                            trace_dir=outdir/f"{tag}_traces";trace_dir.mkdir()
                            cmd+=["--trace-dir",str(trace_dir),"--run-id",tag,"--timestamp-period-ns",str(a["timestamp_period_ns"]),"--timestamp-valid-bits",str(a["timestamp_valid_bits"])]
                            extra["trace_dir"]=trace_dir.name
                        rc,wall=child(cmd,env)
                        parsed=load(result_path) if result_path.exists() else {}
                        errors=validate_arc(parsed,forced)
                        if mode=="PHASE_ONLY":
                            phase_path=outdir/f"{tag}_phase.json"
                            if not phase_path.exists():
                                errors.append("phase_file_missing")
                            else:
                                phase=load(phase_path)
                                required_phase=[
                                    "model_map_inspect_ns","static_graph_contract_ns","vulkan_weight_init_ns",
                                    "request_context_prepare_ns","prefill_and_scan_ns","decode_loop_and_scans_ns",
                                    "close_and_cleanup_ns","runtime_generate_ns",
                                ]
                                if phase.get("schema")!="arcllm.core0d.arc_phase.v0.1":
                                    errors.append("phase_schema")
                                if phase.get("timing_authority")!="DIAGNOSTIC_ONLY":
                                    errors.append("phase_authority")
                                for key in required_phase:
                                    if not isinstance(phase.get(key),(int,float)) or phase.get(key)<0:
                                        errors.append(f"phase_{key}")
                        if mode=="GPU_TRACE":
                            trace_dir=outdir/f"{tag}_traces"
                            prefill=list(trace_dir.glob(f"{tag}_prefill_*.json"))
                            decode=list(trace_dir.glob(f"{tag}_decode_*.json"))
                            life=list(trace_dir.glob(f"{tag}_runtime_lifecycle.json"))
                            if len(prefill)!=1: errors.append(f"prefill_trace_count={len(prefill)}")
                            if len(decode)!=31: errors.append(f"decode_trace_count={len(decode)}")
                            if len(life)!=1: errors.append(f"lifecycle_trace_count={len(life)}")
                    else:
                        cmd=[str(llama_exe),"--model",str(model),"--workload",workload,"--out",str(result_path)]
                        stderr_path=None
                        if mode=="GPU_TRACE":
                            env["GGML_VK_PERF_LOGGER"]="1";env["GGML_VK_PERF_LOGGER_FREQUENCY"]="1";env.pop("GGML_VK_PERF_LOGGER_CONCURRENT",None)
                            stderr_path=outdir/f"{tag}_vkperf.txt"
                        else:
                            env.pop("GGML_VK_PERF_LOGGER",None);env.pop("GGML_VK_PERF_LOGGER_CONCURRENT",None);env.pop("GGML_VK_PERF_LOGGER_FREQUENCY",None)
                        rc,wall=child(cmd,env,stderr_path)
                        parsed=load(result_path) if result_path.exists() else {}
                        errors=validate_llama(parsed,workload,forced)
                        if mode=="GPU_TRACE" and stderr_path is not None:
                            try:
                                vk=parser.parse_text(stderr_path.read_text(encoding="utf-8",errors="replace"))
                                vk_path=outdir/f"{tag}_vkperf.json";vk_path.write_text(json.dumps(vk,indent=2)+"\n",encoding="utf-8")
                                extra["vkperf_file"]=vk_path.name;extra["vkperf_raw"]=stderr_path.name
                            except Exception as exc:errors.append(f"vkperf:{exc}")
                    if rc!=0:errors.append(f"returncode={rc}")
                    pair["arms"][system]={"wall_ms":wall,"returncode":rc,"valid":not errors,"errors":errors,"result_file":result_path.name,**extra}
                pair["valid"]=all(x["valid"] for x in pair["arms"].values())
                rows.append(pair)

    complete=len(rows)==18 and all(r["valid"] for r in rows)
    summary={"schema":"arcllm.core0d.measured_summary.v0.1","classification":"CORE0D_COLLECTION_COMPLETE_PENDING_ADJUDICATION" if complete else "CORE0D_COLLECTION_INCOMPLETE","measured_requests_expected":36,"measured_requests_observed":36,"core0b_performance_authority_preserved":True,"rows":rows}
    (outdir/"CORE0D_MEASURED_SUMMARY.json").write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
    print(f"CORE0D_COLLECTION={summary['classification']}")
    print(f"RESULTS_DIR={outdir}")
    return 0 if complete else 3


if __name__=="__main__":
    raise SystemExit(main())
