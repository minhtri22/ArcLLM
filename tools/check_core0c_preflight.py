#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TX_HEAD = "35f86ac68f98ffe60fc441a790274cd1f1269dfe"
CANONICAL_BLOBS = {
    "src/arcllm_v1_vulkan_runtime_support.h": "1b3a2a935134a3afa680ee4880377a20bc8a466f",
    "src/arcllm_v1_q4_vulkan_backend_v4_runtime.h": "3955d1ed27c0c48f5bbb8e4564128bf34025d1a5",
    "src/arcllm_v1_runtime.cpp": "0c613f6f740931a88ddd3ee5b904533a58001a06",
    "src/arcllm_v1_runtime_cli.cpp": "5526658f6469301f0e05ffa5a46f51bc9b511816",
    "include/arcllm/v1/runtime.h": "d7821270ed253e1d1d96e3916b22b3daee50f5bc",
}


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def git(*args: str) -> str:
    return subprocess.check_output(["git","-C",str(ROOT),*args],text=True).strip()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--token-xray-root", required=True)
    ap.add_argument("--evidence", default=str(ROOT/"results"/"CORE0C_PREFLIGHT_EVIDENCE.json"))
    ap.add_argument("--out", default=str(ROOT/"results"/"CORE0C_PREFLIGHT_INDEPENDENT_ADJUDICATION.json"))
    args = ap.parse_args()
    tx = Path(args.token_xray_root).resolve()
    evidence = load(Path(args.evidence))
    out = Path(args.out)
    findings: list[dict] = []

    def add(fid: str, detail: str) -> None:
        findings.append({"id":fid,"detail":detail})

    if evidence.get("status") != "PASS_CORE0C_STATIC_PREFLIGHT":
        add("UPSTREAM_STATUS", repr(evidence.get("status")))
    if evidence.get("open_findings") != 0:
        add("UPSTREAM_FINDINGS", repr(evidence.get("open_findings")))
    if evidence.get("model_inference_executed") is not False or evidence.get("measured_requests_executed") != 0:
        add("SCIENCE_BOUNDARY", "preflight executed model science")
    if evidence.get("measured_dataset_dirs") != []:
        add("MEASURED_DATASET", repr(evidence.get("measured_dataset_dirs")))
    if evidence.get("science_runner_fail_closed_without_authorization") is not True:
        add("AUTH_GUARD", "runner not fail-closed")

    head = git("rev-parse","HEAD")
    impl = evidence.get("implementation_head")
    if not isinstance(impl,str) or subprocess.run(["git","-C",str(ROOT),"merge-base","--is-ancestor",impl,head]).returncode != 0:
        add("IMPLEMENTATION_ANCESTRY", f"{impl} !<= {head}")

    tx_head = subprocess.check_output(["git","-C",str(tx),"rev-parse","HEAD"],text=True).strip()
    if tx_head != TX_HEAD:
        add("TOKEN_XRAY_HEAD", tx_head)
    if subprocess.check_output(["git","-C",str(tx),"status","--porcelain"],text=True).strip():
        add("TOKEN_XRAY_DIRTY", str(tx))

    for path, expected in CANONICAL_BLOBS.items():
        got = git("rev-parse",f"HEAD:{path}")
        if got != expected:
            add("CANONICAL_BLOB_DRIFT", f"{path}: {got}")

    diff = subprocess.check_output(
        ["git","-C",str(ROOT),"diff","--name-only","7a5672112dc22de15f0e9bb6445508fbb3099b15..HEAD"],text=True
    ).splitlines()
    bad = [
        p for p in diff
        if p.startswith("shaders/")
        or p in CANONICAL_BLOBS
    ]
    if bad:
        add("CANONICAL_OR_KERNEL_DIFF", repr(bad))

    build = evidence.get("build_qualification",{})
    trace_exe = ROOT/"artifacts"/"core0c_trace"/"arcllm_v1_core0c_trace.exe"
    if not trace_exe.exists():
        add("TRACE_EXE_MISSING", str(trace_exe))
    elif sha256(trace_exe) != build.get("trace_exe_sha256"):
        add("TRACE_EXE_HASH", f"{sha256(trace_exe)} != {build.get('trace_exe_sha256')}")
    if build.get("timestamp_period_ns") != 52.0833 or build.get("timestamp_valid_bits") != 64:
        add("TIMESTAMP_FREEZE", f"{build.get('timestamp_period_ns')}/{build.get('timestamp_valid_bits')}")

    manifest = build.get("generated_runtime_manifest",{})
    expected_generated = {
        "support":"1b3a2a935134a3afa680ee4880377a20bc8a466f",
        "q4":"3955d1ed27c0c48f5bbb8e4564128bf34025d1a5",
        "runtime":"0c613f6f740931a88ddd3ee5b904533a58001a06",
    }
    if manifest.get("canonical_blobs") != expected_generated:
        add("GENERATED_PARENT_BLOBS", repr(manifest.get("canonical_blobs")))
    if manifest.get("canonical_runtime_modified") is not False or manifest.get("kernel_source_modified") is not False:
        add("DERIVATIVE_BOUNDARY", repr(manifest))

    sys.path.insert(0,str(tx/"src"))
    from token_xray.runtime_trace import load_json as tx_load, validate_trace_semantics
    from token_xray.runtime_lifecycle import validate_lifecycle_trace
    fixture = ROOT/"results"/"core0c_fixture"
    pf = fixture/"core0c-fixture_prefill_000.json"
    df = fixture/"core0c-fixture_decode_000.json"
    lf = fixture/"core0c-fixture_runtime_lifecycle.json"
    try:
        ps = validate_trace_semantics(tx_load(pf))
        ds = validate_trace_semantics(tx_load(df))
        ls = validate_lifecycle_trace(tx_load(lf))
        if ps["dispatch_count"] != 3 or ds["dispatch_count"] != 3 or ls["event_count"] != 8:
            add("FIXTURE_RECOMPUTE", f"{ps}/{ds}/{ls}")
    except Exception as exc:
        add("FIXTURE_RECOMPUTE", str(exc))

    sc = evidence.get("static_contract",{})
    for key, expected in {
        "decode_geometry":"469/469",
        "prefill_geometry":"441/441",
        "decode_semantics":"451/451",
        "prefill_semantics":"451/451",
    }.items():
        if sc.get(key) != expected:
            add("STATIC_CONTRACT", f"{key}={sc.get(key)!r}")
    if sc.get("lifecycle_separation") is not True:
        add("LIFECYCLE_SEPARATION", repr(sc.get("lifecycle_separation")))
    if evidence.get("fixture_validation",{}).get("phase_mismatch_rejected") is not True:
        add("PHASE_GUARD", "false")
    if evidence.get("observer_runner_fixture_pass") is not True:
        add("OBSERVER_FIXTURE", "false")

    measured_dirs = sorted(p.name for p in (ROOT/"results").glob("core0c_measured_*") if p.is_dir())
    if measured_dirs:
        add("MEASURED_DIR_PRESENT", repr(measured_dirs))
    if (ROOT/"config"/"core0c_science_authorization.json").exists():
        add("PREMATURE_AUTHORIZATION", "core0c_science_authorization.json exists before adjudication")

    runner = (ROOT/"tools"/"run_core0c_measured.py").read_text(encoding="utf-8")
    required = [
        "measured_requests_authorized",
        "DIAGNOSTIC_ONLY",
        "core0b_performance_authority_preserved",
        "TOKEN_XRAY_TRACE",
        "authorization artifact is absent",
        "range(3)",
        "core0c_measured_",
    ]
    for s in required:
        if s not in runner:
            add("RUNNER_CONTRACT", s)
    forbidden = ["vtune.exe","nsys.exe","nsight.exe","HARDWARE_COUNTERS"]
    for s in forbidden:
        if s in runner:
            add("RUNNER_FORBIDDEN_INSTRUMENTATION", s)

    fixture_run = subprocess.run(
        [sys.executable,str(ROOT/"tools"/"run_core0c_measured.py"),"--fixture-self-test"],
        cwd=ROOT,text=True,capture_output=True,
    )
    if fixture_run.returncode != 0 or "CORE0C_OBSERVER_FIXTURE_SELF_TEST=PASS" not in fixture_run.stdout:
        add("OBSERVER_RECOMPUTE", fixture_run.stdout[-1000:]+fixture_run.stderr[-1000:])

    verdict = "PASS_CORE0C_ZERO_SCIENCE_PREFLIGHT" if not findings else "STOP_CORE0C_ZERO_SCIENCE_PREFLIGHT"
    result = {
        "schema":"arcllm.core0c.preflight_independent_adjudication.v0.1",
        "verdict":verdict,
        "open_findings":len(findings),
        "findings":findings,
        "implementation_head":impl,
        "checked_head":head,
        "token_xray_head":tx_head,
        "trace_exe_sha256":sha256(trace_exe) if trace_exe.exists() else None,
        "timestamp_period_ns":build.get("timestamp_period_ns"),
        "timestamp_valid_bits":build.get("timestamp_valid_bits"),
        "model_inference_executed":False,
        "measured_requests_executed":0,
        "performance_authority_changed":False,
        "mechanism_selected":False,
        "next_if_pass":"FREEZE_CORE0C_EXECUTION_LOCK_AND_AUTHORIZE_EXACT_12_REQUEST_DIAGNOSTIC_COLLECTION",
    }
    out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(f"CORE0C_PREFLIGHT_ADJUDICATION={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    for f in findings:
        print(f"FINDING {f['id']}: {f['detail']}")
    return 0 if not findings else 3


if __name__=="__main__":
    raise SystemExit(main())
