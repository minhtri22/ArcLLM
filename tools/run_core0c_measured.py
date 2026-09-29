#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODEL_SHA = "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
MODEL_BYTES = 4683074048
CORE0B_DATASET_COMMIT = "3985a2ac1697225489da01564dbc3e49a298ef69"

WORKLOADS = {
    "W-S": {
        "tokens": [1, 133151, 133152, 152062],
        "profile": "0",
        "reference": ROOT / "results" / "core0b_primary_20260929T033358387271Z" / "A_WS_p0_arc.json",
    },
    "W-C": {
        "tokens": [1, 133151, 133152, 152062] + [
            1 + ((104729 + 7919 * i) % 152063) for i in range(4, 256)
        ],
        "profile": "1",
        "reference": ROOT / "results" / "core0b_primary_20260929T033358387271Z" / "A_WC_p0_arc.json",
    },
}
ORDER = {
    0: ["CONTROL_UNINSTRUMENTED_EXTERNAL_WALL", "TOKEN_XRAY_TRACE"],
    1: ["TOKEN_XRAY_TRACE", "CONTROL_UNINSTRUMENTED_EXTERNAL_WALL"],
    2: ["CONTROL_UNINSTRUMENTED_EXTERNAL_WALL", "TOKEN_XRAY_TRACE"],
}


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def stats(xs: list[float]) -> dict:
    med = statistics.median(xs)
    return {
        "n": len(xs),
        "median": med,
        "min": min(xs),
        "max": max(xs),
        "mad": statistics.median(abs(x - med) for x in xs),
    }


def validate_arc_result(d: dict, reference: dict) -> list[str]:
    errors: list[str] = []
    if d.get("generated_token_ids") != reference.get("generated_token_ids"):
        errors.append("generated_token_identity")
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
        "finite": True,
    }
    for k, v in expected.items():
        if s.get(k) != v:
            errors.append(f"{k}={s.get(k)!r}")
    return errors


def external_child(cmd: list[str], cwd: Path, env: dict[str, str] | None = None) -> tuple[int, float]:
    t0 = time.perf_counter_ns()
    cp = subprocess.run(cmd, cwd=cwd, env=env)
    t1 = time.perf_counter_ns()
    return cp.returncode, (t1 - t0) / 1_000_000.0


def observer_summary(rows: list[dict]) -> dict:
    out: dict[str, dict] = {}
    for workload in WORKLOADS:
        pairs = [r for r in rows if r["workload"] == workload]
        ratios: list[float] = []
        details: list[dict] = []
        for row in pairs:
            control = row["arms"]["CONTROL_UNINSTRUMENTED_EXTERNAL_WALL"]["wall_ms"]
            trace = row["arms"]["TOKEN_XRAY_TRACE"]["wall_ms"]
            ratio = trace / control
            ratios.append(ratio)
            details.append({
                "block": row["block"],
                "observer_ratio_trace_over_control": ratio,
                "observer_overhead_fraction": ratio - 1.0,
                "observer_overhead_percent": (ratio - 1.0) * 100.0,
            })
        out[workload] = {
            "pairs": details,
            "ratio_stats": stats(ratios),
            "overhead_fraction_stats": stats([x - 1.0 for x in ratios]),
        }
    return out


