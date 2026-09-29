#!/usr/bin/env python3
from __future__ import annotations
import argparse, importlib.util, json, subprocess, sys, tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
CANONICAL_PARENT="368ac7b5cfefb66d67d55f2654e4f0a207c48cdc"
CANONICAL_BLOBS={
 "src/arcllm_v1_runtime.cpp":"0c613f6f740931a88ddd3ee5b904533a58001a06",
 "src/arcllm_v1_runtime_cli.cpp":"5526658f6469301f0e05ffa5a46f51bc9b511816",
 "src/arcllm_v1_vulkan_runtime_support.h":"1b3a2a935134a3afa680ee4880377a20bc8a466f",
 "src/arcllm_v1_q4_vulkan_backend_v4_runtime.h":"3955d1ed27c0c48f5bbb8e4564128bf34025d1a5",
 "include/arcllm/v1/runtime.h":"d7821270ed253e1d1d96e3916b22b3daee50f5bc",
}

def load(p:Path):return json.loads(p.read_text(encoding="utf-8-sig"))
def git(*args):return subprocess.check_output(["git","-C",str(ROOT),*args],text=True).strip()

def load_runner():
 p=ROOT/"tools"/"run_core0d_measured.py"
 spec=importlib.util.spec_from_file_location("core0d_runner",p);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def main()->int:
 ap=argparse.ArgumentParser();ap.add_argument("--out",default=str(ROOT/"results"/"CORE0D_PREFLIGHT_EVIDENCE.json"));args=ap.parse_args()
 findings=[];details={}
 def add(i,d):findings.append({"id":i,"detail":d})
 prereg=load(ROOT/"config"/"core0d_excess_cost_attribution_preregistration_v0.1.json")
 build=load(ROOT/"results"/"CORE0D_BUILD_QUALIFICATION.json") if (ROOT/"results"/"CORE0D_BUILD_QUALIFICATION.json").exists() else {}
 if build.get("status")!="PASS_CORE0D_BUILD_AND_ZERO_SCIENCE_SELF_TESTS":add("BUILD_STATUS",repr(build.get("status")))
 if build.get("model_inference_executed") is not False or build.get("measured_requests_executed")!=0:add("SCIENCE_BOUNDARY",repr(build))
 if list((ROOT/"results").glob("core0d_measured_*")):add("MEASURED_DATASET_EXISTS","true")
 if (ROOT/"config"/"core0d_science_authorization.json").exists():add("PREMATURE_AUTHORIZATION","exists")

 head=git("rev-parse","HEAD")
 for p,b in CANONICAL_BLOBS.items():
  got=git("rev-parse",f"HEAD:{p}")
  if got!=b:add("CANONICAL_BLOB_DRIFT",f"{p}:{got}")
 changed=subprocess.check_output(["git","-C",str(ROOT),"diff","--name-only",f"{CANONICAL_PARENT}..HEAD"],text=True).splitlines()
 bad=[p for p in changed if p in CANONICAL_BLOBS or p.startswith("shaders/")]
 if bad:add("CANONICAL_OR_SHADER_DIFF",repr(bad))

 runner=load_runner()
 if runner.FORCED["W-S"]!=prereg["workloads"]["W-S"]["forced_decode_input_tokens"]:add("WS_FORCED_DRIFT","runner")
 if runner.FORCED["W-C"]!=prereg["workloads"]["W-C"]["forced_decode_input_tokens"]:add("WC_FORCED_DRIFT","runner")
 count=sum(1 for w in ["W-S","W-C"] for b in range(3) for m in runner.MODE_ORDER[b] for s in runner.SYSTEM_ORDER[b])
 if count!=36:add("SCHEDULE_COUNT",str(count))
 if runner.MODE_ORDER!={0:["CONTROL_UNINSTRUMENTED","PHASE_ONLY","GPU_TRACE"],1:["GPU_TRACE","CONTROL_UNINSTRUMENTED","PHASE_ONLY"],2:["PHASE_ONLY","GPU_TRACE","CONTROL_UNINSTRUMENTED"]}:add("MODE_ORDER","drift")
 if runner.SYSTEM_ORDER!={0:["ArcLLM","llama.cpp"],1:["llama.cpp","ArcLLM"],2:["ArcLLM","llama.cpp"]}:add("SYSTEM_ORDER","drift")

 for script,marker in [
  ("tools/run_core0d_measured.py","CORE0D_MEASURED_RUNNER_FIXTURE_SELF_TEST=PASS"),
  ("tools/parse_core0d_llama_vk_perf.py","CORE0D_LLAMA_VK_PERF_PARSER_SELF_TEST=PASS")
 ]:
  cp=subprocess.run([sys.executable,str(ROOT/script),"--fixture-self-test"] if "run_core0d" in script else [sys.executable,str(ROOT/script),"--self-test"],cwd=ROOT,text=True,capture_output=True)
  details[script]={"returncode":cp.returncode,"stdout":cp.stdout[-3000:],"stderr":cp.stderr[-1500:]}
  if cp.returncode!=0 or marker not in cp.stdout:add("SELF_TEST",script)

 absent=ROOT/"results"/"INTENTIONALLY_ABSENT_CORE0D_AUTH.json"
 if absent.exists():absent.unlink()
 cp=subprocess.run([sys.executable,str(ROOT/"tools"/"run_core0d_measured.py"),"--model","UNUSED","--token-xray-root","UNUSED","--llama-root","UNUSED","--authorization",str(absent)],cwd=ROOT,text=True,capture_output=True)
 fail_closed=cp.returncode!=0 and "authorization artifact is absent" in ((cp.stdout or "")+(cp.stderr or ""))
 if not fail_closed:add("AUTH_GUARD",(cp.stdout+cp.stderr)[-1000:])

 with tempfile.TemporaryDirectory(prefix="core0d-preflight-") as td:
  cp=subprocess.run([sys.executable,str(ROOT/"tools"/"materialize_core0d_runtime.py"),"--out-dir",td],cwd=ROOT,text=True,capture_output=True)
  if cp.returncode!=0:add("REMATERIALIZE",cp.stderr[-1000:])
  else:
   gen=load(Path(td)/"CORE0D_GENERATED_RUNTIME_MANIFEST.json")
   if gen.get("generated_sha256")!=build.get("generated_runtime_manifest",{}).get("generated_sha256"):add("GENERATED_HASH_REPRO","mismatch")
   control=(Path(td)/"core0d_arcllm_control_phase_runtime.cpp").read_text(encoding="utf-8")
   trace=(Path(td)/"core0d_arcllm_trace_runtime.cpp").read_text(encoding="utf-8")
   for key in ["model_map_inspect_ns","static_graph_contract_ns","vulkan_weight_init_ns","request_context_prepare_ns","prefill_and_scan_ns","decode_loop_and_scans_ns","close_and_cleanup_ns","runtime_generate_ns"]:
    if key not in control:add("PHASE_BOUNDARY_MISSING",key)
   if "core0c_token_xray_bridge" in control or "VkQueryPool" in control:add("CONTROL_TRACE_CONTAMINATION","control contains trace surface")
   if "core0d_forced_decode_ids" not in control or "core0d_forced_decode_ids" not in trace:add("TEACHER_FORCE_MISSING","generated")
   if "core0c_token_xray_bridge" not in (Path(td)/"core0d_arcllm_trace_vulkan_runtime_support.h").read_text(encoding="utf-8"):add("TRACE_BRIDGE_MISSING","support")

 llama_src=(ROOT/"baseline"/"core0d_llama_teacher_forced_adapter.cpp").read_text(encoding="utf-8")
 for token in ["llama_perf_context_reset","forced_decode","predicted.push_back","n_eval==31"]:
  if token not in llama_src:add("LLAMA_ADAPTER_CONTRACT",token)
 parser=(ROOT/"tools"/"parse_core0d_llama_vk_perf.py").read_text(encoding="utf-8")
 if "len(blocks) != 32" not in parser:add("LLAMA_PARSER_CARDINALITY","missing")
 if prereg["authorization"]["measured_execution"] is not False or prereg["authorization"]["measured_requests_authorized"]!=0:add("PREREG_AUTH","not zero")

 result={
  "schema":"arcllm.core0d.preflight_evidence.v0.1",
  "status":"PASS_CORE0D_STATIC_PREFLIGHT" if not findings else "STOP_CORE0D_STATIC_PREFLIGHT",
  "classification":"ZERO_SCIENCE_IMPLEMENTATION_STATIC_PREFLIGHT",
  "implementation_head":head,
  "build_qualification":build,
  "schedule_requests":count,
  "common_trajectory":{"W-S_forced_count":len(runner.FORCED["W-S"]),"W-C_forced_count":len(runner.FORCED["W-C"]),"predicted_feedback_forbidden":True},
  "modes":["CONTROL_UNINSTRUMENTED","PHASE_ONLY","GPU_TRACE"],
  "phase_boundaries":["model_map_inspect_ns","static_graph_contract_ns","vulkan_weight_init_ns","request_context_prepare_ns","prefill_and_scan_ns","decode_loop_and_scans_ns","close_and_cleanup_ns","runtime_generate_ns"],
  "llama_gpu_logger":{"expected_blocks":32,"prefill_blocks":1,"decode_blocks":31,"timing_authority":"DIAGNOSTIC_ONLY"},
  "science_runner_fail_closed_without_authorization":fail_closed,
  "model_inference_executed":False,
  "measured_requests_executed":0,
  "canonical_runtime_modified":False,
  "kernel_source_modified":False,
  "details":details,
  "findings":findings,"open_findings":len(findings)
 }
 Path(args.out).write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
 print(f"CORE0D_PREFLIGHT={result['status']}");print(f"OPEN_FINDINGS={len(findings)}");print(f"SCHEDULE_REQUESTS={count}")
 for f in findings:print(f"FINDING {f['id']}: {f['detail']}")
 return 0 if not findings else 3

if __name__=="__main__":raise SystemExit(main())
