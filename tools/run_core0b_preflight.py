#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARC_PARENT = "7a5672112dc22de15f0e9bb6445508fbb3099b15"
LLAMA_COMMIT = "b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
MODEL_SHA256 = "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
MODEL_BYTES = 4683074048

ACTIVE_SHADERS = [
    "p8c_embedding_q4k_segmented_probe.spv",
    "p7_rmsnorm_seq.spv",
    "p7c_ffn_q4k_tiled.spv",
    "p7c_ffn_q6k_tiled.spv",
    "p7_rope_seq.spv",
    "p7_kv_store.spv",
    "p7_attention_prefill_online.spv",
    "p7_attention_kv_online.spv",
    "p7_add.spv",
    "p7l_ffn_q4k_gateup_fused.spv",
    "p7_swiglu.spv",
    "p7g_ffn_q4k_tiled16.spv",
    "p7g_ffn_q6k_tiled16.spv",
    "p7_q4k_gemm_2d.spv",
    "p7_q6k_gemm_2d.spv",
    "p8q1_lmhead_q6k_segmented_chunk.spv",
    "sa1_q4k_subgroup_splitk.spv",
    "q4_down_exec148_serial.spv",
    "b1_2_exec148_gpu_materialize.comp.spv",
]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def git(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def tokens_for(workload: str) -> list[int]:
    if workload == "W-S":
        return [1, 133151, 133152, 152062]
    if workload == "W-C":
        return [1, 133151, 133152, 152062] + [
            1 + ((104729 + 7919 * i) % 152063) for i in range(4, 256)
        ]
    raise ValueError(workload)


def validate_arc(d: dict) -> list[str]:
    e: list[str] = []
    if len(d.get("generated_token_ids", [])) != 32:
        e.append("generated_count")
    s = d.get("stats", {})
    expected = {
        "prefill_dispatches": 441,
        "prefill_submits": 1,
        "decode_dispatches_per_step": 469,
        "decode_submits_per_step": 1,
        "decode_steps": 31,
        "route_a_steps": 0,
        "route_b_steps": 31,
        "acquire_events": 1,
        "evict_events": 1,
        "b_allocations": 1,
        "b_materializations": 1,
        "b_validations": 1,
        "b_releases": 1,
        "p1_calls": 1,
        "p3_calls": 0,
        "p0_calls": 0,
    }
    for k, v in expected.items():
        if s.get(k) != v:
            e.append(f"{k}={s.get(k)!r}")
    if s.get("finite") is not True:
        e.append("finite")
    return e


def validate_llama(d: dict, qualify_only: bool) -> list[str]:
    e: list[str] = []
    if d.get("baseline_commit") != LLAMA_COMMIT:
        e.append("baseline_commit")
    if d.get("success") is not True:
        e.append("success")
    if d.get("runtime", {}).get("full_offload") is not True:
        e.append("full_offload")
    if d.get("runtime", {}).get("vulkan_log_present") is not True:
        e.append("vulkan")
    r = d.get("resolved", {})
    for k, v in {
        "n_ctx": 4096,
        "n_batch": 256,
        "n_ubatch": 256,
        "threads": 8,
        "threads_batch": 8,
        "kv_k": "F32",
        "kv_v": "F32",
        "n_gpu_layers_requested": -1,
    }.items():
        if r.get(k) != v:
            e.append(f"{k}={r.get(k)!r}")
    if not qualify_only:
        if d.get("final_logits_finite") is not True:
            e.append("finite")
        if len(d.get("generated_token_ids", [])) != 32:
            e.append("generated_count")
    return e


def run(cmd: list[str], out: Path) -> tuple[int, dict]:
    cp = subprocess.run(cmd, cwd=ROOT)
    data = load(out) if out.exists() else {}
    return cp.returncode, data


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", required=True)
    ap.add_argument("--out", default=str(ROOT / "results" / "CORE0B_PREFLIGHT_EVIDENCE.json"))
    args = ap.parse_args()

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    model = Path(args.model).resolve()
    arc = (ROOT / "arcllm_v1_runtime.exe").resolve()
    llama = (ROOT / "artifacts" / "core0b_baseline" / "core0b_llama_cold_adapter.exe").resolve()
    build_arc = ROOT / "results" / "ARCLLM_V1_CANONICAL_RUNTIME_EXTRACTION_BUILD.json"
    build_llama = ROOT / "results" / "CORE0B_LLAMA_BUILD_QUALIFICATION.json"

    findings: list[str] = []
    head = git("rev-parse", "HEAD")
    if subprocess.run(["git", "-C", str(ROOT), "merge-base", "--is-ancestor", ARC_PARENT, head]).returncode != 0:
        findings.append("canonical_parent_not_ancestor")

    for p in [model, arc, llama, build_arc, build_llama]:
        if not p.exists():
            findings.append(f"missing:{p}")

    if findings:
        result = {
            "schema": "arcllm.core0b.preflight_evidence.v0.1",
            "status": "STOP_CORE0B_PREFLIGHT",
            "findings": findings,
            "science_execution": False,
            "primary_timing_retained": False,
        }
        out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print("CORE0B_PREFLIGHT=STOP")
        return 3

    if model.stat().st_size != MODEL_BYTES or sha256(model) != MODEL_SHA256:
        findings.append("model_identity")

    lb = load(build_llama)
    if lb.get("status") != "PASS_BUILD_EXACT_LLAMA_BASELINE":
        findings.append("llama_build_status")
    if lb.get("commit") != LLAMA_COMMIT or lb.get("source_head") != LLAMA_COMMIT:
        findings.append("llama_build_commit")

    qual_dir = ROOT / "results" / "core0b_preflight"
    qual_dir.mkdir(parents=True, exist_ok=True)

    arc_results: dict[str, dict] = {}
    llama_results: dict[str, dict] = {}
    for workload, profile in [("W-S", "0"), ("W-C", "1")]:
        tag = workload.replace("-", "_")

        arc_out = qual_dir / f"arc_{tag}_functional.json"
        arc_cmd = [
            str(arc),
            "--model", str(model),
            "--shader-dir", str(ROOT / "compiled_shaders"),
            "--tokens", ",".join(map(str, tokens_for(workload))),
            "--max-new", "32",
            "--profile", profile,
            "--within-validated-domain", "1",
            "--out", str(arc_out),
        ]
        rc, d = run(arc_cmd, arc_out)
        errs = validate_arc(d)
        if rc != 0:
            errs.append(f"returncode={rc}")
        arc_results[workload] = {"path": str(arc_out.relative_to(ROOT)), "errors": errs}
        findings += [f"arc_{tag}:{x}" for x in errs]

        q_out = qual_dir / f"llama_{tag}_offload_qualification.json"
        q_cmd = [
            str(llama), "--model", str(model), "--workload", workload,
            "--qualify-only", "--out", str(q_out),
        ]
        rc, q = run(q_cmd, q_out)
        errs = validate_llama(q, True)
        if rc != 0:
            errs.append(f"returncode={rc}")
        findings += [f"llama_{tag}_qual:{x}" for x in errs]

        f_out = qual_dir / f"llama_{tag}_functional.json"
        f_cmd = [
            str(llama), "--model", str(model), "--workload", workload,
            "--out", str(f_out),
        ]
        rc, d = run(f_cmd, f_out)
        ferrs = validate_llama(d, False)
        if rc != 0:
            ferrs.append(f"returncode={rc}")
        findings += [f"llama_{tag}_functional:{x}" for x in ferrs]
        llama_results[workload] = {
            "qualification_path": str(q_out.relative_to(ROOT)),
            "qualification_errors": errs,
            "functional_path": str(f_out.relative_to(ROOT)),
            "functional_errors": ferrs,
        }

    shader_hashes: dict[str, str] = {}
    for name in ACTIVE_SHADERS:
        p = ROOT / "compiled_shaders" / name
        if not p.exists():
            findings.append(f"missing_shader:{name}")
        else:
            shader_hashes[name] = sha256(p)

    # Fail-closed proof: the primary runner must refuse to execute without authorization.
    missing_auth = qual_dir / "INTENTIONALLY_ABSENT_AUTH.json"
    if missing_auth.exists():
        missing_auth.unlink()
    guard = subprocess.run(
        [
            sys.executable,
            str(ROOT / "tools" / "run_core0b_primary.py"),
            "--model", str(model),
            "--authorization", str(missing_auth),
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    guard_text = (guard.stdout or "") + (guard.stderr or "")
    guard_pass = guard.returncode != 0 and "authorization artifact is absent" in guard_text
    if not guard_pass:
        findings.append("primary_runner_fail_closed_guard")

    critical_paths = [
        "baseline/core0b_llama_cold_adapter.cpp",
        "baseline/CMakeLists.txt",
        "tools/build_core0b_llama_baseline.ps1",
        "tools/run_core0b_primary.py",
        "tools/run_core0b_preflight.py",
        "src/arcllm_v1_runtime.cpp",
        "src/arcllm_v1_runtime_cli.cpp",
        "src/arcllm_v1_q4_vulkan_backend_v4_runtime.h",
        "include/arcllm/v1/runtime.h",
        "config/arcllm_v1_runtime_active_v0.2.json",
    ]
    blobs = {p: git("rev-parse", f"HEAD:{p}") for p in critical_paths}

    result = {
        "schema": "arcllm.core0b.preflight_evidence.v0.1",
        "status": "PASS_CORE0B_PREFLIGHT_EVIDENCE" if not findings else "STOP_CORE0B_PREFLIGHT",
        "classification": "ZERO_SCIENCE_FUNCTIONAL_PREFLIGHT",
        "implementation_head": head,
        "canonical_parent": ARC_PARENT,
        "target_model": {
            "path": str(model),
            "sha256": sha256(model),
            "bytes": model.stat().st_size,
        },
        "executables": {
            "arcllm": {"path": str(arc), "sha256": sha256(arc)},
            "llama": {"path": str(llama), "sha256": sha256(llama)},
        },
        "llama_build": lb,
        "arc_functional": arc_results,
        "llama_functional": llama_results,
        "active_shader_sha256": shader_hashes,
        "critical_git_blobs": blobs,
        "primary_runner_fail_closed_without_authorization": guard_pass,
        "primary_timing_retained": False,
        "token_xray_used": False,
        "hardware_counters_used": False,
        "profiler_used": False,
        "science_execution": False,
        "primary_requests_executed": 0,
        "mechanism_selection": False,
        "findings": findings,
        "open_findings": len(findings),
    }
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"CORE0B_PREFLIGHT={result['status']}")
    print(f"OPEN_FINDINGS={len(findings)}")
    return 0 if not findings else 3


if __name__ == "__main__":
    raise SystemExit(main())
