from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path


ARC_PIN = "7a5672112dc22de15f0e9bb6445508fbb3099b15"
TX_PIN = "17baf9e9e561bdb5efe9904dd4cd678f9e19368d"
LLAMA_PIN = "b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
MODEL_SHA = "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"

DECODE_SHADER_COUNTS = {
    "p8c_embedding_q4k_segmented_probe.spv": 1,
    "p7_rmsnorm_seq.spv": 57,
    "p7_q4k_gemm_2d.spv": 98,
    "p7_q6k_gemm_2d.spv": 28,
    "p7_rope_seq.spv": 56,
    "p7_kv_store.spv": 28,
    "p7_attention_kv_online.spv": 28,
    "p7_add.spv": 56,
    "sa1_q4k_subgroup_splitk.spv": 56,
    "p7_swiglu.spv": 28,
    "q4_down_exec148_serial.spv": 14,
    "p8q1_lmhead_q6k_segmented_chunk.spv": 19,
}

PREFILL_SHADER_COUNTS = {
    "p8c_embedding_q4k_segmented_probe.spv": 1,
    "p7_rmsnorm_seq.spv": 57,
    "p7c_ffn_q4k_tiled.spv": 98,
    "p7c_ffn_q6k_tiled.spv": 14,
    "p7_rope_seq.spv": 56,
    "p7_kv_store.spv": 28,
    "p7_attention_prefill_online.spv": 28,
    "p7_add.spv": 56,
    "p7l_ffn_q4k_gateup_fused.spv": 28,
    "p7_swiglu.spv": 28,
    "p7g_ffn_q4k_tiled16.spv": 14,
    "p7g_ffn_q6k_tiled16.spv": 14,
    "p8q1_lmhead_q6k_segmented_chunk.spv": 19,
}

DECODE_SUFFIXES = {
    "attn_rmsnorm", "q_proj", "k_proj", "v_proj", "q_rope", "k_rope",
    "kv_store", "cached_gqa", "o_proj", "attn_residual", "ffn_rmsnorm",
    "ffn_gate", "ffn_up", "swiglu", "ffn_down", "ffn_residual",
}


def git_head(root: Path) -> str:
    return subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"], text=True
    ).strip()


def parse_python_local_keys(text: str) -> set[str]:
    return set(re.findall(r'"([A-Za-z0-9_.-]+\.spv)"\s*:', text))


def parse_cpp_shader_literals(text: str) -> set[str]:
    return set(re.findall(r'"([A-Za-z0-9_.-]+\.spv)"', text))


def parse_suffix_keys(text: str) -> set[str]:
    block = text.split("_SUFFIX = {", 1)[1].split("}", 1)[0]
    return set(re.findall(r'"([A-Za-z0-9_]+)"\s*:', block))


