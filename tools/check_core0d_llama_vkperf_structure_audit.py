#!/usr/bin/env python3
from __future__ import annotations
import argparse,json,re,subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DATASET=ROOT/"results"/"core0d_measured_20260929T095154508631Z"
AUDIT=ROOT/"results"/"CORE0D_LLAMA_VKPERF_STRUCTURE_AUDIT.json"
LLAMA_COMMIT="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
HEADER="Vulkan Timings:"
BROAD=re.compile(r"(?m)^Total time:\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s+us\.\s*$")
Q=re.compile(r"q\(128,([0-9]+),28,1\)")

def main()->int:
    ap=argparse.ArgumentParser(); ap.add_argument("--llama-root",required=True); ap.add_argument("--out",default=str(ROOT/"results"/"CORE0D_LLAMA_VKPERF_STRUCTURE_AUDIT_INDEPENDENT.json")); args=ap.parse_args()
    llama=Path(args.llama_root).resolve(); out=Path(args.out)
    f=[]
    def add(i,d):f.append({"id":i,"detail":d})
    a=json.loads(AUDIT.read_text(encoding="utf-8"))
    if a.get("verdict")!="PASS_CORE0D_LLAMA_VKPERF_STRUCTURE_AUDIT_PARSER_GRAMMAR_DEFECT" or a.get("open_findings")!=0:add("UPSTREAM_AUDIT",repr((a.get("verdict"),a.get("open_findings"))))
    if a.get("timing_values_used_for_decision") is not False:add("OUTCOME_BLIND_BOUNDARY","timing values used")
    lh=subprocess.check_output(["git","-C",str(llama),"rev-parse","HEAD"],text=True).strip()
    if lh!=LLAMA_COMMIT:add("LLAMA_HEAD",lh)
    logs=sorted(DATASET.glob("*_GPU_TRACE_llama_vkperf.txt"))
    if len(logs)!=6:add("LOG_COUNT",str(len(logs)))
    rows=[]
    for p in logs:
        txt=p.read_text(encoding="utf-8",errors="replace")
        blocks=txt.split(HEADER)[1:]; totals=BROAD.findall(txt)
        w="W-S" if p.name.startswith("W_S_") else "W-C"; pre=4 if w=="W-S" else 256
        qs=[]
        for b in blocks:
            m=Q.search(b); qs.append(int(m.group(1)) if m else None)
        classes=["SCI" if "e" in t.lower() else "PLAIN" for t in totals]
        old=re.findall(r"(?m)^Total time:\s*([0-9]+(?:\.[0-9]+)?)\s+us\.\s*$",txt)
        if len(blocks)!=32:add("HEADERS",f"{p.name}:{len(blocks)}")
        if len(totals)!=32:add("TOTALS",f"{p.name}:{len(totals)}")
        if qs[:1]!=[pre] or len(qs)!=32 or any(x!=1 for x in qs[1:]):add("Q_SHAPE",f"{p.name}:{qs}")
        if classes[:1]!=["SCI"] or classes[1:]!=["PLAIN"]*31:add("LEXICAL_CLASS",f"{p.name}:{classes}")
        if len(old)!=31:add("OLD_PARSER_REPRO",f"{p.name}:{len(old)}")
        rows.append({"file":p.name,"headers":len(blocks),"totals":len(totals),"prefill_qn":qs[0],"decode_qn_unique":sorted(set(qs[1:])),"first_total_class":classes[0],"remaining_total_class_unique":sorted(set(classes[1:])),"old_parser_records":len(old)})
    parser=(ROOT/"tools"/"parse_core0d_llama_vk_perf.py").read_text(encoding="utf-8")
    if "[0-9]+(?:\\.[0-9]+)?" not in parser or "(?:[eE]" in parser:add("PARSER_GRAMMAR","executed parser contract drift")
    src=(llama/"ggml"/"src"/"ggml-vulkan"/"ggml-vulkan.cpp").read_text(encoding="utf-8")
    if 'std::cerr << "Total time: " << total_all_op_times / 1000.0 << " us."' not in src:add("LOGGER_PRINT","default numeric stream line absent")
    if "ctx->perf_logger->print_timings();" not in src:add("LOGGER_CALLSITE","print_timings call absent")
    verdict="PASS_CORE0D_LLAMA_VKPERF_STRUCTURE_AUDIT_INDEPENDENT" if not f else "STOP_CORE0D_LLAMA_VKPERF_STRUCTURE_AUDIT_INDEPENDENT"
    result={"schema":"arcllm.core0d.llama_vkperf_structure_audit_independent.v0.1","verdict":verdict,"open_findings":len(f),"findings":f,"llama_commit":lh,"rows":rows,"independent_conclusion":"RAW_SURFACE_IS_1_PREFILL_PLUS_31_DECODE; EXECUTED_DECIMAL_ONLY_PARSER_DROPPED_PREFILL_SCIENTIFIC_NOTATION","timing_values_used":False,"recommended_governance":"FORMAL_CLOSE_CURRENT_CORE0D_STOP_AND_OPEN_PROSPECTIVE_NUMERIC_GRAMMAR_REPAIR_SUCCESSOR"}
    out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(f"CORE0D_VKPERF_INDEPENDENT={verdict}");print(f"OPEN_FINDINGS={len(f)}")
    return 0 if not f else 3
if __name__=="__main__":raise SystemExit(main())
