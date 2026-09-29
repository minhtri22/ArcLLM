#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARENT_R1_PREREG_HEAD = "a5a97ef29547ea076f5fb654764c98e657cdec22"
OLD_DATASET = ROOT / "results" / "core0d_measured_20260929T095154508631Z"
OLD_DATASET_COMMIT = "bce0d1c84afd953264d1efb35ee93d575e34dd49"
LLAMA_HEAD = "b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
TOKEN_XRAY_HEAD = "35f86ac68f98ffe60fc441a790274cd1f1269dfe"

EXPECTED_UNCHANGED_BLOBS = {
    "baseline/core0d_llama_teacher_forced_adapter.cpp": "acffb1db5d6cff3be6a863dfc67ab4018edd083b",
    "baseline/CMakeLists.txt": "4b8628ad83055479ee56de509a08965a5d40addf",
    "src/core0d_arcllm_teacher_forced_cli.cpp": "d2bb7b7f4475b52523c849cb4fb1a1355a9b788e",
    "tools/materialize_core0d_runtime.py": "62d0040bd5205e64988b92ed36731a21333b0b7e",
    "tools/run_core0d_measured.py": "9566cf90da18069ddc8259ece16261b5ca6b5cac",
    "src/core0c_token_xray_bridge.h": "f089f055bbf55f20e85810ec217471b93b36ffaf",
    "tools/materialize_core0c_trace_runtime.py": "fd794eae998ac00067f38153fe5f552f622275c0",
    "src/arcllm_v1_runtime.cpp": "0c613f6f740931a88ddd3ee5b904533a58001a06",
    "src/arcllm_v1_runtime_cli.cpp": "5526658f6469301f0e05ffa5a46f51bc9b511816",
    "src/arcllm_v1_vulkan_runtime_support.h": "1b3a2a935134a3afa680ee4880377a20bc8a466f",
    "src/arcllm_v1_q4_vulkan_backend_v4_runtime.h": "3955d1ed27c0c48f5bbb8e4564128bf34025d1a5",
    "include/arcllm/v1/runtime.h": "d7821270ed253e1d1d96e3916b22b3daee50f5bc",
    "config/arcllm_v1_runtime_active_v0.2.json": "8e60d8b60ad5750e1c3bdbd472fdd15f0d4bcfb4",
}
EXPECTED_EXE_SHA256 = {
    "arc_control_phase": "72FC72A5E217A694A47A6FC7B866B9DC50904418317FD54CC2E3FFA5C2D8ABD2",
    "arc_trace": "7DF37ED84E64AD87CF13B5B8D9F65CF3A9CAB9E8239CDE41E2A145EB358F0AFE",
    "llama": "A9BA63AB72E84AE6F4442F0CC2D41700838178A962D4F4CAC632AB2C1629204C",
}
NEW_GRAMMAR = r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"


