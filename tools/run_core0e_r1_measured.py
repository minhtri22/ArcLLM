#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, importlib.util, json, os, subprocess, time
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
MODE_ORDER={0:["CONTROL_UNINSTRUMENTED","COMBINED_PHASE_TRACE"],1:["COMBINED_PHASE_TRACE","CONTROL_UNINSTRUMENTED"],2:["CONTROL_UNINSTRUMENTED","COMBINED_PHASE_TRACE"]}
SYSTEM_ORDER={0:["ArcLLM","llama.cpp"],1:["llama.cpp","ArcLLM"],2:["ArcLLM","llama.cpp"]}
MARKERS=["child_science_window_start_ns","prefill_wall_start_ns","prefill_wall_end_ns","decode_wall_start_ns","decode_wall_end_ns","child_science_window_end_ns"]

def prompt(w):
    if w=="W-S":return [1,133151,133152,152062]
    v=[0]*256;v[:4]=[1,133151,133152,152062]
    for i in range(4,256):v[i]=1+((104729+7919*i)%152063)
    return v

def sha256(p:Path)->str:
    h=hashlib.sha256()
    with p.open("rb") as f:
        for c in iter(lambda:f.read(8*1024*1024),b""):h.update(c)
    return h.hexdigest().upper()

def load(p:Path):return json.loads(p.read_text(encoding="utf-8-sig"))

def child(cmd,env=None,stderr_path=None):
    t0=time.perf_counter_ns()
    if stderr_path is None: cp=subprocess.run(cmd,cwd=ROOT,env=env)
    else:
        with stderr_path.open("w",encoding="utf-8") as ef:cp=subprocess.run(cmd,cwd=ROOT,env=env,stderr=ef)
    return cp.returncode,time.perf_counter_ns()-t0

def load_module(path:Path,name:str):
    spec=importlib.util.spec_from_file_location(name,path);mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);return mod

def validate_markers(d):
    e=[]
    if d.get("schema")!="arcllm.core0e.phase_markers.v0.1":e.append("phase_schema")
    if d.get("timing_authority")!="DIAGNOSTIC_ONLY":e.append("phase_authority")
    if d.get("clock")!="std::chrono::steady_clock":e.append("phase_clock")
    vals=[]
    for k in MARKERS:
        v=d.get(k)
        if not isinstance(v,(int,float)):e.append(f"marker_{k}");return e
        vals.append(v)
    start,pfs,pfe,ds,de,end=vals
    if not (start < pfs < pfe <= ds < de < end):e.append("marker_order")
    return e

def validate_arc(d,forced,combined):
    e=[]
    schema="arcllm.core0e.arc_teacher_forced.v0.1" if combined else "arcllm.core0d.arc_teacher_forced.v0.1"
    if d.get("schema")!=schema:e.append("schema")
    if d.get("success") is not True or d.get("teacher_forced") is not True:e.append("success")
    if d.get("forced_decode_input_ids")!=forced:e.append("forced_ids")
    if len(d.get("predicted_token_ids",[]))!=32:e.append("predicted_count")
    s=d.get("stats",{})
    for k,v in {"prefill_dispatches":441,"prefill_submits":1,"decode_dispatches_per_step":469,"decode_submits_per_step":1,"decode_steps":31,"route_a_steps":0,"route_b_steps":31,"acquire_events":1,"evict_events":1,"b_allocations":1,"b_materializations":1,"b_validations":1,"b_releases":1,"p1_calls":1,"p3_calls":0,"p0_calls":0,"finite":True}.items():
        if s.get(k)!=v:e.append(f"{k}={s.get(k)!r}")
    return e

def validate_llama(d,w,forced,combined):
    e=[];schema="arcllm.core0e.llama_teacher_forced.v0.1" if combined else "arcllm.core0d.llama_teacher_forced.v0.1"
    if d.get("schema")!=schema:e.append("schema")
    if d.get("success") is not True or d.get("teacher_forced") is not True:e.append("success")
    if d.get("workload")!=w or d.get("forced_decode_input_ids")!=forced:e.append("trajectory")
    if len(d.get("predicted_token_ids",[]))!=32 or d.get("final_logits_finite") is not True:e.append("logits")
    p=d.get("perf",{})
    if p.get("n_eval")!=31 or p.get("n_p_eval")!=(4 if w=="W-S" else 256):e.append("perf_counts")
    r=d.get("runtime",{})
    if r.get("full_offload") is not True or r.get("offloaded_layers")!=29 or r.get("offloaded_layers_total")!=29:e.append("offload")
    return e

