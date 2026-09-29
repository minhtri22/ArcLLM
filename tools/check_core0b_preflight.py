#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARC_PARENT = "7a5672112dc22de15f0e9bb6445508fbb3099b15"
LLAMA_COMMIT = "b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
MODEL_SHA256 = "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
MODEL_BYTES = 4683074048


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def git(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def add(findings: list[dict], fid: str, detail: str) -> None:
    findings.append({"id": fid, "detail": detail})


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--evidence",
        default=str(ROOT / "results" / "CORE0B_PREFLIGHT_EVIDENCE.json"),
    )
    ap.add_argument(
        "--out",
        default=str(ROOT / "results" / "CORE0B_PREFLIGHT_INDEPENDENT_ADJUDICATION.json"),
    )
    args = ap.parse_args()

    ev_path = Path(args.evidence)
    out = Path(args.out)
    findings: list[dict] = []

    if not ev_path.exists():
        add(findings, "EVIDENCE_MISSING", str(ev_path))
        ev = {}
    else:
        ev = load(ev_path)

    if ev.get("schema") != "arcllm.core0b.preflight_evidence.v0.1":
        add(findings, "SCHEMA", f"got={ev.get('schema')!r}")
    if ev.get("status") != "PASS_CORE0B_PREFLIGHT_EVIDENCE":
        add(findings, "UPSTREAM_STATUS", f"got={ev.get('status')!r}")
    if ev.get("classification") != "ZERO_SCIENCE_FUNCTIONAL_PREFLIGHT":
        add(findings, "CLASSIFICATION", f"got={ev.get('classification')!r}")
    if ev.get("science_execution") is not False or ev.get("primary_requests_executed") != 0:
        add(findings, "SCIENCE_BOUNDARY", "preflight consumed primary science")
    if ev.get("primary_timing_retained") is not False:
        add(findings, "TIMING_BOUNDARY", "preflight retained primary timing")
    if any(ev.get(k) is not False for k in ["token_xray_used", "hardware_counters_used", "profiler_used"]):
        add(findings, "INSTRUMENTATION_BOUNDARY", "instrumentation flag present")
    if ev.get("primary_runner_fail_closed_without_authorization") is not True:
        add(findings, "AUTH_GUARD", "primary runner did not prove fail-closed")

    head = git("rev-parse", "HEAD")
    impl = ev.get("implementation_head")
    if not isinstance(impl, str):
        add(findings, "IMPLEMENTATION_HEAD", "missing")
    else:
        rc = subprocess.run(
            ["git", "-C", str(ROOT), "merge-base", "--is-ancestor", impl, head]
        ).returncode
        if rc != 0:
            add(findings, "IMPLEMENTATION_ANCESTRY", f"{impl} !<= {head}")
    if subprocess.run(
        ["git", "-C", str(ROOT), "merge-base", "--is-ancestor", ARC_PARENT, head]
    ).returncode != 0:
        add(findings, "CANONICAL_PARENT", "ArcLLM canonical parent is not ancestor")

    model = Path(ev.get("target_model", {}).get("path", ""))
    if not model.exists():
        add(findings, "MODEL_PATH", str(model))
    else:
        if model.stat().st_size != MODEL_BYTES:
            add(findings, "MODEL_BYTES", str(model.stat().st_size))
        if sha256(model) != MODEL_SHA256:
            add(findings, "MODEL_SHA", sha256(model))

    exe_info = ev.get("executables", {})
    for arm, key in [("arcllm", "arcllm"), ("llama", "llama")]:
        info = exe_info.get(key, {})
        p = Path(info.get("path", ""))
        if not p.exists():
            add(findings, f"{arm.upper()}_EXE_MISSING", str(p))
        elif sha256(p) != info.get("sha256"):
            add(findings, f"{arm.upper()}_EXE_SHA", "recomputed hash mismatch")

    lb = ev.get("llama_build", {})
    if lb.get("status") != "PASS_BUILD_EXACT_LLAMA_BASELINE":
        add(findings, "LLAMA_BUILD_STATUS", str(lb.get("status")))
    if lb.get("commit") != LLAMA_COMMIT or lb.get("source_head") != LLAMA_COMMIT:
        add(findings, "LLAMA_COMMIT", f"{lb.get('commit')} / {lb.get('source_head')}")
    if lb.get("source_clean") is not True or lb.get("backend") != "Vulkan":
        add(findings, "LLAMA_BUILD_CONTRACT", "clean Vulkan exact build not proven")
    if lb.get("science_execution") is not False:
        add(findings, "LLAMA_BUILD_SCIENCE", "build artifact says science execution")

    for workload in ["W-S", "W-C"]:
        a = ev.get("arc_functional", {}).get(workload, {})
        apath = ROOT / a.get("path", "")
        if not apath.exists():
            add(findings, f"ARC_{workload}_MISSING", str(apath))
        else:
            d = load(apath)
            s = d.get("stats", {})
            checks = {
                "generated_count": len(d.get("generated_token_ids", [])) == 32,
                "prefill_dispatches": s.get("prefill_dispatches") == 441,
                "prefill_submits": s.get("prefill_submits") == 1,
                "decode_dispatches": s.get("decode_dispatches_per_step") == 469,
                "decode_submits": s.get("decode_submits_per_step") == 1,
                "decode_steps": s.get("decode_steps") == 31,
                "route_a": s.get("route_a_steps") == 0,
                "route_b": s.get("route_b_steps") == 31,
                "acquire": s.get("acquire_events") == 1,
                "evict": s.get("evict_events") == 1,
                "alloc": s.get("b_allocations") == 1,
                "mat": s.get("b_materializations") == 1,
                "validate": s.get("b_validations") == 1,
                "release": s.get("b_releases") == 1,
                "p1": s.get("p1_calls") == 1,
                "p3": s.get("p3_calls") == 0,
                "p0": s.get("p0_calls") == 0,
                "finite": s.get("finite") is True,
            }
            bad = [k for k, ok in checks.items() if not ok]
            if bad:
                add(findings, f"ARC_{workload}_INVARIANTS", ",".join(bad))

        l = ev.get("llama_functional", {}).get(workload, {})
        for kind, relkey, qonly in [
            ("QUAL", "qualification_path", True),
            ("FUNC", "functional_path", False),
        ]:
            p = ROOT / l.get(relkey, "")
            if not p.exists():
                add(findings, f"LLAMA_{workload}_{kind}_MISSING", str(p))
                continue
            d = load(p)
            if d.get("baseline_commit") != LLAMA_COMMIT:
                add(findings, f"LLAMA_{workload}_{kind}_COMMIT", str(d.get("baseline_commit")))
            if d.get("success") is not True:
                add(findings, f"LLAMA_{workload}_{kind}_SUCCESS", str(d.get("error")))
            rt = d.get("runtime", {})
            if rt.get("vulkan_log_present") is not True or rt.get("full_offload") is not True:
                add(findings, f"LLAMA_{workload}_{kind}_OFFLOAD", str(rt))
            r = d.get("resolved", {})
            expected = {
                "n_ctx": 4096,
                "n_batch": 256,
                "n_ubatch": 256,
                "threads": 8,
                "threads_batch": 8,
                "kv_k": "F32",
                "kv_v": "F32",
                "n_gpu_layers_requested": -1,
            }
            bad = [k for k, v in expected.items() if r.get(k) != v]
            if bad:
                add(findings, f"LLAMA_{workload}_{kind}_SETTINGS", ",".join(bad))
            if not qonly:
                if d.get("final_logits_finite") is not True or len(d.get("generated_token_ids", [])) != 32:
                    add(findings, f"LLAMA_{workload}_{kind}_INFERENCE", "finite/count")

    for name, expected in ev.get("active_shader_sha256", {}).items():
        p = ROOT / "compiled_shaders" / name
        if not p.exists() or sha256(p) != expected:
            add(findings, "SHADER_HASH", name)

    for path, expected_blob in ev.get("critical_git_blobs", {}).items():
        try:
            got = git("rev-parse", f"HEAD:{path}")
        except subprocess.CalledProcessError:
            add(findings, "CRITICAL_BLOB_MISSING", path)
            continue
        if got != expected_blob:
            add(findings, "CRITICAL_BLOB_DRIFT", path)

    runner = (ROOT / "tools" / "run_core0b_primary.py").read_text(encoding="utf-8")
    lowered = runner.lower()
    forbidden = [
        "q2_resource_sampler",
        "q2_gpu_sampler",
        "token_xray_trace",
        "hardware_counters_used": true",
        "vtune",
        "nsight",
        "nsys",
    ]
    for term in forbidden:
        if term in lowered:
            add(findings, "PRIMARY_RUNNER_INSTRUMENTATION", term)
    for required in [
        "UNINSTRUMENTED_CHILD_WALL_MS",
        "authorization artifact is absent",
        "primary_requests_authorized",
        "time.perf_counter_ns",
    ]:
        if required not in runner:
            add(findings, "PRIMARY_RUNNER_GUARD_MISSING", required)

    verdict = (
        "PASS_CORE0B_ZERO_SCIENCE_PREFLIGHT"
        if not findings
        else "STOP_CORE0B_ZERO_SCIENCE_PREFLIGHT"
    )
    result = {
        "schema": "arcllm.core0b.preflight_independent_adjudication.v0.1",
        "verdict": verdict,
        "open_findings": len(findings),
        "findings": findings,
        "evidence_path": str(ev_path),
        "implementation_head": ev.get("implementation_head"),
        "checked_head": head,
        "science_execution": False,
        "primary_requests_executed": 0,
        "performance_claim_created": False,
        "mechanism_selected": False,
        "next_if_pass": "CREATE_EXACT_CORE0B_SCIENCE_AUTHORIZATION_BUT_DO_NOT_EXECUTE_PRIMARY_COLLECTION",
    }
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"CORE0B_PREFLIGHT_ADJUDICATION={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    for f in findings:
        print(f"FINDING {f['id']}: {f['detail']}")
    return 0 if not findings else 3


if __name__ == "__main__":
    raise SystemExit(main())