def git(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def blob(path: str) -> str:
    return git("rev-parse", f"HEAD:{path}")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def load_parser():
    path = ROOT / "tools" / "parse_core0d_llama_vk_perf.py"
    spec = importlib.util.spec_from_file_location("core0d_r1_parser", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load CORE0D-R1 parser")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--llama-root", required=True)
    ap.add_argument("--token-xray-root", required=True)
    ap.add_argument("--artifact-dir", default=str(ROOT / "artifacts" / "core0d"))
    ap.add_argument("--out", default=str(ROOT / "results" / "CORE0D_R1_PREFLIGHT_EVIDENCE.json"))
    args = ap.parse_args()

    llama = Path(args.llama_root).resolve()
    tx = Path(args.token_xray_root).resolve()
    artifact_dir = Path(args.artifact_dir).resolve()
    out = Path(args.out).resolve()
    findings: list[dict[str, str]] = []

    def add(fid: str, detail: str) -> None:
        findings.append({"id": fid, "detail": detail})

    prereg = json.loads(
        (ROOT / "config" / "core0d_r1_llama_vkperf_numeric_grammar_repair_preregistration_v0.1.json")
        .read_text(encoding="utf-8")
    )
    if prereg.get("status") != "PREREGISTERED_NOT_AUTHORIZED_FOR_MEASURED_EXECUTION":
        add("PREREG_STATUS", repr(prereg.get("status")))
    auth = prereg.get("authorization", {})
    if auth.get("measured_execution") is not False or auth.get("measured_requests_authorized") != 0:
        add("PREREG_MEASURED_AUTH", repr(auth))

    old_auth = json.loads((ROOT / "config" / "core0d_science_authorization.json").read_text(encoding="utf-8"))
    if old_auth.get("authorized") is not False or old_auth.get("consumed") is not True:
        add("PARENT_AUTH_NOT_CONSUMED", repr((old_auth.get("authorized"), old_auth.get("consumed"))))

    for path, expected in EXPECTED_UNCHANGED_BLOBS.items():
        got = blob(path)
        if got != expected:
            add("UNCHANGED_BLOB_DRIFT", f"{path}:{got}!={expected}")

    parser_path = ROOT / "tools" / "parse_core0d_llama_vk_perf.py"
    parser_blob = blob("tools/parse_core0d_llama_vk_perf.py")
    parser_src = parser_path.read_text(encoding="utf-8")
    if NEW_GRAMMAR not in parser_src:
        add("NEW_GRAMMAR_ABSENT", NEW_GRAMMAR)
    if "inspect_structure" not in parser_src:
        add("STRUCTURAL_VALIDATOR_ABSENT", "inspect_structure")
    if "math.isfinite" not in parser_src:
        add("FINITE_GUARD_ABSENT", "math.isfinite")

    compile_cp = subprocess.run(
        ["py", "-3", "-m", "py_compile", str(parser_path)],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if compile_cp.returncode != 0:
        add("PARSER_COMPILE", compile_cp.stderr[-2000:])

    self_cp = subprocess.run(
        ["py", "-3", str(parser_path), "--self-test"],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if self_cp.returncode != 0 or "CORE0D_R1_LLAMA_VK_PERF_PARSER_SELF_TEST=PASS" not in self_cp.stdout:
        add("PARSER_SELF_TEST", (self_cp.stdout + self_cp.stderr)[-2500:])

    runner_cp = subprocess.run(
        ["py", "-3", str(ROOT / "tools" / "run_core0d_measured.py"), "--fixture-self-test"],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if runner_cp.returncode != 0 or "CORE0D_MEASURED_RUNNER_FIXTURE_SELF_TEST=PASS" not in runner_cp.stdout:
        add("RUNNER_FIXTURE_SELF_TEST", (runner_cp.stdout + runner_cp.stderr)[-2500:])

    parser = load_parser()
    structural_rows = []
    logs = sorted(OLD_DATASET.glob("*_GPU_TRACE_llama_vkperf.txt"))
    if len(logs) != 6:
        add("OLD_STRUCTURE_LOG_COUNT", str(len(logs)))
    for p in logs:
        try:
            s = parser.inspect_structure(p.read_text(encoding="utf-8", errors="replace"))
        except Exception as exc:
            add("OLD_STRUCTURE_REGRESSION", f"{p.name}:{exc}")
            continue
        workload = "W-S" if p.name.startswith("W_S_") else "W-C"
        expected_prefill = 4 if workload == "W-S" else 256
        if s.get("group_count") != 32:
            add("OLD_STRUCTURE_GROUP_COUNT", f"{p.name}:{s.get('group_count')}")
        if s.get("prefill_q_sequence_length") != expected_prefill:
            add("OLD_STRUCTURE_PREFILL_SHAPE", f"{p.name}:{s.get('prefill_q_sequence_length')}")
        if s.get("decode_q_sequence_lengths_unique") != [1]:
            add("OLD_STRUCTURE_DECODE_SHAPE", f"{p.name}:{s.get('decode_q_sequence_lengths_unique')}")
        if s.get("timing_magnitudes_returned") is not False:
            add("OLD_STRUCTURE_TIMING_LEAK", p.name)
        structural_rows.append({
            "file": p.name,
            "workload": workload,
            "group_count": s.get("group_count"),
            "prefill_q_sequence_length": s.get("prefill_q_sequence_length"),
            "decode_q_sequence_lengths_unique": s.get("decode_q_sequence_lengths_unique"),
            "numeric_lexical_classes": s.get("numeric_lexical_classes"),
            "timing_magnitudes_returned": s.get("timing_magnitudes_returned"),
        })

    llama_head = subprocess.check_output(["git", "-C", str(llama), "rev-parse", "HEAD"], text=True).strip()
    tx_head = subprocess.check_output(["git", "-C", str(tx), "rev-parse", "HEAD"], text=True).strip()
    if llama_head != LLAMA_HEAD:
        add("LLAMA_HEAD", llama_head)
    if tx_head != TOKEN_XRAY_HEAD:
        add("TOKEN_XRAY_HEAD", tx_head)
    if subprocess.check_output(["git", "-C", str(llama), "status", "--porcelain"], text=True).strip():
        add("LLAMA_DIRTY", str(llama))
    if subprocess.check_output(["git", "-C", str(tx), "status", "--porcelain"], text=True).strip():
        add("TOKEN_XRAY_DIRTY", str(tx))

    exe_paths = {
        "arc_control_phase": artifact_dir / "core0d_arcllm_control_phase.exe",
        "arc_trace": artifact_dir / "core0d_arcllm_trace.exe",
        "llama": artifact_dir / "core0d_llama_teacher_forced_adapter.exe",
    }
    exe_hashes = {}
    for key, path in exe_paths.items():
        if not path.exists():
            add("EXE_MISSING", str(path))
            continue
        got = sha256(path)
        exe_hashes[key] = got
        if got != EXPECTED_EXE_SHA256[key]:
            add("EXE_DRIFT", f"{key}:{got}!={EXPECTED_EXE_SHA256[key]}")

    r1_dirs = sorted(p.name for p in (ROOT / "results").glob("core0d_r1_measured_*") if p.is_dir())
    if r1_dirs:
        add("R1_MEASURED_ALREADY_EXISTS", repr(r1_dirs))

    verdict = "PASS_CORE0D_R1_PARSER_REPAIR_ZERO_SCIENCE_PREFLIGHT" if not findings else "STOP_CORE0D_R1_PARSER_REPAIR_PREFLIGHT"
    result = {
        "schema": "arcllm.core0d_r1.preflight_evidence.v0.1",
        "classification": "ZERO_SCIENCE_IMPLEMENTATION_STATIC_PREFLIGHT",
        "verdict": verdict,
        "open_findings": len(findings),
        "findings": findings,
        "checked_head": git("rev-parse", "HEAD"),
        "parent_r1_prereg_head": PARENT_R1_PREREG_HEAD,
        "parent_failed_dataset_commit": OLD_DATASET_COMMIT,
        "parser_blob": parser_blob,
        "parser_sha256": sha256(parser_path),
        "numeric_grammar": NEW_GRAMMAR,
        "parser_compile_pass": compile_cp.returncode == 0,
        "parser_fixture_self_test_pass": self_cp.returncode == 0,
        "runner_fixture_self_test_pass": runner_cp.returncode == 0,
        "old_failed_dataset_use": "STRUCTURE_ONLY_REGRESSION",
        "old_structure_rows": structural_rows,
        "timing_magnitudes_used_from_old_dataset": False,
        "llama_head": llama_head,
        "token_xray_head": tx_head,
        "executable_sha256": exe_hashes,
        "unchanged_blob_count": len(EXPECTED_UNCHANGED_BLOBS),
        "model_inference_executed": False,
        "measured_requests_executed": 0,
        "fresh_r1_measured_dataset_count": len(r1_dirs),
        "canonical_runtime_modified": False,
        "kernel_shader_modified": False,
        "next_if_pass": "INDEPENDENT_PREFLIGHT_ADJUDICATION_THEN_EXECUTION_LOCK",
    }
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"CORE0D_R1_PREFLIGHT={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    print(f"PARSER_BLOB={parser_blob}")
    print(f"PARSER_SHA256={result['parser_sha256']}")
    print(f"OLD_STRUCTURE_LOGS={len(structural_rows)}")
    print("MODEL_INFERENCE_EXECUTED=false")
    print("MEASURED_REQUESTS_EXECUTED=0")
    for f in findings:
        print(f"FINDING {f['id']}: {f['detail']}")
    return 0 if not findings else 3


if __name__ == "__main__":
    raise SystemExit(main())