def fixture_self_test() -> int:
    rows = []
    synthetic = {
        "W-S": [(100.0, 110.0), (120.0, 126.0), (90.0, 99.0)],
        "W-C": [(200.0, 220.0), (180.0, 198.0), (240.0, 264.0)],
    }
    for workload, vals in synthetic.items():
        for block, (control, trace) in enumerate(vals):
            rows.append({
                "workload": workload,
                "block": block,
                "arms": {
                    "CONTROL_UNINSTRUMENTED_EXTERNAL_WALL": {"wall_ms": control},
                    "TOKEN_XRAY_TRACE": {"wall_ms": trace},
                },
            })
    out = observer_summary(rows)
    ws = out["W-S"]["ratio_stats"]["median"]
    wc = out["W-C"]["ratio_stats"]["median"]
    if not math.isclose(ws, 1.1, rel_tol=0, abs_tol=1e-12):
        raise SystemExit("fixture observer W-S median mismatch")
    if not math.isclose(wc, 1.1, rel_tol=0, abs_tol=1e-12):
        raise SystemExit("fixture observer W-C median mismatch")
    print("CORE0C_OBSERVER_FIXTURE_SELF_TEST=PASS")
    print(json.dumps(out, indent=2))
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--model")
    ap.add_argument("--token-xray-root")
    ap.add_argument("--authorization", default=str(ROOT / "config" / "core0c_science_authorization.json"))
    ap.add_argument("--fixture-self-test", action="store_true")
    args = ap.parse_args()

    if args.fixture_self_test:
        return fixture_self_test()

    if not args.model or not args.token_xray_root:
        raise SystemExit("STOP: CORE0C measured runner requires --model and --token-xray-root")

    auth_path = Path(args.authorization)
    if not auth_path.exists():
        raise SystemExit("STOP: CORE0C science authorization artifact is absent")
    auth = load(auth_path)
    if auth.get("schema") != "arcllm.core0c.science_authorization.v0.1":
        raise SystemExit("STOP: invalid CORE0C authorization schema")
    if auth.get("authorized") is not True or auth.get("measured_requests_authorized") != 12:
        raise SystemExit("STOP: CORE0C 12-request collection is not authorized")
    if auth.get("timing_authority") != "DIAGNOSTIC_ONLY":
        raise SystemExit("STOP: CORE0C trace timing authority drift")
    if auth.get("core0b_performance_authority_preserved") is not True:
        raise SystemExit("STOP: CORE0B authority preservation missing")

    model = Path(args.model).resolve()
    tx = Path(args.token_xray_root).resolve()
    control_exe = (ROOT / "arcllm_v1_runtime.exe").resolve()
    trace_exe = (ROOT / "artifacts" / "core0c_trace" / "arcllm_v1_core0c_trace.exe").resolve()
    shader_dir = (ROOT / "compiled_shaders").resolve()

    if not model.exists() or model.stat().st_size != MODEL_BYTES or sha256(model) != MODEL_SHA:
        raise SystemExit("STOP: CORE0C exact model identity mismatch")
    if sha256(control_exe) != auth["control_exe_sha256"]:
        raise SystemExit("STOP: CORE0C control executable hash drift")
    if sha256(trace_exe) != auth["trace_exe_sha256"]:
        raise SystemExit("STOP: CORE0C trace executable hash drift")

    head = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip()
    canonical_parent = auth.get("canonical_parent")
    if not canonical_parent or subprocess.run(
        ["git", "-C", str(ROOT), "merge-base", "--is-ancestor", canonical_parent, head]
    ).returncode != 0:
        raise SystemExit("STOP: CORE0C canonical-parent ancestry drift")
    for path, expected_blob in auth.get("critical_git_blobs", {}).items():
        try:
            got = subprocess.check_output(
                ["git", "-C", str(ROOT), "rev-parse", f"HEAD:{path}"], text=True
            ).strip()
        except subprocess.CalledProcessError:
            raise SystemExit(f"STOP: CORE0C critical git blob missing: {path}")
        if got != expected_blob:
            raise SystemExit(f"STOP: CORE0C critical git blob drift: {path}")
    instrumentation = auth.get("instrumentation", {})
    if any(instrumentation.get(k) is not False for k in (
        "hardware_counters", "profiler", "resource_sampler"
    )):
        raise SystemExit("STOP: CORE0C forbidden instrumentation authorization drift")
    if auth.get("mechanism_selection_authorized") is not False:
        raise SystemExit("STOP: CORE0C mechanism selection must remain unauthorized")
    if auth.get("core0d_authorized") is not False:
        raise SystemExit("STOP: CORE0D must remain unauthorized during CORE0C collection")

    tx_head = subprocess.check_output(["git", "-C", str(tx), "rev-parse", "HEAD"], text=True).strip()
    if tx_head != auth["token_xray_head"]:
        raise SystemExit("STOP: CORE0C Token-XRay freeze drift")

    for name, expected in auth["active_shader_sha256"].items():
        p = shader_dir / name
        if not p.exists() or sha256(p) != expected:
            raise SystemExit(f"STOP: CORE0C shader drift: {name}")

    build = load(ROOT / "results" / "CORE0C_BUILD_QUALIFICATION.json")
    period = build["timestamp_period_ns"]
    bits = build["timestamp_valid_bits"]
    if period != auth["timestamp_period_ns"] or bits != auth["timestamp_valid_bits"]:
        raise SystemExit("STOP: CORE0C timestamp preflight drift")

    # Import exact frozen Token-XRay validators.
    sys.path.insert(0, str(tx / "src"))
    from token_xray.runtime_trace import load_json as tx_load, validate_trace_semantics
    from token_xray.runtime_lifecycle import validate_lifecycle_trace

    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    out_dir = ROOT / "results" / f"core0c_measured_{stamp}"
    if any((ROOT / "results").glob("core0c_measured_*")):
        raise SystemExit("STOP: CORE0C measured dataset already exists")
    out_dir.mkdir(parents=True, exist_ok=False)

    rows: list[dict] = []
    for workload, w in WORKLOADS.items():
        reference = load(w["reference"])
        for block in range(3):
            row = {"workload": workload, "block": block, "order": ORDER[block], "arms": {}}
            for ordinal, mode in enumerate(ORDER[block]):
                arm_tag = "control" if mode.startswith("CONTROL") else "trace"
                result_path = out_dir / f"{workload.replace('-', '_')}_b{block}_{arm_tag}.json"
                cmd = [
                    str(control_exe if arm_tag == "control" else trace_exe),
                    "--model", str(model),
                    "--shader-dir", str(shader_dir),
                    "--tokens", ",".join(map(str, w["tokens"])),
                    "--max-new", "32",
                    "--profile", w["profile"],
                    "--within-validated-domain", "1",
                    "--out", str(result_path),
                ]
                env = None
                trace_dir = None
                run_id = None
                if arm_tag == "trace":
                    run_id = f"core0c-{workload.replace('-', '').lower()}-b{block}"
                    trace_dir = out_dir / f"{run_id}_traces"
                    trace_dir.mkdir()
                    env = os.environ.copy()
                    env["ARCLLM_CORE0C_TRACE_DIR"] = str(trace_dir)
                    env["ARCLLM_CORE0C_RUN_ID"] = run_id
                    env["ARCLLM_CORE0C_REQUEST_ID"] = f"{run_id}-request"
                    env["ARCLLM_CORE0C_TIMESTAMP_PERIOD_NS"] = str(period)
                    env["ARCLLM_CORE0C_TIMESTAMP_VALID_BITS"] = str(bits)

                rc, wall_ms = external_child(cmd, ROOT, env)
                parsed = load(result_path) if result_path.exists() else {}
                errors = validate_arc_result(parsed, reference)
                if rc != 0:
                    errors.append(f"returncode={rc}")

                trace_validation = None
                if arm_tag == "trace" and trace_dir is not None:
                    prefill = sorted(trace_dir.glob(f"{run_id}_prefill_*.json"))
                    decode = sorted(trace_dir.glob(f"{run_id}_decode_*.json"))
                    life = trace_dir / f"{run_id}_runtime_lifecycle.json"
                    if len(prefill) != 1:
                        errors.append(f"prefill_trace_count={len(prefill)}")
                    if len(decode) != 31:
                        errors.append(f"decode_trace_count={len(decode)}")
                    if not life.exists():
                        errors.append("lifecycle_trace_missing")

                    trace_summaries = []
                    for p in prefill + decode:
                        t = tx_load(p)
                        s = validate_trace_semantics(t)
                        expected_dispatch = 441 if t["token"]["mode"] == "prefill" else 469
                        if s["dispatch_count"] != expected_dispatch:
                            errors.append(f"{p.name}:dispatch_count={s['dispatch_count']}")
                        if s["timestamped_dispatch_count"] != expected_dispatch:
                            errors.append(f"{p.name}:timestamp_coverage={s['timestamped_dispatch_count']}")
                        if t.get("measurement_context", {}).get("measurement_mode") != "TOKEN_XRAY_TRACE":
                            errors.append(f"{p.name}:measurement_mode")
                        if t.get("measurement_context", {}).get("timing_authority") != "DIAGNOSTIC_ONLY":
                            errors.append(f"{p.name}:timing_authority")
                        semantic_ids = {
                            sid for d in t["dispatches"] for sid in d["semantic_node_ids"]
                        }
                        if len(semantic_ids) != 451:
                            errors.append(f"{p.name}:semantic_nodes={len(semantic_ids)}")
                        trace_summaries.append({
                            "file": p.name,
                            "mode": t["token"]["mode"],
                            "dispatch_count": s["dispatch_count"],
                            "semantic_node_count": len(semantic_ids),
                        })

                    lifecycle_summary = None
                    if life.exists():
                        lt = tx_load(life)
                        lifecycle_summary = validate_lifecycle_trace(lt)
                        types = [e["event_type"] for e in lt["events"]]
                        required_types = {
                            "representation_acquire", "representation_materialize",
                            "representation_validate", "representation_resident",
                            "representation_reuse", "representation_evict",
                            "representation_release",
                        }
                        if not required_types.issubset(set(types)):
                            errors.append("lifecycle_event_types")
                        if types.count("representation_materialize") != 14:
                            errors.append(f"materialize_event_count={types.count('representation_materialize')}")

                    trace_validation = {
                        "token_traces": trace_summaries,
                        "lifecycle": lifecycle_summary,
                    }

                row["arms"][mode] = {
                    "ordinal": ordinal,
                    "wall_ms": wall_ms,
                    "returncode": rc,
                    "valid": not errors,
                    "errors": errors,
                    "result_file": result_path.name,
                    "trace_dir": trace_dir.name if trace_dir else None,
                    "trace_validation": trace_validation,
                }
            row["valid"] = all(a["valid"] for a in row["arms"].values())
            rows.append(row)

    complete = len(rows) == 6 and all(r["valid"] for r in rows)
    summary = {
        "schema": "arcllm.core0c.measured_summary.v0.1",
        "classification": "CORE0C_COLLECTION_COMPLETE_PENDING_ADJUDICATION" if complete else "CORE0C_COLLECTION_INCOMPLETE",
        "measurement_mode": "TOKEN_XRAY_TRACE_WITH_ADJACENT_CONTROL",
        "timing_authority": "DIAGNOSTIC_ONLY",
        "core0b_performance_authority_preserved": True,
        "measured_requests_expected": 12,
        "measured_requests_observed": len(rows) * 2,
        "rows": rows,
        "observer_effect": observer_summary(rows) if complete else None,
        "hardware_counters_used": False,
        "profiler_used": False,
        "mechanism_selected": False,
    }
    (out_dir / "CORE0C_MEASURED_SUMMARY.json").write_text(
        json.dumps(summary, indent=2) + "\n", encoding="utf-8"
    )
    print(f"CORE0C_COLLECTION={summary['classification']}")
    print(f"RESULTS_DIR={out_dir}")
    return 0 if complete else 3


if __name__ == "__main__":
    raise SystemExit(main())
