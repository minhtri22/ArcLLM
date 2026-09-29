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

DECODE_LAYER_OPS = [
    "attn_rmsnorm","q_proj","k_proj","v_proj","q_rope","k_rope","kv_store",
    "cached_gqa","o_proj","attn_residual","ffn_rmsnorm","ffn_gate","ffn_up",
    "swiglu","ffn_down","ffn_residual",
]
PREFILL_LAYER_OPS = [
    "attn_rmsnorm","q_proj","k_proj","v_proj","q_rope","k_rope","kv_store",
    "causal_gqa","o_proj","attn_residual","ffn_rmsnorm","ffn_gate_up_fused",
    "swiglu","ffn_down","ffn_residual",
]


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--token-xray-root", required=True)
    ap.add_argument("--out", default=str(ROOT / "results" / "CORE0C_PREFLIGHT_EVIDENCE.json"))
    args = ap.parse_args()

    tx = Path(args.token_xray_root).resolve()
    out = Path(args.out).resolve()
    findings: list[dict] = []
    details: dict = {}

    def add(fid: str, detail: str) -> None:
        findings.append({"id": fid, "detail": detail})

    build_path = ROOT / "results" / "CORE0C_BUILD_QUALIFICATION.json"
    if not build_path.exists():
        add("BUILD_EVIDENCE_MISSING", str(build_path))
        build = {}
    else:
        build = load(build_path)
        if build.get("status") != "PASS_CORE0C_BUILD_AND_SYNTHETIC_FIXTURE":
            add("BUILD_STATUS", repr(build.get("status")))

    tx_head = subprocess.check_output(["git","-C",str(tx),"rev-parse","HEAD"],text=True).strip()
    if tx_head != TX_HEAD:
        add("TOKEN_XRAY_HEAD", tx_head)
    if subprocess.check_output(["git","-C",str(tx),"status","--porcelain"],text=True).strip():
        add("TOKEN_XRAY_DIRTY", str(tx))

    measured_dirs = sorted(p.name for p in (ROOT / "results").glob("core0c_measured_*") if p.is_dir())
    if measured_dirs:
        add("MEASURED_DATASET_EXISTS", repr(measured_dirs))

    # Exact frozen Token-XRay regression subset.
    pytest = subprocess.run(
        [
            sys.executable, "-m", "pytest", "-q",
            "tests/test_arcllm_adapter.py",
            "tests/test_runtime_trace.py",
            "tests/test_runtime_lifecycle.py",
            "tests/test_sdk_compile.py",
        ],
        cwd=tx, text=True, capture_output=True,
    )
    details["token_xray_pytest"] = {
        "returncode": pytest.returncode,
        "stdout": pytest.stdout[-4000:],
        "stderr": pytest.stderr[-4000:],
    }
    if pytest.returncode != 0:
        add("TOKEN_XRAY_REGRESSION", "selected compatibility tests failed")

    sys.path.insert(0, str(tx / "src"))
    from token_xray.adapters import arcllm_v1 as adapter
    from token_xray.runtime_trace import (
        RuntimeTraceError,
        join_trace_into_ledger,
        load_json as tx_load,
        validate_trace_semantics,
    )
    from token_xray.runtime_lifecycle import validate_lifecycle_trace

    # Full static geometry coverage.
    decode_geom = sum(DECODE_SHADER_COUNTS.values())
    prefill_geom = sum(PREFILL_SHADER_COUNTS.values())
    try:
        for shader in set(DECODE_SHADER_COUNTS) | set(PREFILL_SHADER_COUNTS):
            adapter.local_size(shader)
    except Exception as exc:
        add("GEOMETRY_MAPPING", str(exc))
    if decode_geom != 469:
        add("DECODE_GEOMETRY", str(decode_geom))
    if prefill_geom != 441:
        add("PREFILL_GEOMETRY", str(prefill_geom))

    decode_ids: set[str] = set()
    prefill_ids: set[str] = set()
    try:
        decode_ids.update(adapter.semantic_node_ids("token_embedding", "decode"))
        prefill_ids.update(adapter.semantic_node_ids("token_embedding", "prefill"))
        for layer in range(28):
            for suffix in DECODE_LAYER_OPS:
                decode_ids.update(adapter.semantic_node_ids(f"L{layer:02d}.{suffix}", "decode"))
            for suffix in PREFILL_LAYER_OPS:
                prefill_ids.update(adapter.semantic_node_ids(f"L{layer:02d}.{suffix}", "prefill"))
        decode_ids.update(adapter.semantic_node_ids("output_norm", "decode"))
        decode_ids.update(adapter.semantic_node_ids("lm_head", "decode"))
        prefill_ids.update(adapter.semantic_node_ids("output_norm", "prefill"))
        prefill_ids.update(adapter.semantic_node_ids("lm_head", "prefill"))
    except Exception as exc:
        add("SEMANTIC_MAPPING", str(exc))
    if len(decode_ids) != 451:
        add("DECODE_SEMANTICS", str(len(decode_ids)))
    if len(prefill_ids) != 451:
        add("PREFILL_SEMANTICS", str(len(prefill_ids)))

    # Lifecycle separation: semantic path must reject, lifecycle path must accept.
    lifecycle_separation = False
    try:
        adapter.semantic_node_ids("Q4V4.P1.L3", "decode")
    except ValueError:
        try:
            e = adapter.q4v4_lifecycle_event("Q4V4.P1.L3")
            lifecycle_separation = e["event_type"] == "representation_materialize"
        except Exception:
            lifecycle_separation = False
    if not lifecycle_separation:
        add("LIFECYCLE_SEPARATION", "Q4V4 lifecycle separation failed")

    fixture_dir = ROOT / "results" / "core0c_fixture"
    prefill_file = fixture_dir / "core0c-fixture_prefill_000.json"
    decode_file = fixture_dir / "core0c-fixture_decode_000.json"
    life_file = fixture_dir / "core0c-fixture_runtime_lifecycle.json"
    for p in [prefill_file, decode_file, life_file]:
        if not p.exists():
            add("FIXTURE_MISSING", str(p))

    fixture_result = {}
    if all(p.exists() for p in [prefill_file, decode_file, life_file]):
        prefill = tx_load(prefill_file)
        decode = tx_load(decode_file)
        life = tx_load(life_file)
        try:
            ps = validate_trace_semantics(prefill)
            ds = validate_trace_semantics(decode)
            ls = validate_lifecycle_trace(life)
            fixture_result = {"prefill": ps, "decode": ds, "lifecycle": ls}
        except Exception as exc:
            add("FIXTURE_SEMANTICS", str(exc))
        for name, trace, mode in [("prefill",prefill,"prefill"),("decode",decode,"decode")]:
            ctx = trace.get("measurement_context", {})
            if trace.get("schema_version") != "0.2":
                add("TRACE_SCHEMA", name)
            if trace.get("token", {}).get("mode") != mode:
                add("TRACE_PHASE", name)
            if ctx.get("measurement_mode") != "TOKEN_XRAY_TRACE" or ctx.get("timing_authority") != "DIAGNOSTIC_ONLY":
                add("TRACE_AUTHORITY", name)
            if ctx.get("instrumentation_overhead") is not None:
                add("TRACE_OVERHEAD_NULL", name)
        fused = [
            d for d in prefill.get("dispatches", [])
            if d.get("runtime_name") == "L00.ffn_gate_up_fused"
        ]
        if len(fused) != 1 or len(fused[0].get("semantic_node_ids", [])) != 2:
            add("FUSED_SHARED_MAPPING", repr(fused))

        # Explicit phase mismatch rejection.
        fake_decode_ledger = {
            "artifact_type": "NODE_LEDGER",
            "mode": "decode_template",
            "source_model": {"sha256": MODEL_SHA},
            "hardware_profile_id": "arc140v-dev-host",
            "nodes": [],
        }
        mismatch_rejected = False
        try:
            join_trace_into_ledger(fake_decode_ledger, prefill, strict=True)
        except RuntimeTraceError as exc:
            mismatch_rejected = "phase mismatch" in str(exc)
        if not mismatch_rejected:
            add("PHASE_MISMATCH_GUARD", "prefill trace joined into decode ledger")
        fixture_result["phase_mismatch_rejected"] = mismatch_rejected

        event_types = {e["event_type"] for e in life.get("events", [])}
        required = {
            "representation_acquire","representation_materialize",
            "representation_validate","representation_resident",
            "representation_reuse","representation_evict","representation_release",
        }
        if not required.issubset(event_types):
            add("LIFECYCLE_EVENT_COVERAGE", repr(sorted(event_types)))

    observer = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "run_core0c_measured.py"), "--fixture-self-test"],
        cwd=ROOT, text=True, capture_output=True,
    )
    details["observer_fixture"] = {
        "returncode": observer.returncode,
        "stdout": observer.stdout[-5000:],
        "stderr": observer.stderr[-2000:],
    }
    if observer.returncode != 0 or "CORE0C_OBSERVER_FIXTURE_SELF_TEST=PASS" not in observer.stdout:
        add("OBSERVER_RUNNER_FIXTURE", "observer self-test failed")

    absent_auth = ROOT / "results" / "INTENTIONALLY_ABSENT_CORE0C_AUTH.json"
    if absent_auth.exists():
        absent_auth.unlink()
    guard = subprocess.run(
        [
            sys.executable, str(ROOT / "tools" / "run_core0c_measured.py"),
            "--model", "INTENTIONALLY_UNUSED",
            "--token-xray-root", str(tx),
            "--authorization", str(absent_auth),
        ],
        cwd=ROOT, text=True, capture_output=True,
    )
    guard_text = (guard.stdout or "") + (guard.stderr or "")
    fail_closed = guard.returncode != 0 and "authorization artifact is absent" in guard_text
    if not fail_closed:
        add("SCIENCE_AUTH_GUARD", guard_text[-1000:])

    trace_exe = ROOT / "artifacts" / "core0c_trace" / "arcllm_v1_core0c_trace.exe"
    if not trace_exe.exists():
        add("TRACE_EXE_MISSING", str(trace_exe))

    result = {
        "schema": "arcllm.core0c.preflight_evidence.v0.1",
        "status": "PASS_CORE0C_STATIC_PREFLIGHT" if not findings else "STOP_CORE0C_STATIC_PREFLIGHT",
        "classification": "ZERO_SCIENCE_IMPLEMENTATION_STATIC_PREFLIGHT",
        "implementation_head": subprocess.check_output(["git","-C",str(ROOT),"rev-parse","HEAD"],text=True).strip(),
        "token_xray_head": tx_head,
        "build_qualification": build,
        "static_contract": {
            "decode_geometry": f"{decode_geom}/469",
            "prefill_geometry": f"{prefill_geom}/441",
            "decode_semantics": f"{len(decode_ids)}/451",
            "prefill_semantics": f"{len(prefill_ids)}/451",
            "lifecycle_separation": lifecycle_separation,
        },
        "fixture_validation": fixture_result,
        "observer_runner_fixture_pass": observer.returncode == 0,
        "science_runner_fail_closed_without_authorization": fail_closed,
        "trace_exe_sha256": sha256(trace_exe) if trace_exe.exists() else None,
        "measured_dataset_dirs": measured_dirs,
        "model_inference_executed": False,
        "measured_requests_executed": 0,
        "canonical_runtime_modified": False,
        "kernel_source_modified": False,
        "details": details,
        "findings": findings,
        "open_findings": len(findings),
    }
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"CORE0C_PREFLIGHT={result['status']}")
    print(f"OPEN_FINDINGS={len(findings)}")
    print(f"DECODE_GEOMETRY={decode_geom}/469")
    print(f"PREFILL_GEOMETRY={prefill_geom}/441")
    print(f"DECODE_SEMANTICS={len(decode_ids)}/451")
    print(f"PREFILL_SEMANTICS={len(prefill_ids)}/451")
    for f in findings:
        print(f"FINDING {f['id']}: {f['detail']}")
    return 0 if not findings else 3


if __name__ == "__main__":
    raise SystemExit(main())