def run_process(cmd: list[str], cwd: Path) -> dict:
    cp = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True)
    return {
        "command": cmd,
        "returncode": cp.returncode,
        "stdout": cp.stdout[-4000:],
        "stderr": cp.stderr[-4000:],
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--token-xray-root", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    arc = Path(__file__).resolve().parents[1]
    tx = Path(args.token_xray_root).resolve()
    out = Path(args.out)

    arc_head = git_head(arc)
    tx_head = git_head(tx)

    arc_rt = (arc / "src/arcllm_v1_runtime.cpp").read_text(encoding="utf-8")
    arc_q4 = (arc / "src/arcllm_v1_q4_vulkan_backend_v4_runtime.h").read_text(encoding="utf-8")
    tx_py_path = tx / "src/token_xray/adapters/arcllm_v1.py"
    tx_cpp_path = tx / "sdk/vulkan/token_xray_arcllm_v1_adapter.h"
    tx_init_path = tx / "src/token_xray/adapters/__init__.py"
    tx_schema_path = tx / "schemas/token_trace.schema.json"
    tx_readme_path = tx / "README.md"
    tx_handoff_path = tx / "docs/ARCLLM_REVALIDATION_HANDOFF.md"

    tx_py = tx_py_path.read_text(encoding="utf-8")
    tx_cpp = tx_cpp_path.read_text(encoding="utf-8")
    tx_schema = tx_schema_path.read_text(encoding="utf-8")
    tx_readme = tx_readme_path.read_text(encoding="utf-8")
    tx_init = tx_init_path.read_text(encoding="utf-8")

    py_shaders = parse_python_local_keys(tx_py)
    cpp_shaders = parse_cpp_shader_literals(tx_cpp)
    common_adapter_shaders = py_shaders & cpp_shaders
    suffixes = parse_suffix_keys(tx_py)

    # Prove the current runtime sources actually contain the shader mechanisms being audited.
    runtime_literals = set(re.findall(r'"([A-Za-z0-9_.-]+\.spv)"', arc_rt + "\n" + arc_q4))
    required_runtime_literals = set(DECODE_SHADER_COUNTS) | set(PREFILL_SHADER_COUNTS) | {
        "b1_2_exec148_gpu_materialize.comp.spv"
    }
    runtime_source_presence = {
        s: s in runtime_literals for s in sorted(required_runtime_literals)
    }

    decode_unmapped = {
        s: n for s, n in DECODE_SHADER_COUNTS.items() if s not in common_adapter_shaders
    }
    decode_mapped = sum(
        n for s, n in DECODE_SHADER_COUNTS.items() if s in common_adapter_shaders
    )
    prefill_unmapped = {
        s: n for s, n in PREFILL_SHADER_COUNTS.items() if s not in common_adapter_shaders
    }
    prefill_mapped = sum(
        n for s, n in PREFILL_SHADER_COUNTS.items() if s in common_adapter_shaders
    )

    decode_suffix_missing = sorted(DECODE_SUFFIXES - suffixes)

    # Current adapter is explicitly decode-ID producing. Prefill has two additional mapping
    # requirements: causal_gqa and a fused gate+up dispatch mapping to two semantic nodes.
    prefill_semantic_issues = []
    if '"decode.' in tx_py:
        prefill_semantic_issues.append("adapter_semantic_ids_are_decode_prefixed")
    if "causal_gqa" not in suffixes:
        prefill_semantic_issues.append("causal_gqa_runtime_name_not_supported")
    if "ffn_gate_up_fused" not in suffixes:
        prefill_semantic_issues.append("ffn_gate_up_fused_runtime_name_not_supported")
    if "semantic_node_ids" not in tx_py:
        prefill_semantic_issues.append("adapter_api_has_no_multi_semantic_fused_mapping")

    lifecycle_shader = "b1_2_exec148_gpu_materialize.comp.spv"
    lifecycle = {
        "p1_materialize_dispatches_per_acquire": 14,
        "p1_shader": lifecycle_shader,
        "python_shader_mapping": lifecycle_shader in py_shaders,
        "cpp_shader_mapping": lifecycle_shader in cpp_shaders,
        "q4v4_runtime_name_shape": "Q4V4.P1.L<layer>",
        "q4v4_runtime_name_supported_by_semantic_adapter": "Q4V4.P1." in tx_py,
        "explicit_lifecycle_event_surface_detected": any(
            k in tx_schema for k in [
                "representation_acquire", "representation_materialize",
                "representation_validate", "representation_evict"
            ]
        ),
    }

    observer_fields = {
        k: k in tx_schema for k in [
            "measurement_mode", "timing_authority", "instrumentation_overhead"
        ]
    }

    compileall = run_process(["py", "-3", "-m", "compileall", "-q", "src"], tx)
    pytest = run_process(["py", "-3", "-m", "pytest", "-q"], tx)

    findings = []

    if arc_head != ARC_PIN:
        findings.append({"id": "PIN-ARC", "severity": "BLOCKER", "detail": f"ArcLLM HEAD drift: {arc_head}"})
    if tx_head != TX_PIN:
        findings.append({"id": "PIN-TX", "severity": "BLOCKER", "detail": f"Token-XRay HEAD drift: {tx_head}"})

    if compileall["returncode"] != 0 or pytest["returncode"] != 0:
        findings.append({
            "id": "TX-BASELINE-QA",
            "severity": "BLOCKER",
            "detail": "Pinned Token-XRay main cannot execute its Python QA baseline; adapters/__init__.py contains a literal \\n token and raises SyntaxError."
        })

    if decode_suffix_missing:
        findings.append({
            "id": "DECODE-SEMANTIC-MAP",
            "severity": "BLOCKER",
            "detail": f"Decode semantic suffixes missing: {decode_suffix_missing}"
        })

    if decode_unmapped:
        findings.append({
            "id": "DECODE-SHADER-DRIFT",
            "severity": "BLOCKER",
            "detail": f"{sum(decode_unmapped.values())}/469 current decode dispatches use shader(s) absent from both pinned adapters: {decode_unmapped}"
        })

    if prefill_unmapped or prefill_semantic_issues:
        findings.append({
            "id": "PREFILL-COVERAGE",
            "severity": "BLOCKER",
            "detail": f"Current prefill is not representable without loss/mislabeling; geometry unmapped={sum(prefill_unmapped.values())}/441, shaders={prefill_unmapped}, semantic_issues={prefill_semantic_issues}"
        })

    if not lifecycle["python_shader_mapping"] or not lifecycle["cpp_shader_mapping"] or not lifecycle["explicit_lifecycle_event_surface_detected"]:
        findings.append({
            "id": "RUNTIME-LIFECYCLE",
            "severity": "BLOCKER",
            "detail": "Q4V4 acquire/materialize/validate/resident/reuse/evict lifecycle is not represented as a separate evidence surface; P1 GPU materialization shader is not mapped."
        })

    if not all(observer_fields.values()):
        findings.append({
            "id": "OBSERVER-EFFECT",
            "severity": "REQUIRED_BEFORE_CORE0C",
            "detail": f"Token trace contract lacks explicit observer/timing-authority fields: {observer_fields}"
        })

    if "docs/ARCLLM_REVALIDATION_HANDOFF.md" in tx_readme and not tx_handoff_path.exists():
        findings.append({
            "id": "DOC-HANDOFF-DRIFT",
            "severity": "NORMAL",
            "detail": "README references docs/ARCLLM_REVALIDATION_HANDOFF.md but the file is absent at the pinned Token-XRay commit."
        })

    source_presence_pass = all(runtime_source_presence.values())
    verdict = (
        "PASS_CORE0A_TOKEN_XRAY_COMPATIBLE"
        if not findings and source_presence_pass
        else "STOP_BEFORE_INSTRUMENTED_RUN_TOKEN_XRAY_ADAPTER_DRIFT"
    )

    result = {
        "schema": "arcllm.core0a.token_xray_compatibility.v0.1",
        "classification": "ZERO_SCIENCE_COMPATIBILITY_PREFLIGHT",
        "verdict": verdict,
        "science_measured_inferences": 0,
        "performance_measurement_authorized": False,
        "mechanism_selection_authorized": False,
        "pins": {
            "arcllm_canonical_parent": ARC_PIN,
            "arcllm_audit_checkout_head": arc_head,
            "token_xray": TX_PIN,
            "token_xray_audit_checkout_head": tx_head,
            "llama_cpp": LLAMA_PIN,
            "model_sha256": MODEL_SHA,
        },
        "runtime_source_presence": {
            "pass": source_presence_pass,
            "shaders": runtime_source_presence,
        },
        "token_xray_baseline_qa": {
            "adapter_init_raw": tx_init,
            "compileall": compileall,
            "pytest": pytest,
        },
        "decode": {
            "expected_dispatches": 469,
            "expected_semantic_nodes": 451,
            "semantic_suffix_mapping_pass": not decode_suffix_missing,
            "missing_semantic_suffixes": decode_suffix_missing,
            "geometry_mapped_dispatches": decode_mapped,
            "geometry_unmapped_dispatches": sum(decode_unmapped.values()),
            "unmapped_shader_dispatch_counts": decode_unmapped,
        },
        "prefill": {
            "expected_dispatches": 441,
            "geometry_mapped_dispatches": prefill_mapped,
            "geometry_unmapped_dispatches": sum(prefill_unmapped.values()),
            "unmapped_shader_dispatch_counts": prefill_unmapped,
            "semantic_issues": prefill_semantic_issues,
        },
        "runtime_lifecycle": lifecycle,
        "observer_contract": observer_fields,
        "documentation": {
            "readme_references_arcllm_revalidation_handoff": "docs/ARCLLM_REVALIDATION_HANDOFF.md" in tx_readme,
            "referenced_handoff_exists": tx_handoff_path.exists(),
        },
        "findings": findings,
        "open_finding_count": len(findings),
    }

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"CORE0A_VERDICT={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    print(f"DECODE_GEOMETRY={decode_mapped}/469")
    print(f"PREFILL_GEOMETRY={prefill_mapped}/441")
    for f in findings:
        print(f"FINDING {f['id']} [{f['severity']}] {f['detail']}")


if __name__ == "__main__":
    main()
