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

ARC_PARENT = "7a5672112dc22de15f0e9bb6445508fbb3099b15"
LLAMA_COMMIT = "b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
MODEL_SHA256 = "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
MODEL_BYTES = 4683074048

ROOT = Path(__file__).resolve().parents[1]

CELLS = [
    ("A_WS", "A", "W-S"),
    ("A_WC", "A", "W-C"),
    ("B_WS", "B", "W-S"),
    ("B_WC", "B", "W-C"),
]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def git(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def tokens_for(workload: str) -> list[int]:
    if workload == "W-S":
        return [1, 133151, 133152, 152062]
    if workload == "W-C":
        out = [1, 133151, 133152, 152062]
        out += [1 + ((104729 + 7919 * i) % 152063) for i in range(4, 256)]
        return out
    raise ValueError(workload)


def stats(xs: list[float]) -> dict | None:
    if not xs:
        return None
    med = statistics.median(xs)
    mad = statistics.median(abs(x - med) for x in xs)
    return {"n": len(xs), "median": med, "min": min(xs), "max": max(xs), "mad": mad}


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def validate_arc(d: dict) -> tuple[bool, list[str]]:
    errors: list[str] = []
    s = d.get("stats", {})
    if len(d.get("generated_token_ids", [])) != 32:
        errors.append("generated_count")
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
            errors.append(f"{k}={s.get(k)!r} expected {v}")
    if s.get("finite") is not True:
        errors.append("finite")
    return not errors, errors


def validate_llama(d: dict) -> tuple[bool, list[str]]:
    errors: list[str] = []
    if d.get("baseline_commit") != LLAMA_COMMIT:
        errors.append("baseline_commit")
    if d.get("success") is not True:
        errors.append("success")
    if d.get("final_logits_finite") is not True:
        errors.append("finite")
    if len(d.get("generated_token_ids", [])) != 32:
        errors.append("generated_count")
    if d.get("runtime", {}).get("full_offload") is not True:
        errors.append("full_offload")
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
    for k, v in expected.items():
        if r.get(k) != v:
            errors.append(f"{k}={r.get(k)!r} expected {v}")
    return not errors, errors


def measured_child(cmd: list[str], cwd: Path) -> tuple[int, float]:
    t0 = time.perf_counter_ns()
    cp = subprocess.run(cmd, cwd=cwd)
    t1 = time.perf_counter_ns()
    return cp.returncode, (t1 - t0) / 1_000_000.0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", required=True)
    ap.add_argument(
        "--authorization",
        default=str(ROOT / "config" / "core0b_science_authorization.json"),
    )
    args = ap.parse_args()

    auth_path = Path(args.authorization)
    if not auth_path.exists():
        raise SystemExit("STOP: CORE0B science authorization artifact is absent")
    auth = load_json(auth_path)
    if auth.get("schema") != "arcllm.core0b.science_authorization.v0.1":
        raise SystemExit("STOP: invalid CORE0B authorization schema")
    if auth.get("authorized") is not True or auth.get("primary_requests_authorized") != 40:
        raise SystemExit("STOP: CORE0B primary collection is not authorized")
    if auth.get("measurement_authority") != "UNINSTRUMENTED_CHILD_WALL_MS":
        raise SystemExit("STOP: CORE0B measurement authority drift")

    head = git("rev-parse", "HEAD")
    if subprocess.run(
        ["git", "-C", str(ROOT), "merge-base", "--is-ancestor", auth["implementation_head"], head]
    ).returncode != 0:
        raise SystemExit("STOP: CORE0B implementation head is not an ancestor")
    if subprocess.run(
        ["git", "-C", str(ROOT), "merge-base", "--is-ancestor", ARC_PARENT, head]
    ).returncode != 0:
        raise SystemExit("STOP: canonical ArcLLM parent is not preserved")

    model = Path(args.model).resolve()
    arc_exe = (ROOT / "arcllm_v1_runtime.exe").resolve()
    llama_exe = (ROOT / "artifacts" / "core0b_baseline" / "core0b_llama_cold_adapter.exe").resolve()
    shader_dir = (ROOT / "compiled_shaders").resolve()
    for p in [model, arc_exe, llama_exe, shader_dir]:
        if not p.exists():
            raise SystemExit(f"STOP: required CORE0B path missing: {p}")
    if model.stat().st_size != MODEL_BYTES or sha256(model) != MODEL_SHA256:
        raise SystemExit("STOP: CORE0B exact model identity mismatch")
    if sha256(arc_exe) != auth["arc_exe_sha256"]:
        raise SystemExit("STOP: ArcLLM executable hash drift")
    if sha256(llama_exe) != auth["llama_exe_sha256"]:
        raise SystemExit("STOP: llama adapter executable hash drift")

    for path, expected_blob in auth.get("critical_git_blobs", {}).items():
        got = git("rev-parse", f"HEAD:{path}")
        if got != expected_blob:
            raise SystemExit(f"STOP: critical git blob drift: {path}")

    if any(k.upper().startswith(("TOKEN_XRAY", "VTUNE", "NSYS", "NSIGHT")) for k in os.environ):
        raise SystemExit("STOP: profiler/instrumentation environment detected")

    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    out_dir = ROOT / "results" / f"core0b_primary_{stamp}"
    out_dir.mkdir(parents=True, exist_ok=False)

    pair_rows: list[dict] = []
    for cell, session, workload in CELLS:
        toks = tokens_for(workload)
        for pair in range(5):
            offset = 1 if session == "B" else 0
            arc_first = ((pair + offset) % 2) == 0
            order = ["arc", "llama"] if arc_first else ["llama", "arc"]
            arm_rows: dict[str, dict] = {}
            for ordinal, arm in enumerate(order):
                result_path = out_dir / f"{cell}_p{pair}_{arm}.json"
                if arm == "arc":
                    profile = "0" if workload == "W-S" else "1"
                    cmd = [
                        str(arc_exe),
                        "--model", str(model),
                        "--shader-dir", str(shader_dir),
                        "--tokens", ",".join(map(str, toks)),
                        "--max-new", "32",
                        "--profile", profile,
                        "--within-validated-domain", "1",
                        "--out", str(result_path),
                    ]
                else:
                    cmd = [
                        str(llama_exe),
                        "--model", str(model),
                        "--workload", workload,
                        "--out", str(result_path),
                    ]
                rc, wall_ms = measured_child(cmd, ROOT)
                parsed = load_json(result_path) if result_path.exists() else {}
                if arm == "arc":
                    valid, errors = validate_arc(parsed)
                else:
                    valid, errors = validate_llama(parsed)
                valid = valid and rc == 0
                arm_rows[arm] = {
                    "ordinal": ordinal,
                    "returncode": rc,
                    "wall_ms": wall_ms,
                    "valid": valid,
                    "errors": errors,
                    "result_file": result_path.name,
                }
            valid_pair = arm_rows["arc"]["valid"] and arm_rows["llama"]["valid"]
            ratio = (
                arm_rows["arc"]["wall_ms"] / arm_rows["llama"]["wall_ms"]
                if valid_pair and arm_rows["llama"]["wall_ms"] > 0
                else None
            )
            pair_rows.append({
                "cell": cell,
                "session": session,
                "workload": workload,
                "pair": pair,
                "order": order,
                "valid": valid_pair,
                "arc": arm_rows["arc"],
                "llama": arm_rows["llama"],
                "request_wall_ratio_arc_over_llama": ratio,
            })

    cells: dict[str, dict] = {}
    cell_medians: list[float] = []
    complete = True
    for cell, session, workload in CELLS:
        rows = [r for r in pair_rows if r["cell"] == cell]
        ratios = [r["request_wall_ratio_arc_over_llama"] for r in rows if r["valid"]]
        s = stats(ratios)
        cells[cell] = {
            "session": session,
            "workload": workload,
            "pairs_recorded": len(rows),
            "valid_pairs": len(ratios),
            "request_wall_ratio_arc_over_llama": s,
        }
        if len(ratios) < 3:
            complete = False
        if s:
            cell_medians.append(float(s["median"]))

    if complete and len(pair_rows) == 20 and len(cell_medians) == 4:
        classification = "CORE0B_MATCHED_REQUEST_CHARACTERIZATION_COMPLETE"
        geomean = math.exp(sum(math.log(x) for x in cell_medians) / 4.0)
    else:
        any_baseline_invalid = any(not r["llama"]["valid"] for r in pair_rows)
        any_arc_invalid = any(not r["arc"]["valid"] for r in pair_rows)
        if any_baseline_invalid:
            classification = "CORE0B_BASELINE_NOT_MATCHED"
        elif any_arc_invalid:
            classification = "CORE0B_RUNTIME_INCOMPLETE"
        else:
            classification = "CORE0B_MEASUREMENT_INVALID"
        geomean = None

    summary = {
        "schema": "arcllm.core0b.primary_summary.v0.1",
        "classification": classification,
        "measurement_mode": "UNINSTRUMENTED",
        "timing_authority": "BENCHMARK_AUTHORITY",
        "measurement_authority": "UNINSTRUMENTED_CHILD_WALL_MS",
        "primary_requests_expected": 40,
        "primary_requests_observed": len(pair_rows) * 2,
        "cells": cells,
        "global_geometric_mean_of_cell_medians_arc_over_llama": geomean,
        "pairs": pair_rows,
        "token_xray_used": False,
        "hardware_counters_used": False,
        "profiler_used": False,
        "mechanism_selection_authorized": False,
    }
    (out_dir / "CORE0B_PRIMARY_SUMMARY.json").write_text(
        json.dumps(summary, indent=2) + "\n", encoding="utf-8"
    )
    print(f"CORE0B_CLASSIFICATION={classification}")
    print(f"RESULTS_DIR={out_dir}")
    return 0 if classification == "CORE0B_MATCHED_REQUEST_CHARACTERIZATION_COMPLETE" else 3


if __name__ == "__main__":
    raise SystemExit(main())
