#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import statistics
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATASET_NAME = "core0b_primary_20260929T033358387271Z"
DATASET_COMMIT = "3985a2ac1697225489da01564dbc3e49a298ef69"
ARC_PARENT = "7a5672112dc22de15f0e9bb6445508fbb3099b15"
IMPLEMENTATION_HEAD = "7754d898f2e4c09263e583ac2e24e2938e1f9d7a"
LLAMA_COMMIT = "b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
CELLS = [
    ("A_WS", "A", "W-S"),
    ("A_WC", "A", "W-C"),
    ("B_WS", "B", "W-S"),
    ("B_WC", "B", "W-C"),
]

ARC_STATS = {
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
LLAMA_RESOLVED = {
    "n_ctx": 4096,
    "n_batch": 256,
    "n_ubatch": 256,
    "threads": 8,
    "threads_batch": 8,
    "kv_k": "F32",
    "kv_v": "F32",
    "n_gpu_layers_requested": -1,
}
PROMPT_HASH = {"W-S": "93833ffb49890aba", "W-C": "5973d0cfd8ad6313"}


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def git(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def approx(a: float, b: float, rel: float = 1e-12, abs_: float = 1e-9) -> bool:
    return math.isclose(float(a), float(b), rel_tol=rel, abs_tol=abs_)


def calc(xs: list[float]) -> dict:
    med = statistics.median(xs)
    return {
        "n": len(xs),
        "median": med,
        "min": min(xs),
        "max": max(xs),
        "mad": statistics.median(abs(x - med) for x in xs),
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dataset", default=str(ROOT / "results" / DATASET_NAME))
    ap.add_argument(
        "--out",
        default=str(ROOT / "results" / "CORE0B_PRIMARY_INDEPENDENT_ADJUDICATION.json"),
    )
    args = ap.parse_args()

    dataset = Path(args.dataset).resolve()
    out = Path(args.out).resolve()
    findings: list[dict] = []
    notes: list[dict] = []

    def finding(fid: str, detail: str) -> None:
        findings.append({"id": fid, "detail": detail})

    if not dataset.is_dir():
        finding("DATASET_MISSING", str(dataset))
        summary = {}
    else:
        summaries = list(dataset.glob("CORE0B_PRIMARY_SUMMARY.json"))
        if len(summaries) != 1:
            finding("SUMMARY_CARDINALITY", str(len(summaries)))
            summary = {}
        else:
            summary = load(summaries[0])

    primary_dirs = sorted(
        p.name for p in (ROOT / "results").glob("core0b_primary_*") if p.is_dir()
    )
    if primary_dirs != [DATASET_NAME]:
        finding("FIRST_COLLECTION_RULE", repr(primary_dirs))

    head = git("rev-parse", "HEAD")
    for ancestor, fid in [
        (ARC_PARENT, "CANONICAL_PARENT_ANCESTRY"),
        (IMPLEMENTATION_HEAD, "IMPLEMENTATION_ANCESTRY"),
        (DATASET_COMMIT, "DATASET_COMMIT_ANCESTRY"),
    ]:
        rc = subprocess.run(
            ["git", "-C", str(ROOT), "merge-base", "--is-ancestor", ancestor, head]
        ).returncode
        if rc != 0:
            finding(fid, f"{ancestor} !<= {head}")

    auth_path = ROOT / "config" / "core0b_science_authorization.json"
    auth = load(auth_path) if auth_path.exists() else {}
    if auth.get("authorized") is not True:
        finding("AUTHORIZATION", "authorized != true")
    if auth.get("primary_requests_authorized") != 40:
        finding("AUTHORIZATION_COUNT", repr(auth.get("primary_requests_authorized")))
    if auth.get("measurement_authority") != "UNINSTRUMENTED_CHILD_WALL_MS":
        finding("AUTHORITY", repr(auth.get("measurement_authority")))
    if auth.get("implementation_head") != IMPLEMENTATION_HEAD:
        finding("AUTH_IMPLEMENTATION_HEAD", repr(auth.get("implementation_head")))
    if any(auth.get("instrumentation", {}).get(k) is not False for k in [
        "token_xray", "hardware_counters", "profiler", "resource_sampler"
    ]):
        finding("AUTH_INSTRUMENTATION", repr(auth.get("instrumentation")))

    expected_top = {
        "schema": "arcllm.core0b.primary_summary.v0.1",
        "classification": "CORE0B_MATCHED_REQUEST_CHARACTERIZATION_COMPLETE",
        "measurement_mode": "UNINSTRUMENTED",
        "timing_authority": "BENCHMARK_AUTHORITY",
        "measurement_authority": "UNINSTRUMENTED_CHILD_WALL_MS",
        "primary_requests_expected": 40,
        "primary_requests_observed": 40,
    }
    for k, v in expected_top.items():
        if summary.get(k) != v:
            finding("SUMMARY_FIELD", f"{k}={summary.get(k)!r} expected {v!r}")
    for k in ["token_xray_used", "hardware_counters_used", "profiler_used"]:
        if summary.get(k) is not False:
            finding("SUMMARY_INSTRUMENTATION", f"{k}={summary.get(k)!r}")
    if summary.get("mechanism_selection_authorized") is not False:
        finding("MECHANISM_BOUNDARY", repr(summary.get("mechanism_selection_authorized")))

    pairs = summary.get("pairs", [])
    if len(pairs) != 20:
        finding("PAIR_COUNT", str(len(pairs)))

    expected_names: set[str] = set()
    seen_names: set[str] = set()
    pair_map: dict[tuple[str, int], dict] = {}
    timing_rows: dict[str, list[float]] = {c[0]: [] for c in CELLS}
    arm_hashes: dict[str, dict[str, set[str]]] = {
        "arc": {"W-S": set(), "W-C": set()},
        "llama": {"W-S": set(), "W-C": set()},
    }

    for row in pairs:
        cell = row.get("cell")
        pair = row.get("pair")
        if (cell, pair) in pair_map:
            finding("DUP_PAIR", f"{cell}/{pair}")
        pair_map[(cell, pair)] = row

    for cell, session, workload in CELLS:
        for pidx in range(5):
            row = pair_map.get((cell, pidx))
            if row is None:
                finding("PAIR_MISSING", f"{cell}/{pidx}")
                continue
            if row.get("session") != session or row.get("workload") != workload:
                finding("PAIR_IDENTITY", f"{cell}/{pidx}")
            expected_arc_first = ((pidx + (1 if session == "B" else 0)) % 2) == 0
            expected_order = ["arc", "llama"] if expected_arc_first else ["llama", "arc"]
            if row.get("order") != expected_order:
                finding("PAIR_ORDER", f"{cell}/{pidx}: {row.get('order')} != {expected_order}")
            if row.get("valid") is not True:
                finding("PAIR_VALID", f"{cell}/{pidx}")

            for arm in ["arc", "llama"]:
                a = row.get(arm, {})
                name = f"{cell}_p{pidx}_{arm}.json"
                expected_names.add(name)
                seen_names.add(a.get("result_file"))
                if a.get("result_file") != name:
                    finding("RESULT_FILENAME", f"{cell}/{pidx}/{arm}: {a.get('result_file')}")
                if a.get("returncode") != 0 or a.get("valid") is not True or a.get("errors") != []:
                    finding("ARM_STATUS", f"{cell}/{pidx}/{arm}: {a}")
                if not isinstance(a.get("wall_ms"), (int, float)) or a.get("wall_ms") <= 0:
                    finding("WALL_MS", f"{cell}/{pidx}/{arm}: {a.get('wall_ms')}")

                raw_path = dataset / name
                if not raw_path.exists():
                    finding("RAW_MISSING", name)
                    continue
                raw = load(raw_path)
                gen = raw.get("generated_token_ids", [])
                if len(gen) != 32:
                    finding("GENERATED_COUNT", f"{name}: {len(gen)}")
                tok_digest = hashlib.sha256(
                    json.dumps(gen, separators=(",", ":")).encode("utf-8")
                ).hexdigest()
                arm_hashes[arm][workload].add(tok_digest)

                if arm == "arc":
                    if raw.get("schema") != "arcllm.v1.runtime.result.v0.1":
                        finding("ARC_SCHEMA", name)
                    s = raw.get("stats", {})
                    for k, v in ARC_STATS.items():
                        if s.get(k) != v:
                            finding("ARC_INVARIANT", f"{name}: {k}={s.get(k)!r}")
                    if s.get("finite") is not True:
                        finding("ARC_FINITE", name)
                else:
                    if raw.get("schema") != "arcllm.core0b.llama_cold_request.v0.1":
                        finding("LLAMA_SCHEMA", name)
                    if raw.get("system") != "llama.cpp" or raw.get("baseline_commit") != LLAMA_COMMIT:
                        finding("LLAMA_IDENTITY", name)
                    if raw.get("workload") != workload:
                        finding("LLAMA_WORKLOAD", name)
                    if raw.get("prompt_hash_fnv1a64") != PROMPT_HASH[workload]:
                        finding("LLAMA_PROMPT_HASH", name)
                    if raw.get("qualify_only") is not False or raw.get("success") is not True:
                        finding("LLAMA_SUCCESS", name)
                    if raw.get("final_logits_finite") is not True:
                        finding("LLAMA_FINITE", name)
                    r = raw.get("resolved", {})
                    for k, v in LLAMA_RESOLVED.items():
                        if r.get(k) != v:
                            finding("LLAMA_SETTING", f"{name}: {k}={r.get(k)!r}")
                    rt = raw.get("runtime", {})
                    if rt.get("vulkan_log_present") is not True:
                        finding("LLAMA_VULKAN", name)
                    if rt.get("full_offload") is not True or rt.get("offloaded_layers") != 29 or rt.get("offloaded_layers_total") != 29:
                        finding("LLAMA_OFFLOAD", f"{name}: {rt}")

            arc_ms = float(row["arc"]["wall_ms"])
            llama_ms = float(row["llama"]["wall_ms"])
            recomputed_ratio = arc_ms / llama_ms
            if not approx(recomputed_ratio, row.get("request_wall_ratio_arc_over_llama")):
                finding("PAIR_RATIO_MATH", f"{cell}/{pidx}")
            timing_rows[cell].append(recomputed_ratio)

    actual_json = {
        p.name for p in dataset.glob("*.json") if p.name != "CORE0B_PRIMARY_SUMMARY.json"
    }
    if actual_json != expected_names:
        finding(
            "RAW_FILE_SET",
            f"missing={sorted(expected_names-actual_json)} extra={sorted(actual_json-expected_names)}",
        )
    if seen_names != expected_names:
        finding("SUMMARY_RESULT_FILE_SET", f"seen={len(seen_names)} expected={len(expected_names)}")

    recomputed_cells: dict[str, dict] = {}
    cell_medians: list[float] = []
    for cell, session, workload in CELLS:
        xs = timing_rows[cell]
        if len(xs) != 5:
            finding("CELL_TIMING_COUNT", f"{cell}: {len(xs)}")
            continue
        r = calc(xs)
        recomputed_cells[cell] = r
        cell_medians.append(r["median"])
        reported = summary.get("cells", {}).get(cell, {})
        if reported.get("pairs_recorded") != 5 or reported.get("valid_pairs") != 5:
            finding("CELL_VALIDITY", f"{cell}: {reported}")
        rr = reported.get("request_wall_ratio_arc_over_llama", {})
        for k in ["n", "median", "min", "max", "mad"]:
            if k == "n":
                if rr.get(k) != r[k]:
                    finding("CELL_STAT_MATH", f"{cell}/{k}: {rr.get(k)} != {r[k]}")
            elif not approx(rr.get(k), r[k]):
                finding("CELL_STAT_MATH", f"{cell}/{k}: {rr.get(k)} != {r[k]}")

    if len(cell_medians) == 4:
        global_geo = math.exp(sum(math.log(x) for x in cell_medians) / 4.0)
    else:
        global_geo = None
    reported_geo = summary.get("global_geometric_mean_of_cell_medians_arc_over_llama")
    if global_geo is None or not approx(global_geo, reported_geo):
        finding("GLOBAL_GEOMEAN_MATH", f"{reported_geo} != {global_geo}")

    # Diagnostic consistency only; this was not an outcome gate.
    repeat_consistency = {
        arm: {workload: len(hashes) == 1 for workload, hashes in ws.items()}
        for arm, ws in arm_hashes.items()
    }
    notes.append({
        "id": "REPEAT_OUTPUT_CONSISTENCY_DIAGNOSTIC",
        "detail": repeat_consistency,
        "gate": False,
    })

    # Freeze a simple manifest over the exact dataset files after validating them.
    manifest = {}
    for p in sorted(dataset.glob("*.json")):
        manifest[p.name] = sha256(p)

    verdict = (
        "PASS_CORE0B_MATCHED_REQUEST_CHARACTERIZATION_COMPLETE"
        if not findings
        else "STOP_CORE0B_PRIMARY_ADJUDICATION"
    )
    result = {
        "schema": "arcllm.core0b.primary_independent_adjudication.v0.1",
        "verdict": verdict,
        "open_findings": len(findings),
        "findings": findings,
        "notes": notes,
        "dataset_name": DATASET_NAME,
        "dataset_commit": DATASET_COMMIT,
        "checked_head": head,
        "primary_requests": 40,
        "matched_pairs": 20,
        "valid_pairs_by_cell": {
            c: summary.get("cells", {}).get(c, {}).get("valid_pairs") for c, _, _ in CELLS
        },
        "recomputed_cell_ratio_stats": recomputed_cells,
        "recomputed_global_geometric_mean_of_cell_medians_arc_over_llama": global_geo,
        "reported_global_geometric_mean_of_cell_medians_arc_over_llama": reported_geo,
        "measurement_authority": "UNINSTRUMENTED_CHILD_WALL_MS",
        "token_xray_used": False,
        "hardware_counters_used": False,
        "profiler_used": False,
        "mechanism_selected": False,
        "dataset_sha256_manifest": manifest,
        "next_if_pass": "FORMAL_CLOSE_CORE0B_AND_OPEN_CORE0C_PREREGISTRATION_ONLY",
    }
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"CORE0B_PRIMARY_ADJUDICATION={verdict}")
    print(f"OPEN_FINDINGS={len(findings)}")
    if global_geo is not None:
        print(f"GLOBAL_GEOMEAN_ARC_OVER_LLAMA={global_geo:.12f}")
    for c, s in recomputed_cells.items():
        print(f"{c}_MEDIAN={s['median']:.12f} N={s['n']}")
    for f in findings:
        print(f"FINDING {f['id']}: {f['detail']}")
    return 0 if not findings else 3


if __name__ == "__main__":
    raise SystemExit(main())
