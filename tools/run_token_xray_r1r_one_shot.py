from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import pathlib
import statistics
import subprocess
import sys
import tempfile
from ctypes import wintypes
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "config" / "token_xray_r1r_execution_contract_v0.1.json"


def load(path: pathlib.Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256_file(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda: f.read(1024 * 1024), b""):
            h.update(b)
    return h.hexdigest().upper()


def atomic_json(path: pathlib.Path, obj: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(obj, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    os.replace(tmp, path)


def run(argv: list[str], cwd: pathlib.Path | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    cp = subprocess.run(argv, cwd=str(cwd) if cwd else None, text=True, capture_output=True)
    if check and cp.returncode != 0:
        raise RuntimeError(
            "command failed rc=%d argv=%r\nstdout=%s\nstderr=%s"
            % (cp.returncode, argv, cp.stdout[-8000:], cp.stderr[-8000:])
        )
    return cp


def git(args: list[str], cwd: pathlib.Path) -> str:
    return run(["git", "-C", str(cwd), *args]).stdout.strip()


def verify_git_identity(root: pathlib.Path, freeze_ref: str, expected_head: str | None = None) -> str:
    head = git(["rev-parse", "HEAD"], root)
    ref_head = git(["rev-parse", freeze_ref], root)
    if head != ref_head:
        raise RuntimeError(f"HEAD {head} != frozen ref {freeze_ref}@{ref_head}")
    if expected_head and head != expected_head:
        raise RuntimeError(f"HEAD {head} != expected locked head {expected_head}")
    dirty = git(["status", "--porcelain", "--untracked-files=no"], root)
    if dirty:
        raise RuntimeError("tracked worktree dirty: " + dirty)
    return head


def resolve_model(contract: dict[str, Any], explicit: str | None) -> pathlib.Path:
    if explicit:
        p = pathlib.Path(explicit)
    else:
        cfg = load(ROOT / "config" / "p8_target.json")
        ollama_root = pathlib.Path(os.environ.get("OLLAMA_MODELS") or (pathlib.Path.home() / ".ollama" / "models"))
        p = ollama_root / str(cfg["blob_relative"]).replace("/", os.sep)
    if not p.is_file():
        raise RuntimeError(f"model missing: {p}")
    if p.stat().st_size != int(contract["model"]["size_bytes"]):
        raise RuntimeError("model size mismatch")
    observed = sha256_file(p)
    if observed.upper() != str(contract["model"]["sha256"]).upper():
        raise RuntimeError("model SHA256 mismatch")
    return p


def job_object_status() -> dict[str, Any]:
    if os.name != "nt":
        return {"platform": os.name, "in_job": False, "memory_limit_flags": False}

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    GetCurrentProcess = kernel32.GetCurrentProcess
    GetCurrentProcess.restype = wintypes.HANDLE
    IsProcessInJob = kernel32.IsProcessInJob
    IsProcessInJob.argtypes = [wintypes.HANDLE, wintypes.HANDLE, ctypes.POINTER(wintypes.BOOL)]
    IsProcessInJob.restype = wintypes.BOOL
    QueryInformationJobObject = kernel32.QueryInformationJobObject
    QueryInformationJobObject.argtypes = [wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]
    QueryInformationJobObject.restype = wintypes.BOOL

    class IO_COUNTERS(ctypes.Structure):
        _fields_ = [
            ("ReadOperationCount", ctypes.c_ulonglong), ("WriteOperationCount", ctypes.c_ulonglong),
            ("OtherOperationCount", ctypes.c_ulonglong), ("ReadTransferCount", ctypes.c_ulonglong),
            ("WriteTransferCount", ctypes.c_ulonglong), ("OtherTransferCount", ctypes.c_ulonglong),
        ]

    class BASIC_LIMIT(ctypes.Structure):
        _fields_ = [
            ("PerProcessUserTimeLimit", ctypes.c_longlong), ("PerJobUserTimeLimit", ctypes.c_longlong),
            ("LimitFlags", wintypes.DWORD), ("MinimumWorkingSetSize", ctypes.c_size_t),
            ("MaximumWorkingSetSize", ctypes.c_size_t), ("ActiveProcessLimit", wintypes.DWORD),
            ("Affinity", ctypes.c_size_t), ("PriorityClass", wintypes.DWORD), ("SchedulingClass", wintypes.DWORD),
        ]

    class EXT_LIMIT(ctypes.Structure):
        _fields_ = [
            ("BasicLimitInformation", BASIC_LIMIT), ("IoInfo", IO_COUNTERS),
            ("ProcessMemoryLimit", ctypes.c_size_t), ("JobMemoryLimit", ctypes.c_size_t),
            ("PeakProcessMemoryUsed", ctypes.c_size_t), ("PeakJobMemoryUsed", ctypes.c_size_t),
        ]

    in_job = wintypes.BOOL(False)
    if not IsProcessInJob(GetCurrentProcess(), None, ctypes.byref(in_job)):
        raise OSError(ctypes.get_last_error())

    out: dict[str, Any] = {"platform": "windows", "in_job": bool(in_job.value)}
    if in_job.value:
        info = EXT_LIMIT()
        returned = wintypes.DWORD()
        if not QueryInformationJobObject(None, 9, ctypes.byref(info), ctypes.sizeof(info), ctypes.byref(returned)):
            raise OSError(ctypes.get_last_error())
        flags = int(info.BasicLimitInformation.LimitFlags)
        out["limit_flags"] = flags
        out["process_memory_limit"] = int(info.ProcessMemoryLimit)
        out["job_memory_limit"] = int(info.JobMemoryLimit)
        out["process_memory_limited"] = bool(flags & 0x100)
        out["job_memory_limited"] = bool(flags & 0x200)
        out["working_set_limited"] = bool(flags & 0x1)
        out["memory_limit_flags"] = bool(flags & (0x100 | 0x200 | 0x1))
    else:
        out["memory_limit_flags"] = False
    return out


def verify_assets(contract: dict[str, Any]) -> dict[str, Any]:
    lock = contract["execution_lock"]
    orch = ROOT / "tools" / "run_token_xray_r1r_one_shot.py"
    harness = ROOT / "token_xray_r1_capture_harness.exe"
    tok = ROOT / "token_xray_r1r_tokenizer_probe.exe"
    capture_shader = ROOT / "compiled_shaders" / "token_xray_r1_capture_row.spv"
    for p in [orch, harness, tok, capture_shader]:
        if not p.is_file():
            raise RuntimeError(f"locked execution asset missing: {p}")

    observed = {
        "orchestrator_sha256": sha256_file(orch),
        "harness_sha256": sha256_file(harness),
        "tokenizer_probe_sha256": sha256_file(tok),
        "capture_shader_sha256": sha256_file(capture_shader),
    }
    expected_map = {
        "orchestrator_sha256": "expected_orchestrator_sha256",
        "harness_sha256": "expected_harness_sha256",
        "tokenizer_probe_sha256": "expected_tokenizer_probe_sha256",
        "capture_shader_sha256": "expected_capture_shader_sha256",
    }
    for observed_key, lock_key in expected_map.items():
        expected = lock.get(lock_key)
        if not expected or observed[observed_key].upper() != str(expected).upper():
            raise RuntimeError(f"locked asset mismatch: {observed_key}")

    inventory = []
    for p in sorted((ROOT / "compiled_shaders").glob("*.spv"), key=lambda x: x.name.lower()):
        inventory.append({"name": p.name, "sha256": sha256_file(p), "bytes": p.stat().st_size})
    expected_inventory = lock.get("expected_shader_inventory")
    if not isinstance(expected_inventory, list) or inventory != expected_inventory:
        raise RuntimeError("compiled shader inventory mismatch")
    observed["compiled_shaders"] = inventory
    return observed


def run_tokenizer_gate(contract: dict[str, Any], model: pathlib.Path, out_path: pathlib.Path) -> dict[str, Any]:
    probe = ROOT / "token_xray_r1r_tokenizer_probe.exe"
    argv = [str(probe), "--model", str(model), "--out", str(out_path)]
    for pid in ["P0", "P1", "P2", "P3"]:
        argv += ["--prompt", f"{pid}={contract['prompts'][pid]['utf8_hex']}"]
    run(argv, ROOT)

    raw = load(out_path)
    gate_rows: dict[str, Any] = {}
    all_match = True
    for pid in ["P0", "P1", "P2", "P3"]:
        frozen = contract["prompts"][pid]
        observed = raw["prompts"][pid]
        byte_seq = bytes.fromhex(str(frozen["utf8_hex"]))
        expected_sha = str(frozen["utf8_sha256"]).lower()
        observed_sha = hashlib.sha256(byte_seq).hexdigest()
        # Strict UTF-8 decode is a guard only; tokenizer consumes the exact bytes.
        decoded = byte_seq.decode("utf-8", errors="strict")
        expected_ids = [int(x) for x in frozen["token_ids"]]
        observed_ids = [int(x) for x in observed["token_ids"]]
        row_match = (
            str(observed["utf8_hex"]).lower() == str(frozen["utf8_hex"]).lower()
            and int(observed["byte_count"]) == len(byte_seq)
            and observed_sha == expected_sha
            and observed_ids == expected_ids
        )
        gate_rows[pid] = {
            "utf8_hex": frozen["utf8_hex"],
            "utf8_sha256": observed_sha,
            "byte_count": len(byte_seq),
            "decoded_utf8": decoded,
            "expected_token_ids": expected_ids,
            "observed_token_ids": observed_ids,
            "token_count": len(observed_ids),
            "match": row_match,
        }
        all_match = all_match and row_match

    gate = {
        "schema": "arcllm.token_xray_r1r.exact_gguf_gate.v0.1",
        "status": "PASS_EXACT_GGUF_TOKENIZATION" if all_match else "FAIL_EXACT_GGUF_TOKENIZATION",
        "model_sha256": sha256_file(model),
        "prompt_source": "frozen_hex_decoded_bytes",
        "prompts": gate_rows,
        "model_inference_executed": False,
    }
    atomic_json(out_path, gate)
    if not all_match:
        raise RuntimeError("STOP_BEFORE_MODEL_EXECUTION: exact-GGUF tokenization mismatch")
    return gate


def same_ids(a: Any, b: Any) -> bool:
    return [int(x) for x in a] == [int(x) for x in b]


def raw_hashes(raw_dir: pathlib.Path) -> list[dict[str, Any]]:
    return [
        {"file": p.name, "sha256": sha256_file(p), "bytes": p.stat().st_size}
        for p in sorted(raw_dir.glob("*.json"), key=lambda x: x.name)
    ]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--token-xray-root", required=True)
    ap.add_argument("--model-path")
    ap.add_argument("--mode", choices=["route-preflight", "science"], required=True)
    args = ap.parse_args()

    contract = load(CONTRACT_PATH)
    lock = contract["execution_lock"]
    tx_root = pathlib.Path(args.token_xray_root).resolve()

    arc_head = verify_git_identity(ROOT, str(lock["arcllm_freeze_ref"]))
    tx_head = verify_git_identity(tx_root, str(lock["token_xray_freeze_ref"]), str(lock["token_xray_head"]))

    model = resolve_model(contract, args.model_path)
    job = job_object_status()
    if job.get("memory_limit_flags"):
        raise RuntimeError("managed route has effective process/job/working-set memory limit")

    assets = verify_assets(contract)

    results_root = ROOT / "results"
    preflight_dir = results_root / "token_xray_r1r_route_preflight_v0.1"
    science_dir = ROOT / pathlib.Path(str(lock["canonical_namespace"]).replace("/", os.sep))

    if args.mode == "route-preflight":
        if science_dir.exists():
            raise RuntimeError("R1R science namespace must be absent during route preflight")
        preflight_dir.mkdir(parents=True, exist_ok=True)
        gate_path = preflight_dir / "R1R_EXACT_GGUF_TOKENIZATION.json"
        gate = run_tokenizer_gate(contract, model, gate_path)
        result = {
            "schema": "arcllm.token_xray_r1r.route_preflight.v0.1",
            "status": "PASS_R1R_REMOTE_MCP_PYTHON_ROUTE_PREFLIGHT",
            "route": contract["canonical_execution_route"],
            "arcllm_head": arc_head,
            "token_xray_head": tx_head,
            "model_sha256": sha256_file(model),
            "job_object": job,
            "assets": assets,
            "tokenizer_gate": gate,
            "model_execution_started": False,
            "science_namespace_exists": False,
        }
        atomic_json(preflight_dir / "ROUTE_PREFLIGHT.json", result)
        print("TOKEN_XRAY_R1R_ROUTE_PREFLIGHT=PASS")
        return 0

    if science_dir.exists():
        raise RuntimeError("R1R fresh namespace already exists; rerun forbidden")

    with tempfile.TemporaryDirectory(prefix="r1r_gate_") as td:
        gate_tmp = pathlib.Path(td) / "gate.json"
        gate = run_tokenizer_gate(contract, model, gate_tmp)

    preflight = science_dir / "preflight"
    raw_dir = science_dir / "raw"
    preflight.mkdir(parents=True, exist_ok=False)
    raw_dir.mkdir(parents=True, exist_ok=False)
    atomic_json(preflight / "R1R_EXACT_GGUF_TOKENIZATION.json", gate)

    state_path = science_dir / "EXECUTION_STATE.json"
    state: dict[str, Any] = {
        "schema": "token_xray.r1r.one_shot_state.v0.1",
        "status": "PREFLIGHT_COMPLETE_NOT_STARTED",
        "arcllm_head": arc_head,
        "arcllm_freeze_ref": lock["arcllm_freeze_ref"],
        "token_xray_head": tx_head,
        "token_xray_freeze_ref": lock["token_xray_freeze_ref"],
        "model_sha256": sha256_file(model),
        "model_size_bytes": model.stat().st_size,
        "exact_gguf_tokenization": "PASS",
        "canonical_route": contract["canonical_execution_route"],
        "job_object": job,
        "execution_lock": assets,
        "pairs": [
            {"pair": int(p["pair"]), "prompt": str(p["prompt"]), "order": list(p["order"]), "members": []}
            for p in contract["matched_pairs"]
        ],
        "current_pair": None,
        "current_member": None,
        "model_execution_started": False,
        "pairs_consumed": 0,
    }
    atomic_json(state_path, state)

    harness = ROOT / "token_xray_r1_capture_harness.exe"
    shader_dir = ROOT / "compiled_shaders"
    fail_reason: str | None = None
    raw_candidate = "PASS_CANDIDATE_PENDING_INDEPENDENT_QA"

    for pair in contract["matched_pairs"]:
        pair_num = int(pair["pair"])
        pid = str(pair["prompt"])
        pair_state = next(x for x in state["pairs"] if int(x["pair"]) == pair_num)
        first_member = True
        for member in pair["order"]:
            mode = str(member)
            state["status"] = "RUNNING"
            state["current_pair"] = pair_num
            state["current_member"] = mode
            if not state["model_execution_started"]:
                state["model_execution_started"] = True
            if first_member:
                state["pairs_consumed"] = int(state["pairs_consumed"]) + 1
                first_member = False
            pair_state["members"].append({"mode": mode, "status": "STARTED"})
            atomic_json(state_path, state)

            # Revalidate immutable execution assets immediately before every model member.
            verify_assets(contract)

            out_file = raw_dir / f"pair{pair_num:02d}_{mode}.json"
            tokens = ",".join(str(int(x)) for x in contract["prompts"][pid]["token_ids"])
            cp = run(
                [
                    str(harness),
                    "--model", str(model),
                    "--shader-dir", str(shader_dir),
                    "--tokens", tokens,
                    "--mode", mode,
                    "--prompt-id", pid,
                    "--arcllm-head", arc_head,
                    "--model-sha256", str(contract["model"]["sha256"]),
                    "--out", str(out_file),
                ],
                ROOT,
                check=False,
            )
            if cp.returncode != 0:
                pair_state["members"][-1]["status"] = "RUNTIME_FAILURE_AFTER_START"
                state["status"] = "STOPPED_RUNTIME_FAILURE_AFTER_MODEL_START"
                atomic_json(state_path, state)
                raise RuntimeError("R1R runtime failure after model execution began; rerun forbidden")
            pair_state["members"][-1] = {"mode": mode, "status": "COMPLETE", "file": out_file.name}
            atomic_json(state_path, state)

        b = load(raw_dir / f"pair{pair_num:02d}_baseline.json")
        i = load(raw_dir / f"pair{pair_num:02d}_instrumented.json")
        if not same_ids(b["input_token_ids"], i["input_token_ids"]):
            fail_reason = f"prompt_token_mismatch_pair_{pair_num}"
        elif not same_ids(b["generated_token_ids"], i["generated_token_ids"]):
            fail_reason = f"generated_token_mismatch_pair_{pair_num}"
        elif not bool(b["runtime"]["finite"]) or not bool(i["runtime"]["finite"]):
            fail_reason = f"runtime_finite_flag_pair_{pair_num}"
        if fail_reason:
            raw_candidate = "FAIL_R1R_REAL_RUNTIME_CAPTURE_OR_EXECUTION_INTEGRITY"
            state["status"] = "RAW_FAIL_STOP"
            atomic_json(state_path, state)
            break

    validation_path = science_dir / "R1R_RAW_EXECUTION_VALIDATION.json"
    validator = tx_root / str(contract["token_xray_validator_freeze"]["tool"]).replace("/", os.sep)
    underlying_path = science_dir / "R1_UNDERLYING_CAPTURE_VALIDATION.json"
    cp = run(
        [sys.executable, str(validator), "--raw-dir", str(raw_dir), "--contract", str(CONTRACT_PATH), "--out", str(underlying_path)],
        tx_root,
        check=False,
    )
    underlying = load(underlying_path) if underlying_path.exists() else {"verdict": None, "errors": ["validator output missing"]}
    if cp.returncode != 0 or underlying.get("verdict") != "PASS_R1_REAL_RUNTIME_CAPTURE_SEMANTICS_PRESERVED":
        raw_candidate = "FAIL_R1R_REAL_RUNTIME_CAPTURE_OR_EXECUTION_INTEGRITY"
        fail_reason = fail_reason or "token_xray_capture_validation_failed"

    validation = {
        "schema": "token_xray.r1r.raw_execution_validation.v0.1",
        "raw_candidate": (
            "PASS_R1R_REAL_RUNTIME_CAPTURE_SEMANTICS_PRESERVED"
            if raw_candidate.startswith("PASS_")
            else "FAIL_R1R_REAL_RUNTIME_CAPTURE_OR_EXECUTION_INTEGRITY"
        ),
        "final_scientific_verdict": None,
        "underlying_r1_validator_verdict": underlying.get("verdict"),
        "underlying_r1_validator_errors": underlying.get("errors", []),
        "pair_checks": underlying.get("pair_checks", []),
        "derived_trajectory_count": underlying.get("derived_trajectory_count"),
        "observer_effect": underlying.get("observer_effect"),
        "raw_files": raw_hashes(raw_dir),
        "fail_reason": fail_reason,
        "boundaries": {
            "performance_authority": False,
            "semantic_interpretation": False,
            "causal_mechanism": False,
            "final_adjudication_open": True,
        },
    }
    atomic_json(validation_path, validation)

    state["current_pair"] = None
    state["current_member"] = None
    state["status"] = "ONE_SHOT_COMPLETE"
    state["raw_candidate"] = validation["raw_candidate"]
    state["fail_reason"] = fail_reason
    state["raw_validation_file"] = validation_path.name
    atomic_json(state_path, state)

    print("TOKEN_XRAY_R1R_ONE_SHOT_EXECUTION=COMPLETE")
    print("RAW_CANDIDATE=" + str(validation["raw_candidate"]))
    print("OUT_DIR=" + str(science_dir))
    return 0 if validation["raw_candidate"] == "PASS_R1R_REAL_RUNTIME_CAPTURE_SEMANTICS_PRESERVED" else 21


if __name__ == "__main__":
    raise SystemExit(main())
