#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATASET = ROOT / "results" / "core0d_measured_20260929T095154508631Z"
DATASET_COMMIT = "bce0d1c84afd953264d1efb35ee93d575e34dd49"
LLAMA_COMMIT = "b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
HEADER = "Vulkan Timings:"
BROAD_TOTAL = re.compile(
    r"(?m)^Total time:\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s+us\.\s*$"
)
OLD_TOTAL_PATTERN_LITERAL = r'^Total time:\s*([0-9]+(?:\.[0-9]+)?)\s*us\.\s*$'
Q_SHAPE = re.compile(r"q\(128,([0-9]+),28,1\)")

def sha256(path: Path) -> str:
    h=hashlib.sha256()
    with path.open("rb") as f:
        for c in iter(lambda:f.read(8*1024*1024),b""):
            h.update(c)
    return h.hexdigest().upper()

def classify_numeric(token: str) -> str:
    return "SCIENTIFIC" if "e" in token.lower() else "PLAIN"

def extract_op_count(block: str) -> int:
    n=0
    for line in block.splitlines():
        if ":" in line and not line.startswith("Total time:"):
            n+=1
    return n

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--llama-root", required=True)
    ap.add_argument("--out", default=str(ROOT/"results"/"CORE0D_LLAMA_VKPERF_STRUCTURE_AUDIT.json"))
    args=ap.parse_args()
    llama=Path(args.llama_root).resolve()
    out=Path(args.out).resolve()
    findings=[]

    def add(fid:str,detail:str)->None:
        findings.append({"id":fid,"detail":detail})

    head=subprocess.check_output(["git","-C",str(ROOT),"rev-parse","HEAD"],text=True).strip()
    if subprocess.run(["git","-C",str(ROOT),"merge-base","--is-ancestor",DATASET_COMMIT,head]).returncode!=0:
        add("DATASET_ANCESTRY",f"{DATASET_COMMIT} !<= {head}")
    llama_head=subprocess.check_output(["git","-C",str(llama),"rev-parse","HEAD"],text=True).strip()
    if llama_head!=LLAMA_COMMIT:
        add("LLAMA_HEAD",llama_head)
    if subprocess.check_output(["git","-C",str(llama),"status","--porcelain"],text=True).strip():
        add("LLAMA_DIRTY",str(llama))

    parser_path=ROOT/"tools"/"parse_core0d_llama_vk_perf.py"
    parser_src=parser_path.read_text(encoding="utf-8")
    if "[0-9]+(?:\\.[0-9]+)?" not in parser_src:
        add("PARSER_PATTERN_NOT_FROZEN","expected decimal-only parser fragment absent")
    if "(?:[eE]" in parser_src:
        add("PARSER_ALREADY_ACCEPTS_EXPONENT","parser no longer represents executed collection contract")

    logger_path=llama/"ggml"/"src"/"ggml-vulkan"/"ggml-vulkan.cpp"
    logger_src=logger_path.read_text(encoding="utf-8")
    source_checks={
        "total_default_stream": 'std::cerr << "Total time: " << total_all_op_times / 1000.0 << " us."' in logger_src,
        "print_timings_at_graph_end": "ctx->perf_logger->print_timings();" in logger_src,
        "forced_submit_wait_comment": "// End the command buffer and submit/wait" in logger_src,
        "frequency_env": 'getenv("GGML_VK_PERF_LOGGER_FREQUENCY")' in logger_src,
    }
    for k,v in source_checks.items():
        if not v:add("LOGGER_SOURCE_CONTRACT",k)

    logs=sorted(DATASET.glob("*_GPU_TRACE_llama_vkperf.txt"))
    if len(logs)!=6:
        add("LOG_COUNT",str(len(logs)))

    per_log=[]
    for p in logs:
        text=p.read_text(encoding="utf-8",errors="replace")
        blocks=text.split(HEADER)[1:]
        totals=BROAD_TOTAL.findall(text)
        workload="W-S" if p.name.startswith("W_S_") else "W-C"
        expected_prefill=4 if workload=="W-S" else 256
        qdims=[]
        op_counts=[]
        for b in blocks:
            m=Q_SHAPE.search(b)
            qdims.append(int(m.group(1)) if m else None)
            op_counts.append(extract_op_count(b))
        classes=[classify_numeric(t) for t in totals]

        if len(blocks)!=32:add("HEADER_COUNT",f"{p.name}:{len(blocks)}")
        if len(totals)!=32:add("TOTAL_LINE_COUNT",f"{p.name}:{len(totals)}")
        if not qdims or qdims[0]!=expected_prefill:
            add("PREFILL_SHAPE",f"{p.name}:{qdims[:1]}")
        if len(qdims)!=32 or any(x!=1 for x in qdims[1:]):
            add("DECODE_SHAPE",f"{p.name}:{qdims[1:]}")
        if len(classes)!=32 or classes[0]!="SCIENTIFIC":
            add("FIRST_TOTAL_LEXICAL_CLASS",f"{p.name}:{classes[:1]}")
        if classes.count("SCIENTIFIC")!=1:
            add("SCIENTIFIC_COUNT",f"{p.name}:{classes.count('SCIENTIFIC')}")
        if any(c!="PLAIN" for c in classes[1:]):
            add("DECODE_TOTAL_LEXICAL_CLASS",f"{p.name}:{classes[1:]}")
        if len(op_counts)==32 and not (op_counts[0] > op_counts[1] and len(set(op_counts[1:]))==1):
            add("STRUCTURAL_OP_COUNT_PATTERN",f"{p.name}:{op_counts}")

        # Simulate executed decimal-only parser without consuming timing values.
        decimal_only=re.compile(r"(?m)^Total time:\s*([0-9]+(?:\.[0-9]+)?)\s+us\.\s*$")
        parsed_decimal=decimal_only.findall(text)
        if len(parsed_decimal)!=31:
            add("EXECUTED_PARSER_REPRODUCTION",f"{p.name}:{len(parsed_decimal)}")

        per_log.append({
            "file":p.name,
            "sha256":sha256(p),
            "workload":workload,
            "vulkan_timing_headers":len(blocks),
            "total_time_records_broad_grammar":len(totals),
            "prefill_q_sequence_length":qdims[0] if qdims else None,
            "decode_q_sequence_lengths_unique":sorted(set(x for x in qdims[1:] if x is not None)),
            "prefill_op_name_count":op_counts[0] if op_counts else None,
            "decode_op_name_count_unique":sorted(set(op_counts[1:])),
            "first_total_numeric_lexical_class":classes[0] if classes else None,
            "remaining_total_numeric_lexical_classes":sorted(set(classes[1:])),
            "scientific_total_record_count":classes.count("SCIENTIFIC"),
            "executed_decimal_only_parser_record_count":len(parsed_decimal),
            "timing_magnitudes_read_or_reported":False,
        })

    verdict = (
        "PASS_CORE0D_LLAMA_VKPERF_STRUCTURE_AUDIT_PARSER_GRAMMAR_DEFECT"
        if not findings else
        "STOP_CORE0D_LLAMA_VKPERF_STRUCTURE_AUDIT"
    )
    result={
        "schema":"arcllm.core0d.llama_vkperf_structure_audit.v0.1",
        "classification":"OUTCOME_BLIND_STRUCTURE_AND_SOURCE_AUDIT",
        "verdict":verdict,
        "open_findings":len(findings),
        "findings":findings,
        "dataset":DATASET.name,
        "dataset_commit":DATASET_COMMIT,
        "llama_commit":llama_head,
        "logger_source_sha256":sha256(logger_path),
        "parser_source_sha256":sha256(parser_path),
        "source_checks":source_checks,
        "per_log":per_log,
        "timing_values_used_for_decision":False,
        "structural_conclusion":{
            "raw_logger_groups_per_request":32,
            "group_0_semantics":"PREFILL",
            "groups_1_to_31_semantics":"CACHED_DECODE_STEPS_0_TO_30",
            "missing_logger_group":False,
            "measurement_surface_structurally_complete":True,
            "executed_parser_defect":"Decimal-only Total time grammar rejects scientific notation emitted by the first/prefill group.",
            "why_31_was_observed_by_parser":"The first Total time record is scientific notation in all six logs; the remaining 31 records are plain decimal/integer forms accepted by the executed parser."
        },
        "governance_consequence":{
            "current_collection_may_be_reinterpreted_or_reparsed_to_pass":False,
            "selective_rerun_allowed":False,
            "same_study_posthoc_parser_repair_allowed":False,
            "recommended_route":"FORMAL_CLOSE_CURRENT_CORE0D_AS_STOP_GPU_MEASUREMENT_NOT_QUALIFIED_AND_OPEN_PROSPECTIVE_CORE0D_R1_PARSER_GRAMMAR_REPAIR",
            "repair_scope":"Numeric grammar only: accept finite decimal or scientific-notation Total time records; preserve exact 32-group requirement and all G0-G4/Amdahl thresholds unchanged."
        }
    }
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(f"CORE0D_LLAMA_VKPERF_AUDIT={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    print("RAW_GROUPS=32")
    print("SEMANTICS=1_PREFILL_PLUS_31_DECODE")
    print("EXECUTED_PARSER_GROUPS=31")
    print("ROOT_CAUSE=SCIENTIFIC_NOTATION_REJECTED_BY_DECIMAL_ONLY_GRAMMAR")
    for f in findings:
        print(f"FINDING {f['id']}: {f['detail']}")
    return 0 if not findings else 3

if __name__=="__main__":
    raise SystemExit(main())
