#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CORE0D_PATH = ROOT / "tools" / "materialize_core0d_runtime.py"
CANONICAL_RUNTIME = ROOT / "src" / "arcllm_v1_runtime.cpp"
EXPECTED_RUNTIME_BLOB = "0c613f6f740931a88ddd3ee5b904533a58001a06"


def load_core0d():
    spec = importlib.util.spec_from_file_location("core0d_materializer", CORE0D_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load CORE0D materializer")
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


PHASE_HELPERS = r'''
static bool core0e_phase_enabled(){
    const char* p=std::getenv("ARCLLM_CORE0E_PHASE_OUT");
    return p&&*p;
}
static uint64_t core0e_abs_ns(const std::chrono::steady_clock::time_point& t){
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count());
}
'''


def patch_phase_markers(s: str) -> str:
    if "#include <chrono>" not in s:
        s = s.replace("#include <map>\n", "#include <map>\n#include <chrono>\n#include <fstream>\n", 1)
    elif "#include <fstream>" not in s:
        s = s.replace("#include <chrono>\n", "#include <chrono>\n#include <fstream>\n", 1)

    s = one_replace(
        s,
        "arcllm::v1::runtime::RunResult arcllm::v1::runtime::generate(const RunRequest& request){\n",
        PHASE_HELPERS
        + "\n"
        + "arcllm::v1::runtime::RunResult arcllm::v1::runtime::generate(const RunRequest& request){\n"
        + "    const bool core0e_phase=core0e_phase_enabled();\n"
        + "    const auto core0e_child_start=core0e_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n",
        "child science window start",
    )

    s = one_replace(
        s,
        "        ChainStats ps=vk.execute_prepared(ppchain,ppops,false);\n",
        "        const auto core0e_prefill_start=core0e_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        ChainStats ps=vk.execute_prepared(ppchain,ppops,false);\n",
        "prefill wall start",
    )
    s = one_replace(
        s,
        '        if(!top.finite)throw std::runtime_error("ArcLLM runtime non-finite prefill logits");\n',
        '        if(!top.finite)throw std::runtime_error("ArcLLM runtime non-finite prefill logits");\n'
        "        const auto core0e_prefill_end=core0e_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n",
        "prefill wall end",
    )

    s = one_replace(
        s,
        "        uint32_t next=core0d_forced[0];\n",
        "        const auto core0e_decode_start=core0e_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        uint32_t next=core0d_forced[0];\n",
        "decode wall start",
    )
    s = one_replace(
        s,
        "        if(dchain_ready)vk.destroy_prepared(dchain);\n\n"
        "        const gp::PolicyDecision close_decision=q4_decide(0u);\n",
        "        const auto core0e_decode_end=core0e_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        if(dchain_ready)vk.destroy_prepared(dchain);\n\n"
        "        const gp::PolicyDecision close_decision=q4_decide(0u);\n",
        "decode wall end",
    )

    s = one_replace(
        s,
        "        for(auto&b:arenas)vk.destroy_buffer(b);\n"
        "        arcllm_core0c::lifecycle().flush();\n"
        "        return result;\n",
        "        for(auto&b:arenas)vk.destroy_buffer(b);\n"
        "        arcllm_core0c::lifecycle().flush();\n"
        "        const auto core0e_child_end=core0e_phase?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};\n"
        "        if(core0e_phase){\n"
        "            const char* phase_path=std::getenv(\"ARCLLM_CORE0E_PHASE_OUT\");\n"
        "            std::ofstream po(phase_path,std::ios::binary|std::ios::trunc);\n"
        "            if(!po)throw std::runtime_error(\"CORE0E cannot write phase sidecar\");\n"
        "            po<<\"{\\n\";\n"
        "            po<<\"  \\\"schema\\\":\\\"arcllm.core0e.phase_markers.v0.1\\\",\\n\";\n"
        "            po<<\"  \\\"timing_authority\\\":\\\"DIAGNOSTIC_ONLY\\\",\\n\";\n"
        "            po<<\"  \\\"clock\\\":\\\"std::chrono::steady_clock\\\",\\n\";\n"
        "            po<<\"  \\\"child_science_window_start_ns\\\":\"<<core0e_abs_ns(core0e_child_start)<<\",\\n\";\n"
        "            po<<\"  \\\"prefill_wall_start_ns\\\":\"<<core0e_abs_ns(core0e_prefill_start)<<\",\\n\";\n"
        "            po<<\"  \\\"prefill_wall_end_ns\\\":\"<<core0e_abs_ns(core0e_prefill_end)<<\",\\n\";\n"
        "            po<<\"  \\\"decode_wall_start_ns\\\":\"<<core0e_abs_ns(core0e_decode_start)<<\",\\n\";\n"
        "            po<<\"  \\\"decode_wall_end_ns\\\":\"<<core0e_abs_ns(core0e_decode_end)<<\",\\n\";\n"
        "            po<<\"  \\\"child_science_window_end_ns\\\":\"<<core0e_abs_ns(core0e_child_end)<<\"\\n}\\n\";\n"
        "        }\n"
        "        return result;\n",
        "child end and sidecar",
    )
    return s


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", required=True)
    args = ap.parse_args()
    out = Path(args.out_dir).resolve()
    out.mkdir(parents=True, exist_ok=True)

    if git_blob(CANONICAL_RUNTIME) != EXPECTED_RUNTIME_BLOB:
        raise SystemExit("CORE0E canonical runtime blob drift")

    c0d = load_core0d()
    c0c = c0d.load_core0c()
    raw = CANONICAL_RUNTIME.read_text(encoding="utf-8")

    trace_support = c0c.materialize_support(c0c.CANONICAL["support"].read_text(encoding="utf-8"))
    trace_q4 = c0c.materialize_q4(c0c.CANONICAL["q4"].read_text(encoding="utf-8"))
    trace_q4 = trace_q4.replace(
        '"core0c_arcllm_v1_vulkan_runtime_support.h"',
        '"core0e_arcllm_trace_vulkan_runtime_support.h"',
    )

    combined = c0c.materialize_runtime(raw)
    combined = combined.replace(
        '"core0c_arcllm_v1_q4_vulkan_backend_v4_runtime.h"',
        '"core0e_arcllm_trace_q4_vulkan_backend_v4_runtime.h"',
    )
    combined = c0d.patch_teacher_forcing(combined)
    combined = patch_phase_markers(combined)

    generated = {
        "core0e_arcllm_combined_runtime.cpp": combined,
        "core0e_arcllm_trace_vulkan_runtime_support.h": trace_support,
        "core0e_arcllm_trace_q4_vulkan_backend_v4_runtime.h": trace_q4,
    }
    for name, text in generated.items():
        (out / name).write_text(text, encoding="utf-8", newline="\n")

    manifest = {
        "schema": "arcllm.core0e.generated_runtime.v0.1",
        "classification": "BENCHMARK_ONLY_DIAGNOSTIC_DERIVATIVE",
        "canonical_runtime_blob": git_blob(CANONICAL_RUNTIME),
        "core0d_materializer_blob": git_blob(CORE0D_PATH),
        "generated_sha256": {k: sha256_text(v) for k, v in generated.items()},
        "patch_scope": [
            "frozen CORE0D-R1 teacher-forced decode inputs",
            "frozen CORE0C Token-XRay Vulkan timestamp tracing",
            "six CORE0E monotonic absolute phase markers",
            "phase-sidecar serialization only after child_science_window_end",
        ],
        "canonical_runtime_modified": False,
        "kernel_shader_source_modified": False,
    }
    (out / "CORE0E_GENERATED_RUNTIME_MANIFEST.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print("CORE0E_MATERIALIZE=PASS")
    for k, v in manifest["generated_sha256"].items():
        print(f"{k}={v}")


if __name__ == "__main__":
    main()
