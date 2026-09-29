#!/usr/bin/env python3
from __future__ import annotations

import argparse
import importlib.util
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE_COMMIT = "32fe037d0bd6efb1e20760d2dd8c535d8abd931c"
FAILED_DATASET_COMMIT = "bce0d1c84afd953264d1efb35ee93d575e34dd49"
FAILED_DATASET_PATH = "results/core0d_measured_20260929T095154508631Z"
NUMERIC = r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"
TOTAL = re.compile(rf"^Total time:\s*({NUMERIC})\s*us\.\s*$", re.MULTILINE)
Q = re.compile(r"q\(128,([0-9]+),28,1\)")
HEADER = "Vulkan Timings:"

UNCHANGED = {
    "baseline/core0d_llama_teacher_forced_adapter.cpp": "acffb1db5d6cff3be6a863dfc67ab4018edd083b",
    "src/core0d_arcllm_teacher_forced_cli.cpp": "d2bb7b7f4475b52523c849cb4fb1a1355a9b788e",
    "tools/materialize_core0d_runtime.py": "62d0040bd5205e64988b92ed36731a21333b0b7e",
    "tools/run_core0d_measured.py": "9566cf90da18069ddc8259ece16261b5ca6b5cac",
    "src/arcllm_v1_runtime.cpp": "0c613f6f740931a88ddd3ee5b904533a58001a06",
    "src/arcllm_v1_runtime_cli.cpp": "5526658f6469301f0e05ffa5a46f51bc9b511816",
    "src/arcllm_v1_vulkan_runtime_support.h": "1b3a2a935134a3afa680ee4880377a20bc8a466f",
    "src/arcllm_v1_q4_vulkan_backend_v4_runtime.h": "3955d1ed27c0c48f5bbb8e4564128bf34025d1a5",
    "include/arcllm/v1/runtime.h": "d7821270ed253e1d1d96e3916b22b3daee50f5bc",
}


