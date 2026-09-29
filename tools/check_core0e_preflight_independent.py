#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, subprocess, tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
PARENT_HEAD="3e06a1dede881a1246b5e16702afd83bb68dc420"
PARENT_DATASET=ROOT/"results"/"core0d_measured_20260929T121751651960Z"
CANONICAL=[
 "src/arcllm_v1_runtime.cpp","src/arcllm_v1_runtime_cli.cpp",
 "src/arcllm_v1_vulkan_runtime_support.h","src/arcllm_v1_q4_vulkan_backend_v4_runtime.h",
 "include/arcllm/v1/runtime.h","config/arcllm_v1_runtime_active_v0.2.json"
]
MARKERS=["child_science_window_start_ns","prefill_wall_start_ns","prefill_wall_end_ns","decode_wall_start_ns","decode_wall_end_ns","child_science_window_end_ns"]

def git(*args):return subprocess.check_output(["git","-C",str(ROOT),*args],text=True).strip()
def sha256(p):
 h=hashlib.sha256()
 with Path(p).open("rb") as f:
  for c in iter(lambda:f.read(8*1024*1024),b""):h.update(c)
 return h.hexdigest().upper()

def main():
 ap=argparse.ArgumentParser();ap.add_argument("--out",default=str(ROOT/"results"/"CORE0E_PREFLIGHT_INDEPENDENT_ADJUDICATION.json"));args=ap.parse_args()
 findings=[]
 def add(i,d):findings.append({"id":i,"detail":d})
 cfg=json.loads((ROOT/"config/core0e_outside_gpu_phase_attribution_preregistration_v0.1.json").read_text())
 evp=ROOT/"results/CORE0E_PREFLIGHT_EVIDENCE.json";bqp=ROOT/"results/CORE0E_BUILD_QUALIFICATION.json"
 if not evp.exists():add("PRIMARY_EVIDENCE","missing");ev={}
 else:
  ev=json.loads(evp.read_text())
  if ev.get("verdict")!="PASS_CORE0E_ZERO_SCIENCE_PREFLIGHT" or ev.get("open_findings")!=0:add("PRIMARY_EVIDENCE",repr((ev.get("verdict"),ev.get("open_findings"))))
 if not bqp.exists():add("BUILD","missing");bq={}
 else:
  bq=json.loads(bqp.read_text())
  if bq.get("status")!="PASS_CORE0E_BUILD_AND_ZERO_SCIENCE_SELF_TESTS":add("BUILD",repr(bq.get("status")))
 if cfg.get("authorization",{}).get("measured_requests_authorized")!=0:add("AUTH","prereg measured authorization nonzero")
 changed=git("diff","--name-only",PARENT_HEAD,"HEAD").splitlines()
 if any(p in CANONICAL or p.startswith("shaders/") for p in changed):add("CANONICAL_MUTATION",repr(changed))
 for cmd,token,label in [
  (["py","-3",str(ROOT/"tools/core0e_phase_contract.py"),"--self-test"],"CORE0E_PHASE_CONTRACT_SELF_TEST=PASS","PHASE"),
  (["py","-3",str(ROOT/"tools/run_core0e_measured.py"),"--fixture-self-test"],"CORE0E_MEASURED_RUNNER_FIXTURE_SELF_TEST=PASS","RUNNER")]:
  cp=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True)
  if cp.returncode or token not in cp.stdout:add(label,(cp.stdout+cp.stderr)[-1200:])
 if bq:
  for key,name in [("arc_combined_exe_sha256","core0e_arcllm_combined.exe"),("llama_combined_exe_sha256","core0e_llama_combined_phase_trace_adapter.exe")]:
   p=ROOT/"artifacts/core0e"/name
   if not p.exists() or sha256(p)!=bq.get(key):add("BINARY",name)
   elif subprocess.run([str(p),"--self-test"],cwd=ROOT,capture_output=True,text=True).returncode!=0:add("BINARY_SELFTEST",name)
 with tempfile.TemporaryDirectory(prefix="core0e_ind_") as td:
  cp=subprocess.run(["py","-3",str(ROOT/"tools/materialize_core0e_runtime.py"),"--out-dir",td],cwd=ROOT,capture_output=True,text=True)
  if cp.returncode:add("REMATERIALIZE",(cp.stdout+cp.stderr)[-1200:])
  else:
   m=json.loads((Path(td)/"CORE0E_GENERATED_RUNTIME_MANIFEST.json").read_text())
   if bq and m.get("generated_sha256")!=bq.get("generated_runtime_manifest",{}).get("generated_sha256"):add("GENERATED_HASH","mismatch")
   src=(Path(td)/"core0e_arcllm_combined_runtime.cpp").read_text()
   pos=[src.find(x.replace("_ns","")) for x in MARKERS]
   # Markers are variable names without _ns in C++ until sidecar keys.
   if any(x<0 for x in pos):add("ARC_MARKERS","missing")
   if not (src.find("arcllm_core0c::lifecycle().flush();")<src.find("core0e_child_end")<src.find("std::ofstream po")):add("ARC_BOUNDARY_ORDER","invalid")
 lsrc=(ROOT/"baseline/core0e_llama_combined_phase_trace_adapter.cpp").read_text()
 order=["child_start","prefill_start","prefill_end","decode_start","decode_end","child_end"]
 p=[lsrc.find(x) for x in order]
 if any(x<0 for x in p) or p!=sorted(p):add("LLAMA_ORDER",repr(p))
 if lsrc.find("child_end")>lsrc.find("std::ofstream o(out_path"):add("LLAMA_SERIALIZATION","inside child window")
 if not PARENT_DATASET.exists():add("PARENT_DATASET","missing")
 else:
  # Structure only; no timing magnitudes are parsed.
  summary=json.loads((PARENT_DATASET/"CORE0D_MEASURED_SUMMARY.json").read_text())
  if summary.get("measured_requests_observed")!=36:add("PARENT_CARDINALITY",repr(summary.get("measured_requests_observed")))
  if len(list(PARENT_DATASET.glob("*GPU_TRACE_arc_traces")))!=6:add("PARENT_ARC_TRACE","count")
  if len(list(PARENT_DATASET.glob("*GPU_TRACE_llama_vkperf.txt")))!=6:add("PARENT_LLAMA_TRACE","count")
 if list((ROOT/"results").glob("core0e_measured_*")):add("MEASURED_DATASET","already exists")
 verdict="PASS_CORE0E_INDEPENDENT_PREFLIGHT" if not findings else "STOP_CORE0E_INDEPENDENT_PREFLIGHT"
 out={"schema":"arcllm.core0e.preflight_independent_adjudication.v0.1","verdict":verdict,"open_findings":len(findings),"findings":findings,"checked_head":git("rev-parse","HEAD"),"parent_phase_timing_magnitudes_used":False,"model_inference_executed":False,"measured_requests_executed":0,"changed_files_from_prereg":changed}
 Path(args.out).write_text(json.dumps(out,indent=2)+"\n")
 print(f"CORE0E_INDEPENDENT_PREFLIGHT={verdict}");print(f"OPEN_FINDINGS={len(findings)}");print("MODEL_INFERENCE_EXECUTED=false");print("MEASURED_REQUESTS_EXECUTED=0")
 for f in findings:print(f"FINDING {f['id']}: {f['detail']}")
 return 0 if not findings else 3
if __name__=="__main__":raise SystemExit(main())
