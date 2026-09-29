#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, subprocess, tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
PARENT_HEAD="3e06a1dede881a1246b5e16702afd83bb68dc420"
CANONICAL={
 "src/arcllm_v1_runtime.cpp":"0c613f6f740931a88ddd3ee5b904533a58001a06",
 "src/arcllm_v1_runtime_cli.cpp":"5526658f6469301f0e05ffa5a46f51bc9b511816",
 "src/arcllm_v1_vulkan_runtime_support.h":"1b3a2a935134a3afa680ee4880377a20bc8a466f",
 "src/arcllm_v1_q4_vulkan_backend_v4_runtime.h":"3955d1ed27c0c48f5bbb8e4564128bf34025d1a5",
 "include/arcllm/v1/runtime.h":"d7821270ed253e1d1d96e3916b22b3daee50f5bc",
 "config/arcllm_v1_runtime_active_v0.2.json":"8e60d8b60ad5750e1c3bdbd472fdd15f0d4bcfb4",
}
LLAMA_HEAD="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
TX_HEAD="35f86ac68f98ffe60fc441a790274cd1f1269dfe"
PARENT_DATASET=ROOT/"results"/"core0d_measured_20260929T121751651960Z"

def git(*args):return subprocess.check_output(["git","-C",str(ROOT),*args],text=True).strip()
def blob(path):return git("rev-parse",f"HEAD:{path}")
def sha256(p:Path):
 h=hashlib.sha256()
 with p.open("rb") as f:
  for c in iter(lambda:f.read(8*1024*1024),b""):h.update(c)
 return h.hexdigest().upper()