def git(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def load_parser():
    p = ROOT / "tools" / "parse_core0d_llama_vk_perf.py"
    spec = importlib.util.spec_from_file_location("core0d_r1_parser_independent", p)
    if spec is None or spec.loader is None:
        raise RuntimeError("parser module load failed")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def fixture(total: str, *, prefill_q: int = 4, groups: int = 32, decode_q: int = 1) -> str:
    out = []
    for i in range(groups):
        qn = prefill_q if i == 0 else decode_q
        token = total if i == 0 else "12345.5"
        out.append("----------------\nVulkan Timings:\n")
        out.append(
            f"FLASH_ATTN_EXT dst(128,28,{qn},1),  q(128,{qn},28,1),  "
            f"k(128,256,4,1),  v(128,256,4,1),  m(256,{qn},1,1): 1 x 1 us = 1 us\n"
        )
        out.append(f"Total time: {token} us.\n")
    return "".join(out)


def expect_reject(parser, text: str) -> bool:
    try:
        parser.parse_text(text)
    except ValueError:
        return True
    return False


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--out", default=str(ROOT/"results"/"CORE0D_R1_PREFLIGHT_INDEPENDENT_ADJUDICATION.json"))
    args=ap.parse_args()
    out=Path(args.out)
    findings=[]

    def add(fid, detail):
        findings.append({"id":fid,"detail":detail})

    ev=json.loads((ROOT/"results"/"CORE0D_R1_PREFLIGHT_EVIDENCE.json").read_text(encoding="utf-8"))
    if ev.get("verdict")!="PASS_CORE0D_R1_PARSER_REPAIR_ZERO_SCIENCE_PREFLIGHT":
        add("EVIDENCE_VERDICT",repr(ev.get("verdict")))
    if ev.get("open_findings")!=0:add("EVIDENCE_FINDINGS",str(ev.get("open_findings")))
    if ev.get("model_inference_executed") is not False:add("MODEL_INFERENCE_BOUNDARY",repr(ev.get("model_inference_executed")))
    if ev.get("measured_requests_executed")!=0:add("MEASURED_BOUNDARY",repr(ev.get("measured_requests_executed")))
    if subprocess.run(["git","-C",str(ROOT),"merge-base","--is-ancestor",EVIDENCE_COMMIT,"HEAD"]).returncode!=0:
        add("EVIDENCE_ANCESTRY",EVIDENCE_COMMIT)

    prereg=json.loads((ROOT/"config"/"core0d_r1_llama_vkperf_numeric_grammar_repair_preregistration_v0.1.json").read_text(encoding="utf-8"))
    if prereg["only_authorized_change"]["new_numeric_grammar"]!=NUMERIC:
        add("PREREG_GRAMMAR",repr(prereg["only_authorized_change"]["new_numeric_grammar"]))
    if prereg["authorization"]["measured_execution"] is not False or prereg["authorization"]["measured_requests_authorized"]!=0:
        add("PREREG_AUTH",repr(prereg["authorization"]))

    parser=load_parser()
    if getattr(parser,"NUMERIC",None)!=NUMERIC:add("IMPLEMENTED_GRAMMAR",repr(getattr(parser,"NUMERIC",None)))
    if getattr(parser,"TOTAL_RE",None) is None:add("TOTAL_RE","absent")

    for tok in ["12345","12345.5","1.2345e+05","1.2345E+05","1.2345e-05"]:
        try:
            r=parser.parse_text(fixture(tok))
        except Exception as exc:
            add("POSITIVE_FIXTURE",f"{tok}:{exc}")
            continue
        if r.get("block_count")!=32 or r.get("decode_step_count")!=31:
            add("POSITIVE_CARDINALITY",f"{tok}:{r.get('block_count')}/{r.get('decode_step_count')}")
    try:
        wc=parser.parse_text(fixture("1.2345e+05",prefill_q=256))
        if wc.get("prefill_q_sequence_length")!=256:add("WC_FIXTURE_SHAPE",repr(wc.get("prefill_q_sequence_length")))
    except Exception as exc:add("WC_FIXTURE",str(exc))

    for tok in ["nan","inf","-inf"]:
        if not expect_reject(parser,fixture(tok)):add("NONFINITE_ACCEPTED",tok)
    if not expect_reject(parser,fixture("1",groups=31)):add("CARDINALITY_31_ACCEPTED","31")
    if not expect_reject(parser,fixture("1",groups=33)):add("CARDINALITY_33_ACCEPTED","33")
    if not expect_reject(parser,fixture("1",prefill_q=1)):add("DECODE_AS_PREFILL_ACCEPTED","q=1")
    if not expect_reject(parser,fixture("1",decode_q=4)):add("PREFILL_AS_DECODE_ACCEPTED","q=4")

    dataset=ROOT/FAILED_DATASET_PATH
    logs=sorted(dataset.glob("*_GPU_TRACE_llama_vkperf.txt"))
    if len(logs)!=6:add("STRUCTURE_LOG_COUNT",str(len(logs)))
    structural=[]
    for p in logs:
        text=p.read_text(encoding="utf-8",errors="replace")
        blocks=text.split(HEADER)[1:]
        workload="W-S" if p.name.startswith("W_S_") else "W-C"
        expected=4 if workload=="W-S" else 256
        qvals=[]
        lexical=[]
        for b in blocks:
            qm=Q.search(b)
            qvals.append(int(qm.group(1)) if qm else None)
            tm=TOTAL.findall(b)
            if len(tm)!=1:
                add("STRUCTURE_TOTAL_COUNT",f"{p.name}:{len(tm)}")
                lexical.append("INVALID")
            else:
                lexical.append("SCIENTIFIC" if "e" in tm[0].lower() else "PLAIN")
        if len(blocks)!=32:add("STRUCTURE_HEADERS",f"{p.name}:{len(blocks)}")
        if qvals[:1]!=[expected]:add("STRUCTURE_PREFILL",f"{p.name}:{qvals[:1]}")
        if len(qvals)!=32 or any(v!=1 for v in qvals[1:]):add("STRUCTURE_DECODE",f"{p.name}:{qvals[1:]}")
        structural.append({"file":p.name,"groups":len(blocks),"prefill_qn":qvals[0] if qvals else None,"decode_qn_unique":sorted(set(qvals[1:])) if len(qvals)>1 else [],"lexical_classes":lexical,"timing_magnitudes_used":False})

    changed=subprocess.check_output(["git","-C",str(ROOT),"diff","--name-only",FAILED_DATASET_COMMIT,"HEAD","--",FAILED_DATASET_PATH],text=True).splitlines()
    if changed:add("FAILED_DATASET_MUTATED",repr(changed))

    for path,expected in UNCHANGED.items():
        got=git("rev-parse",f"HEAD:{path}")
        if got!=expected:add("UNCHANGED_BLOB_DRIFT",f"{path}:{got}!={expected}")

    r1dirs=sorted(p.name for p in (ROOT/"results").glob("core0d_r1_measured_*") if p.is_dir())
    if r1dirs:add("R1_MEASURED_DATASET_EXISTS",repr(r1dirs))

    verdict="PASS_CORE0D_R1_PARSER_REPAIR_INDEPENDENT_PREFLIGHT" if not findings else "STOP_CORE0D_R1_PARSER_REPAIR_INDEPENDENT_PREFLIGHT"
    result={
        "schema":"arcllm.core0d_r1.preflight_independent_adjudication.v0.1",
        "verdict":verdict,
        "open_findings":len(findings),
        "findings":findings,
        "checked_head":git("rev-parse","HEAD"),
        "upstream_evidence_commit":EVIDENCE_COMMIT,
        "numeric_grammar":NUMERIC,
        "synthetic_positive_count":5,
        "synthetic_nonfinite_rejection_count":3,
        "structural_negative_fixture_count":4,
        "old_failed_dataset_structure_logs":structural,
        "old_failed_dataset_timing_magnitudes_used":False,
        "old_failed_dataset_mutated":False if not changed else True,
        "unchanged_runner_adapters_and_canonical_blobs":not any(f["id"]=="UNCHANGED_BLOB_DRIFT" for f in findings),
        "model_inference_executed":False,
        "measured_requests_executed":0,
        "next_if_pass":"FREEZE_CORE0D_R1_EXECUTION_LOCK_AND_AUTHORIZE_ONE_FRESH_36_REQUEST_COLLECTION",
    }
    out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(f"CORE0D_R1_INDEPENDENT_PREFLIGHT={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    print("MODEL_INFERENCE_EXECUTED=false")
    print("MEASURED_REQUESTS_EXECUTED=0")
    for x in findings:print(f"FINDING {x['id']}: {x['detail']}")
    return 0 if not findings else 3

if __name__=="__main__":raise SystemExit(main())
