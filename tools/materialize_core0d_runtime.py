#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CORE0C_PATH = ROOT / "tools" / "materialize_core0c_trace_runtime.py"
CANONICAL_RUNTIME = ROOT / "src" / "arcllm_v1_runtime.cpp"
EXPECTED_RUNTIME_BLOB = "0c613f6f740931a88ddd3ee5b904533a58001a06"


def load_core0c():
    spec = importlib.util.spec_from_file_location("core0c_materializer", CORE0C_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load CORE0C materializer")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def git_blob(path: Path) -> str:
    return subprocess.check_output(
        ["git", "-C", str(ROOT), "rev-parse", f"HEAD:{path.relative_to(ROOT).as_posix()}"],
        text=True,
    ).strip()


def sha256_text(s: str) -> str:
    return hashlib.sha256(s.encode("utf-8")).hexdigest().upper()


def one_replace(text: str, old: str, new: str, label: str) -> str:
    n = text.count(old)
    if n != 1:
        raise RuntimeError(f"{label}: expected 1 anchor, found {n}")
    return text.replace(old, new, 1)


HELPERS = r'''
static std::vector<uint32_t> core0d_forced_decode_ids(){
    const char* raw=std::getenv("ARCLLM_CORE0D_FORCED_DECODE_IDS");
    if(!raw||!*raw)throw std::runtime_error("CORE0D forced decode ids env missing");
    std::vector<uint32_t> out;std::stringstream ss(raw);std::string item;
    while(std::getline(ss,item,',')){
        if(item.empty())throw std::runtime_error("CORE0D forced decode ids contains empty item");
        unsigned long v=std::stoul(item);
        if(v>=152064ul)throw std::runtime_error("CORE0D forced decode token out of vocabulary");
        out.push_back(uint32_t(v));
    }
    if(out.size()!=31u)throw std::runtime_error("CORE0D requires exactly 31 forced decode ids");
    return out;
}
static bool core0d_phase_enabled(){
    const char* p=std::getenv("ARCLLM_CORE0D_PHASE_OUT");return p&&*p;
}
static uint64_t core0d_ns(
    const std::chrono::steady_clock::time_point&a,
    const std::chrono::steady_clock::time_point&b){
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count());
}
'''


def patch_teacher_forcing(s: str) -> str:
    if "#include <cstdlib>" not in s:
        s = s.replace("#include <map>\n", "#include <map>\n#include <cstdlib>\n#include <sstream>\n", 1)
    s = one_replace(
        s,
        "arcllm::v1::runtime::RunResult arcllm::v1::runtime::generate(const RunRequest& request){\n",
        HELPERS + "\n" +
        "arcllm::v1::runtime::RunResult arcllm::v1::runtime::generate(const RunRequest& request){\n",
        "teacher helper insertion",
    )
    s = one_replace(
        s,
        '    if(request.max_new_tokens==0u)\n        throw std::runtime_error("ArcLLM runtime requires max_new_tokens >= 1");\n',
        '    if(request.max_new_tokens==0u)\n        throw std::runtime_error("ArcLLM runtime requires max_new_tokens >= 1");\n'
        '    if(request.max_new_tokens!=32u)throw std::runtime_error("CORE0D requires max_new_tokens=32");\n'
        '    const auto core0d_forced=core0d_forced_decode_ids();\n',
        "teacher request guard",
    )
    s = one_replace(
        s,
        "        uint32_t next=top.top1;\n        std::memcpy(b_dec_id.mapped,&next,sizeof(next));\n",
        "        uint32_t next=core0d_forced[0];\n"
        "        std::memcpy(b_dec_id.mapped,&next,sizeof(next));\n",
        "teacher initial input",
    )
    s = one_replace(
        s,
        "            next=top.top1;\n"
        "            result.generated_token_ids.push_back(next);\n"
        "            std::memcpy(b_dec_id.mapped,&next,sizeof(next));\n",
        "            const uint32_t predicted=top.top1;\n"
        "            result.generated_token_ids.push_back(predicted);\n"
        "            if(di+1u<core0d_forced.size()){next=core0d_forced[di+1u];std::memcpy(b_dec_id.mapped,&next,sizeof(next));}\n",
        "teacher loop input",
    )
    return s


def patch_phase(s: str) -> str:
    if "#include <chrono>" not in s:
        s = s.replace("#include <map>\n", "#include <map>\n#include <chrono>\n#include <fstream>\n", 1)
    s = one_replace(
        s,
        "arcllm::v1::runtime::RunResult arcllm::v1::runtime::generate(const RunRequest& request){\n"
        "    const std::string& model=request.model_path;\n",
        "arcllm::v1::runtime::RunResult arcllm::v1::runtime::generate(const RunRequest& request){\n"
        "    const bool core0d_phase=core0d_phase_enabled();\n"
        "    const auto core0d_t_generate0=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "    const std::string& model=request.model_path;\n",
        "phase generate start",
    )
    s = one_replace(
        s,
        "    const auto core0d_forced=core0d_forced_decode_ids();\n"
        "        GgufInfo gguf=GgufReader(model).read();\n",
        "    const auto core0d_forced=core0d_forced_decode_ids();\n"
        "        const auto core0d_t_model0=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        GgufInfo gguf=GgufReader(model).read();\n",
        "phase model start",
    )
    s = one_replace(
        s,
        '        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)\n'
        '            throw std::runtime_error("ArcLLM runtime TensorStore invariants failed");\n',
        '        if(!ts.mapped||!ts.all_bounds_valid||!ts.no_overlap||!ts.supported_types_only||!ts.q4_k_direct_access)\n'
        '            throw std::runtime_error("ArcLLM runtime TensorStore invariants failed");\n'
        '        const auto core0d_t_model1=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n',
        "phase model end",
    )
    s = one_replace(
        s,
        "        const uint8_t*payload=store.mapped_base()+gguf.data_offset;\n"
        "        VkRuntime vk;vk.init();\n",
        "        const auto core0d_t_contract1=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        const uint8_t*payload=store.mapped_base()+gguf.data_offset;\n"
        "        const auto core0d_t_vk0=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        VkRuntime vk;vk.init();\n",
        "phase contract/vk boundary",
    )
    s = one_replace(
        s,
        '        if(weight_requested!=RUNTIME_WEIGHT_BYTES)throw std::runtime_error("ArcLLM runtime weight residency bytes mismatch");\n',
        '        if(weight_requested!=RUNTIME_WEIGHT_BYTES)throw std::runtime_error("ArcLLM runtime weight residency bytes mismatch");\n'
        '        const auto core0d_t_vk1=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n',
        "phase vk end",
    )
    s = one_replace(
        s,
        "        ChainStats ps=vk.execute_prepared(ppchain,ppops,false);\n",
        "        const auto core0d_t_context1=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        const auto core0d_t_prefill0=core0d_t_context1;\n"
        "        ChainStats ps=vk.execute_prepared(ppchain,ppops,false);\n",
        "phase prefill start",
    )
    s = one_replace(
        s,
        '        if(!top.finite)throw std::runtime_error("ArcLLM runtime non-finite prefill logits");\n',
        '        if(!top.finite)throw std::runtime_error("ArcLLM runtime non-finite prefill logits");\n'
        '        const auto core0d_t_prefill1=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n',
        "phase prefill end",
    )
    s = one_replace(
        s,
        "        uint32_t next=core0d_forced[0];\n",
        "        const auto core0d_t_decode0=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        uint32_t next=core0d_forced[0];\n",
        "phase decode start",
    )
    s = one_replace(
        s,
        "        if(dchain_ready)vk.destroy_prepared(dchain);\n\n"
        "        const gp::PolicyDecision close_decision=q4_decide(0u);\n",
        "        const auto core0d_t_decode1=core0d_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        const auto core0d_t_close0=core0d_t_decode1;\n"
        "        if(dchain_ready)vk.destroy_prepared(dchain);\n\n"
        "        const gp::PolicyDecision close_decision=q4_decide(0u);\n",
        "phase decode end",
    )
    s = one_replace(
        s,
        "        for(auto&b:arenas)vk.destroy_buffer(b);\n"
        "        return result;\n",
        "        for(auto&b:arenas)vk.destroy_buffer(b);\n"
        "        if(core0d_phase){\n"
        "            const auto core0d_t_close1=std::chrono::steady_clock::now();\n"
        "            const char* phase_path=std::getenv(\"ARCLLM_CORE0D_PHASE_OUT\");\n"
        "            std::ofstream po(phase_path,std::ios::binary|std::ios::trunc);\n"
        "            if(!po)throw std::runtime_error(\"CORE0D cannot write phase artifact\");\n"
        "            po<<\"{\\n\";\n"
        "            po<<\"  \\\"schema\\\":\\\"arcllm.core0d.arc_phase.v0.1\\\",\\n\";\n"
        "            po<<\"  \\\"timing_authority\\\":\\\"DIAGNOSTIC_ONLY\\\",\\n\";\n"
        "            po<<\"  \\\"model_map_inspect_ns\\\":\"<<core0d_ns(core0d_t_model0,core0d_t_model1)<<\",\\n\";\n"
        "            po<<\"  \\\"static_graph_contract_ns\\\":\"<<core0d_ns(core0d_t_model1,core0d_t_contract1)<<\",\\n\";\n"
        "            po<<\"  \\\"vulkan_weight_init_ns\\\":\"<<core0d_ns(core0d_t_vk0,core0d_t_vk1)<<\",\\n\";\n"
        "            po<<\"  \\\"request_context_prepare_ns\\\":\"<<core0d_ns(core0d_t_vk1,core0d_t_context1)<<\",\\n\";\n"
        "            po<<\"  \\\"prefill_and_scan_ns\\\":\"<<core0d_ns(core0d_t_prefill0,core0d_t_prefill1)<<\",\\n\";\n"
        "            po<<\"  \\\"decode_loop_and_scans_ns\\\":\"<<core0d_ns(core0d_t_decode0,core0d_t_decode1)<<\",\\n\";\n"
        "            po<<\"  \\\"close_and_cleanup_ns\\\":\"<<core0d_ns(core0d_t_close0,core0d_t_close1)<<\",\\n\";\n"
        "            po<<\"  \\\"runtime_generate_ns\\\":\"<<core0d_ns(core0d_t_generate0,core0d_t_close1)<<\"\\n}\";\n"
        "        }\n"
        "        return result;\n",
        "phase artifact write",
    )
    return s


def main() -> None:
    ap=argparse.ArgumentParser()
    ap.add_argument("--out-dir",required=True)
    args=ap.parse_args()
    out=Path(args.out_dir).resolve()
    out.mkdir(parents=True,exist_ok=True)

    if git_blob(CANONICAL_RUNTIME)!=EXPECTED_RUNTIME_BLOB:
        raise SystemExit("CORE0D canonical runtime blob drift")

    c0=load_core0c()
    raw=CANONICAL_RUNTIME.read_text(encoding="utf-8")

    control=raw.replace('#include "../include/arcllm/v1/runtime.h"','#include "arcllm/v1/runtime.h"')
    control=patch_phase(patch_teacher_forcing(control))

    trace_support=c0.materialize_support(c0.CANONICAL["support"].read_text(encoding="utf-8"))
    trace_support=trace_support.replace('"core0c_token_xray_bridge.h"','"core0c_token_xray_bridge.h"')

    trace_q4=c0.materialize_q4(c0.CANONICAL["q4"].read_text(encoding="utf-8"))
    trace_q4=trace_q4.replace('"core0c_arcllm_v1_vulkan_runtime_support.h"','"core0d_arcllm_trace_vulkan_runtime_support.h"')

    trace=c0.materialize_runtime(raw)
    trace=trace.replace('"core0c_arcllm_v1_q4_vulkan_backend_v4_runtime.h"','"core0d_arcllm_trace_q4_vulkan_backend_v4_runtime.h"')
    trace=patch_teacher_forcing(trace)

    generated={
        "core0d_arcllm_control_phase_runtime.cpp":control,
        "core0d_arcllm_trace_runtime.cpp":trace,
        "core0d_arcllm_trace_vulkan_runtime_support.h":trace_support,
        "core0d_arcllm_trace_q4_vulkan_backend_v4_runtime.h":trace_q4,
    }
    for name,content in generated.items():
        (out/name).write_text(content,encoding="utf-8",newline="\n")

    manifest={
        "schema":"arcllm.core0d.generated_runtime.v0.1",
        "classification":"BENCHMARK_ONLY_DETERMINISTIC_DERIVATIVES",
        "canonical_runtime_blob":git_blob(CANONICAL_RUNTIME),
        "core0c_materializer_blob":git_blob(CORE0C_PATH),
        "generated_sha256":{k:sha256_text(v) for k,v in generated.items()},
        "control_phase_patch_scope":["teacher-forced decode inputs","optional monotonic host phase timestamps"],
        "trace_patch_scope":["teacher-forced decode inputs","frozen CORE0C Token-XRay Vulkan timestamps"],
        "canonical_runtime_modified":False,
        "kernel_source_modified":False,
    }
    (out/"CORE0D_GENERATED_RUNTIME_MANIFEST.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    print("CORE0D_MATERIALIZE=PASS")
    for k,v in manifest["generated_sha256"].items(): print(f"{k}={v}")


if __name__=="__main__":
    main()