def fixture_self_test():
    n=0
    for w in ["W-S","W-C"]:
        for b in range(3):
            for m in MODE_ORDER[b]:
                for s in SYSTEM_ORDER[b]:n+=1
    if n!=24:raise SystemExit("CORE0E fixture schedule count")
    phase={"schema":"arcllm.core0e.phase_markers.v0.1","timing_authority":"DIAGNOSTIC_ONLY","clock":"std::chrono::steady_clock",
      **{k:(i+1)*1000 for i,k in enumerate(MARKERS)}}
    if validate_markers(phase):raise SystemExit("CORE0E marker fixture failed")
    bad=dict(phase);bad["clock"]="system_clock"
    if "phase_clock" not in validate_markers(bad):raise SystemExit("CORE0E marker clock negative fixture failed")
    equal_boundary=dict(phase);equal_boundary["decode_wall_start_ns"]=phase["prefill_wall_end_ns"]
    if validate_markers(equal_boundary):raise SystemExit("CORE0E-R1 equality-boundary fixture failed")
    decreasing=dict(phase);decreasing["decode_wall_start_ns"]=phase["prefill_wall_end_ns"]-1
    if "marker_order" not in validate_markers(decreasing):raise SystemExit("CORE0E-R1 decreasing-boundary negative fixture failed")
    equal_elsewhere=dict(phase);equal_elsewhere["prefill_wall_start_ns"]=phase["child_science_window_start_ns"]
    if "marker_order" not in validate_markers(equal_elsewhere):raise SystemExit("CORE0E-R1 non-recovery-boundary equality fixture failed")
    print("CORE0E_R1_MEASURED_RUNNER_FIXTURE_SELF_TEST=PASS");return 0