def main():
 ap=argparse.ArgumentParser();ap.add_argument("--llama-root",required=True);ap.add_argument("--token-xray-root",required=True);ap.add_argument("--out",default=str(ROOT/"results"/"CORE0E_PREFLIGHT_EVIDENCE.json"))
 args=ap.parse_args();llama=Path(args.llama_root);tx=Path(args.token_xray_root);findings=[]
 def add(i,d):findings.append({"id":i,"detail":d})
 cfg=json.loads((ROOT/"config"/"core0e_outside_gpu_phase_attribution_preregistration_v0.1.json").read_text())
 if cfg.get("status")!="PREREGISTERED_NOT_AUTHORIZED_FOR_MEASURED_EXECUTION":add("PREREG_STATUS",repr(cfg.get("status")))
 if cfg.get("authorization",{}).get("measured_execution") is not False or cfg.get("authorization",{}).get("measured_requests_authorized")!=0:add("MEASURED_AUTH",repr(cfg.get("authorization")))
 for p,e in CANONICAL.items():
  g=blob(p)
  if g!=e:add("CANONICAL_DRIFT",f"{p}:{g}!={e}")
 if subprocess.check_output(["git","-C",str(llama),"rev-parse","HEAD"],text=True).strip()!=LLAMA_HEAD:add("LLAMA_HEAD","drift")
 if subprocess.check_output(["git","-C",str(tx),"rev-parse","HEAD"],text=True).strip()!=TX_HEAD:add("TX_HEAD","drift")
 if subprocess.check_output(["git","-C",str(llama),"status","--porcelain"],text=True).strip():add("LLAMA_DIRTY",str(llama))
 if subprocess.check_output(["git","-C",str(tx),"status","--porcelain"],text=True).strip():add("TX_DIRTY",str(tx))
 for p in ["tools/materialize_core0e_runtime.py","tools/core0e_phase_contract.py","tools/run_core0e_measured.py"]:
  cp=subprocess.run(["py","-3","-m","py_compile",str(ROOT/p)],capture_output=True,text=True)
  if cp.returncode:add("PY_COMPILE",f"{p}:{cp.stderr[-1200:]}")
 for cmd,label,token in [
  (["py","-3",str(ROOT/"tools/core0e_phase_contract.py"),"--self-test"],"PHASE_FIXTURE","CORE0E_PHASE_CONTRACT_SELF_TEST=PASS"),
  (["py","-3",str(ROOT/"tools/run_core0e_measured.py"),"--fixture-self-test"],"RUNNER_FIXTURE","CORE0E_MEASURED_RUNNER_FIXTURE_SELF_TEST=PASS"),
  (["py","-3",str(ROOT/"tools/parse_core0d_llama_vk_perf.py"),"--self-test"],"VK_PARSER","CORE0D_R1_LLAMA_VK_PERF_PARSER_SELF_TEST=PASS")]:
  cp=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True)
  if cp.returncode or token not in cp.stdout:add(label,(cp.stdout+cp.stderr)[-1500:])
 qpath=ROOT/"results"/"CORE0E_BUILD_QUALIFICATION.json"
 if not qpath.exists():add("BUILD_QUALIFICATION","missing");q={}
 else:
  q=json.loads(qpath.read_text())
  if q.get("status")!="PASS_CORE0E_BUILD_AND_ZERO_SCIENCE_SELF_TESTS":add("BUILD_STATUS",repr(q.get("status")))
  if q.get("model_inference_executed") is not False or q.get("measured_requests_executed")!=0:add("BUILD_SCIENCE_BOUNDARY",repr(q))
 for k,p in {"arc_combined":ROOT/"artifacts/core0e/core0e_arcllm_combined.exe","llama_combined":ROOT/"artifacts/core0e/core0e_llama_combined_phase_trace_adapter.exe"}.items():
  if not p.exists():add("EXE_MISSING",str(p))
  elif q and sha256(p)!=q.get(f"{k}_exe_sha256"):add("EXE_HASH",k)
 with tempfile.TemporaryDirectory(prefix="core0e_preflight_") as td:
  cp=subprocess.run(["py","-3",str(ROOT/"tools/materialize_core0e_runtime.py"),"--out-dir",td],cwd=ROOT,capture_output=True,text=True)
  if cp.returncode:add("REMATERIALIZE",(cp.stdout+cp.stderr)[-1500:])
  else:
   m=json.loads((Path(td)/"CORE0E_GENERATED_RUNTIME_MANIFEST.json").read_text())
   qm=q.get("generated_runtime_manifest",{}) if q else {}
   if m.get("generated_sha256")!=qm.get("generated_sha256"):add("REMATERIALIZE_HASH","manifest mismatch")
   src=(Path(td)/"core0e_arcllm_combined_runtime.cpp").read_text()
   required=["core0e_child_start","core0e_prefill_start","core0e_prefill_end","core0e_decode_start","core0e_decode_end","core0e_child_end","ARCLLM_CORE0E_PHASE_OUT"]
   if any(x not in src for x in required):add("ARC_MARKERS","missing source marker")
   if not (src.find("arcllm_core0c::lifecycle().flush();")<src.find("const auto core0e_child_end")<src.find("std::ofstream po")):add("ARC_CHILD_END_ORDER","flush/end/sidecar order")
 llama_src=(ROOT/"baseline/core0e_llama_combined_phase_trace_adapter.cpp").read_text()
 order=["child_start","prefill_start","prefill_end","decode_start","decode_end","child_end"]
 pos=[llama_src.find(x) for x in order]
 if any(x<0 for x in pos) or pos!=sorted(pos):add("LLAMA_MARKER_ORDER",repr(pos))
 if not (llama_src.find("const auto child_end")<llama_src.find("std::ofstream o(out_path")):add("LLAMA_SERIALIZATION_ORDER","result serialization not after child_end")
 if not PARENT_DATASET.exists():add("PARENT_DATASET","missing")
 else:
  summary=json.loads((PARENT_DATASET/"CORE0D_MEASURED_SUMMARY.json").read_text())
  if summary.get("measured_requests_observed")!=36:add("PARENT_STRUCTURE","request count")
  arc_dirs=list(PARENT_DATASET.glob("*GPU_TRACE_arc_traces"));llama_logs=list(PARENT_DATASET.glob("*GPU_TRACE_llama_vkperf.txt"))
  if len(arc_dirs)!=6 or len(llama_logs)!=6:add("PARENT_TRACE_STRUCTURE",f"{len(arc_dirs)}/{len(llama_logs)}")
 if list((ROOT/"results").glob("core0e_measured_*")):add("MEASURED_ALREADY_EXISTS","CORE0E dataset present")
 changed=git("diff","--name-only",PARENT_HEAD,"HEAD").splitlines()
 if any(p in CANONICAL or p.startswith("shaders/") for p in changed):add("PRODUCT_SOURCE_MUTATION",repr(changed))
 verdict="PASS_CORE0E_ZERO_SCIENCE_PREFLIGHT" if not findings else "STOP_CORE0E_PREFLIGHT"
 result={"schema":"arcllm.core0e.preflight_evidence.v0.1","classification":"ZERO_SCIENCE_IMPLEMENTATION_STATIC_PREFLIGHT","verdict":verdict,"open_findings":len(findings),"findings":findings,"checked_head":git("rev-parse","HEAD"),"parent_head":PARENT_HEAD,"build_qualification":q,"parent_dataset_use":"STRUCTURE_ONLY","parent_phase_timing_magnitudes_used":False,"model_inference_executed":False,"measured_requests_executed":0,"changed_files_from_prereg":changed}
 Path(args.out).write_text(json.dumps(result,indent=2)+"\n")
 print(f"CORE0E_PREFLIGHT={verdict}");print(f"OPEN_FINDINGS={len(findings)}");print("MODEL_INFERENCE_EXECUTED=false");print("MEASURED_REQUESTS_EXECUTED=0")
 for f in findings:print(f"FINDING {f['id']}: {f['detail']}")
 return 0 if not findings else 3
if __name__=="__main__":raise SystemExit(main())
