#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, subprocess, sys, tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
CANONICAL_PARENT="368ac7b5cfefb66d67d55f2654e4f0a207c48cdc"
TX_HEAD="35f86ac68f98ffe60fc441a790274cd1f1269dfe"
LLAMA_HEAD="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"

def load(p:Path):return json.loads(p.read_text(encoding="utf-8-sig"))
def git(*a):return subprocess.check_output(["git","-C",str(ROOT),*a],text=True).strip()
def sha256(p:Path):
 h=hashlib.sha256()
 with p.open("rb") as f:
  for c in iter(lambda:f.read(8*1024*1024),b""):h.update(c)
 return h.hexdigest().upper()

def main()->int:
 ap=argparse.ArgumentParser();ap.add_argument("--token-xray-root",required=True);ap.add_argument("--llama-root",required=True);ap.add_argument("--evidence",default=str(ROOT/"results"/"CORE0D_PREFLIGHT_EVIDENCE.json"));ap.add_argument("--out",default=str(ROOT/"results"/"CORE0D_PREFLIGHT_INDEPENDENT_ADJUDICATION.json"));args=ap.parse_args()
 tx=Path(args.token_xray_root);ll=Path(args.llama_root);ev=load(Path(args.evidence));findings=[]
 def add(i,d):findings.append({"id":i,"detail":d})
 if ev.get("status")!="PASS_CORE0D_STATIC_PREFLIGHT":add("UPSTREAM_STATUS",repr(ev.get("status")))
 if ev.get("open_findings")!=0:add("UPSTREAM_FINDINGS",repr(ev.get("open_findings")))
 if ev.get("model_inference_executed") is not False or ev.get("measured_requests_executed")!=0:add("SCIENCE_BOUNDARY","violated")
 if ev.get("schedule_requests")!=36:add("SCHEDULE",repr(ev.get("schedule_requests")))
 if ev.get("science_runner_fail_closed_without_authorization") is not True:add("AUTH_GUARD","false")
 if list((ROOT/"results").glob("core0d_measured_*")):add("MEASURED_DATASET","exists")
 if (ROOT/"config"/"core0d_science_authorization.json").exists():add("PREMATURE_AUTHORIZATION","exists")

 txh=subprocess.check_output(["git","-C",str(tx),"rev-parse","HEAD"],text=True).strip()
 llh=subprocess.check_output(["git","-C",str(ll),"rev-parse","HEAD"],text=True).strip()
 if txh!=TX_HEAD:add("TOKEN_XRAY_HEAD",txh)
 if llh!=LLAMA_HEAD:add("LLAMA_HEAD",llh)
 if subprocess.check_output(["git","-C",str(tx),"status","--porcelain"],text=True).strip():add("TOKEN_XRAY_DIRTY",str(tx))
 if subprocess.check_output(["git","-C",str(ll),"status","--porcelain"],text=True).strip():add("LLAMA_DIRTY",str(ll))

 build=ev.get("build_qualification",{})
 if build.get("status")!="PASS_CORE0D_BUILD_AND_ZERO_SCIENCE_SELF_TESTS":add("BUILD_STATUS",repr(build.get("status")))
 for key,name in [("arc_control_phase_exe_sha256","core0d_arcllm_control_phase.exe"),("arc_trace_exe_sha256","core0d_arcllm_trace.exe"),("llama_adapter_exe_sha256","core0d_llama_teacher_forced_adapter.exe")]:
  p=ROOT/"artifacts"/"core0d"/name
  if not p.exists():add("EXE_MISSING",name)
  elif sha256(p)!=build.get(key):add("EXE_HASH",name)

 head=git("rev-parse","HEAD")
 impl=ev.get("implementation_head")
 if not impl or subprocess.run(["git","-C",str(ROOT),"merge-base","--is-ancestor",impl,head]).returncode!=0:add("IMPLEMENTATION_ANCESTRY",repr(impl))
 bad=subprocess.check_output(["git","-C",str(ROOT),"diff","--name-only",f"{CANONICAL_PARENT}..HEAD"],text=True).splitlines()
 bad=[p for p in bad if p in {"src/arcllm_v1_runtime.cpp","src/arcllm_v1_runtime_cli.cpp","src/arcllm_v1_vulkan_runtime_support.h","src/arcllm_v1_q4_vulkan_backend_v4_runtime.h","include/arcllm/v1/runtime.h"} or p.startswith("shaders/")]
 if bad:add("CANONICAL_DIFF",repr(bad))

 with tempfile.TemporaryDirectory(prefix="core0d-adjudicate-") as td:
  cp=subprocess.run([sys.executable,str(ROOT/"tools"/"materialize_core0d_runtime.py"),"--out-dir",td],cwd=ROOT,text=True,capture_output=True)
  if cp.returncode!=0:add("REMATERIALIZE",cp.stderr[-1000:])
  else:
   m=load(Path(td)/"CORE0D_GENERATED_RUNTIME_MANIFEST.json")
   if m.get("generated_sha256")!=build.get("generated_runtime_manifest",{}).get("generated_sha256"):add("GENERATED_HASH","mismatch")

 for cmd,marker in [
  ([sys.executable,str(ROOT/"tools"/"run_core0d_measured.py"),"--fixture-self-test"],"CORE0D_MEASURED_RUNNER_FIXTURE_SELF_TEST=PASS"),
  ([sys.executable,str(ROOT/"tools"/"parse_core0d_llama_vk_perf.py"),"--self-test"],"CORE0D_LLAMA_VK_PERF_PARSER_SELF_TEST=PASS"),
  ([str(ROOT/"artifacts"/"core0d"/"core0d_arcllm_control_phase.exe"),"--self-test"],"CORE0D_ARCLLM_CLI_SELF_TEST=PASS"),
  ([str(ROOT/"artifacts"/"core0d"/"core0d_arcllm_trace.exe"),"--self-test"],"CORE0D_ARCLLM_CLI_SELF_TEST=PASS"),
  ([str(ROOT/"artifacts"/"core0d"/"core0d_llama_teacher_forced_adapter.exe"),"--self-test"],"CORE0D_LLAMA_ADAPTER_SELF_TEST=PASS")
 ]:
  cp=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True)
  if cp.returncode!=0 or marker not in cp.stdout:add("RECOMPUTE_SELF_TEST",marker)

 result={"schema":"arcllm.core0d.preflight_independent_adjudication.v0.1","verdict":"PASS_CORE0D_ZERO_SCIENCE_PREFLIGHT" if not findings else "STOP_CORE0D_ZERO_SCIENCE_PREFLIGHT","open_findings":len(findings),"findings":findings,"implementation_head":impl,"checked_head":head,"token_xray_head":txh,"llama_head":llh,"arc_control_phase_exe_sha256":build.get("arc_control_phase_exe_sha256"),"arc_trace_exe_sha256":build.get("arc_trace_exe_sha256"),"llama_adapter_exe_sha256":build.get("llama_adapter_exe_sha256"),"model_inference_executed":False,"measured_requests_executed":0,"next_if_pass":"FREEZE_CORE0D_EXECUTION_LOCK_AND_AUTHORIZE_EXACT_36_REQUEST_COLLECTION"}
 Path(args.out).write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
 print(f"CORE0D_PREFLIGHT_ADJUDICATION={result['verdict']}");print(f"OPEN_FINDINGS={len(findings)}")
 for f in findings:print(f"FINDING {f['id']}: {f['detail']}")
 return 0 if not findings else 3

if __name__=="__main__":raise SystemExit(main())