def main():
    ap=argparse.ArgumentParser();ap.add_argument("--model");ap.add_argument("--token-xray-root");ap.add_argument("--llama-root")
    ap.add_argument("--authorization",default=str(ROOT/"config"/"core0e_r1_science_authorization.json"));ap.add_argument("--fixture-self-test",action="store_true")
    args=ap.parse_args()
    if args.fixture_self_test:return fixture_self_test()
    parent_state=load(ROOT/"config"/"core0e_science_authorization.json")
    if parent_state.get("authorized") is not False or parent_state.get("status")!="CONSUMED_BY_CORE0E_COLLECTION_INCOMPLETE":raise SystemExit("STOP: parent CORE0E authorization not consumed")
    a=load(Path(args.authorization))
    if a.get("program")!="CORE-0E-R1" or a.get("authorized") is not True or a.get("measured_requests_authorized")!=24:raise SystemExit("STOP: CORE0E-R1 24-request execution not authorized")
    if a.get("rerun_authorized") is not False or a.get("selective_rerun_authorized") is not False or a.get("early_stop_authorized") is not False:raise SystemExit("STOP: CORE0E-R1 rerun/early-stop contract drift")
    if a.get("optimization_authorized") is not False or a.get("mechanism_selection_authorized") is not False or a.get("npu_authorized") is not False:raise SystemExit("STOP: CORE0E-R1 forbidden science authorization drift")
    model=Path(args.model).resolve();tx=Path(args.token_xray_root).resolve();llama=Path(args.llama_root).resolve()
    if not model.exists() or model.stat().st_size!=MODEL_BYTES or sha256(model)!=MODEL_SHA:raise SystemExit("STOP: model mismatch")
    if subprocess.check_output(["git","-C",str(tx),"rev-parse","HEAD"],text=True).strip()!=TX_HEAD:raise SystemExit("STOP: Token-XRay pin drift")
    if subprocess.check_output(["git","-C",str(llama),"rev-parse","HEAD"],text=True).strip()!=LLAMA_HEAD:raise SystemExit("STOP: llama pin drift")
    for path,blob in a.get("critical_git_blobs",{}).items():
        got=subprocess.check_output(["git","-C",str(ROOT),"rev-parse",f"HEAD:{path}"],text=True).strip()
        if got!=blob:raise SystemExit(f"STOP: critical blob drift {path}")
    shader_dir=ROOT/"compiled_shaders"
    for name,expected in a.get("active_shader_sha256",{}).items():
        if sha256(shader_dir/name)!=expected:raise SystemExit(f"STOP: shader drift {name}")
    bins={
      "arc_control":ROOT/"artifacts"/"core0d"/"core0d_arcllm_control_phase.exe",
      "llama_control":ROOT/"artifacts"/"core0d"/"core0d_llama_teacher_forced_adapter.exe",
      "arc_combined":ROOT/"artifacts"/"core0e"/"core0e_arcllm_combined.exe",
      "llama_combined":ROOT/"artifacts"/"core0e"/"core0e_llama_combined_phase_trace_adapter.exe",
    }
    for k,p in bins.items():
        if sha256(p)!=a["executable_sha256"][k]:raise SystemExit(f"STOP: {k} exe drift")
    if list((ROOT/"results").glob("core0e_r1_measured_*")):raise SystemExit("STOP: CORE0E-R1 measured dataset already exists")
    parser=load_module(ROOT/"tools"/"parse_core0d_llama_vk_perf.py","core0e_r1_vk")
    stamp=datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ");outdir=ROOT/"results"/f"core0e_r1_measured_{stamp}";outdir.mkdir()
    rows=[]
    for w in ["W-S","W-C"]:
      toks=prompt(w);forced=FORCED[w];profile="0" if w=="W-S" else "1"
      for b in range(3):
       for mode in MODE_ORDER[b]:
        pair={"workload":w,"block":b,"mode":mode,"system_order":SYSTEM_ORDER[b],"arms":{}}
        for system in SYSTEM_ORDER[b]:
          combined=mode=="COMBINED_PHASE_TRACE";tag=f"{w.replace('-','_')}_b{b}_{mode}_{'arc' if system=='ArcLLM' else 'llama'}"
          result=outdir/f"{tag}.json";phase=outdir/f"{tag}_phase.json";env=os.environ.copy();extra={};stderr=None
          if system=="ArcLLM":
            exe=bins["arc_combined"] if combined else bins["arc_control"]
            arc_mode="COMBINED_PHASE_TRACE" if combined else "CONTROL_UNINSTRUMENTED"
            cmd=[str(exe),"--model",str(model),"--shader-dir",str(shader_dir),"--tokens",",".join(map(str,toks)),"--forced-decode-tokens",",".join(map(str,forced)),"--profile",profile,"--mode",arc_mode,"--out",str(result)]
            if combined:
              trace_dir=outdir/f"{tag}_traces";trace_dir.mkdir()
              cmd += ["--phase-out",str(phase),"--trace-dir",str(trace_dir),"--run-id",tag,"--timestamp-period-ns",str(a["timestamp_period_ns"]),"--timestamp-valid-bits",str(a["timestamp_valid_bits"])]
              extra={"phase_file":phase.name,"trace_dir":trace_dir.name}
          else:
            exe=bins["llama_combined"] if combined else bins["llama_control"]
            cmd=[str(exe),"--model",str(model),"--workload",w,"--out",str(result)]
            if combined:
              cmd+=["--phase-out",str(phase)];stderr=outdir/f"{tag}_vkperf.txt"
              env["GGML_VK_PERF_LOGGER"]="1";env["GGML_VK_PERF_LOGGER_FREQUENCY"]="1";env.pop("GGML_VK_PERF_LOGGER_CONCURRENT",None)
              extra={"phase_file":phase.name,"vkperf_raw":stderr.name}
            else:
              env.pop("GGML_VK_PERF_LOGGER",None);env.pop("GGML_VK_PERF_LOGGER_FREQUENCY",None);env.pop("GGML_VK_PERF_LOGGER_CONCURRENT",None)
          rc,wall_ns=child(cmd,env,stderr);parsed=load(result) if result.exists() else {}
          errors=validate_arc(parsed,forced,combined) if system=="ArcLLM" else validate_llama(parsed,w,forced,combined)
          if combined:
            if not phase.exists():errors.append("phase_missing")
            else:errors+=validate_markers(load(phase))
            if system=="ArcLLM":
              td=outdir/f"{tag}_traces";pre=list(td.glob(f"{tag}_prefill_*.json"));dec=list(td.glob(f"{tag}_decode_*.json"));life=list(td.glob(f"{tag}_runtime_lifecycle.json"))
              if len(pre)!=1:errors.append(f"prefill_trace_count={len(pre)}")
              if len(dec)!=31:errors.append(f"decode_trace_count={len(dec)}")
              if len(life)!=1:errors.append(f"lifecycle_trace_count={len(life)}")
            else:
              try:
                vk=parser.parse_text(stderr.read_text(encoding="utf-8",errors="replace"));vkp=outdir/f"{tag}_vkperf.json";vkp.write_text(json.dumps(vk,indent=2)+"\n",encoding="utf-8");extra["vkperf_file"]=vkp.name
              except Exception as exc:errors.append(f"vkperf:{exc}")
          if rc!=0:errors.append(f"returncode={rc}")
          pair["arms"][system]={"wall_ns":wall_ns,"returncode":rc,"valid":not errors,"errors":errors,"result_file":result.name,**extra}
        pair["valid"]=all(x["valid"] for x in pair["arms"].values());rows.append(pair)
    complete=len(rows)==12 and all(r["valid"] for r in rows)
    summary={"schema":"arcllm.core0e_r1.measured_summary.v0.1","classification":"CORE0E_R1_COLLECTION_COMPLETE_PENDING_ADJUDICATION" if complete else "CORE0E_R1_COLLECTION_INCOMPLETE","measured_requests_expected":24,"measured_requests_observed":24,"rows":rows}
    (outdir/"CORE0E_R1_MEASURED_SUMMARY.json").write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
    print(f"CORE0E_R1_COLLECTION={summary['classification']}");print(f"RESULTS_DIR={outdir}");return 0 if complete else 3

if __name__=="__main__":raise SystemExit(main())
