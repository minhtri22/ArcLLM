from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import pathlib
import subprocess
from ctypes import wintypes
from typing import Any

ROOT = pathlib.Path(__file__).resolve().parents[1]


def sha256_file(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def run(argv: list[str], cwd: pathlib.Path | None = None) -> str:
    cp = subprocess.run(argv, cwd=str(cwd) if cwd else None, text=True, capture_output=True)
    if cp.returncode != 0:
        raise RuntimeError(
            "command failed rc=%d argv=%r\nstdout=%s\nstderr=%s"
            % (cp.returncode, argv, cp.stdout[-8000:], cp.stderr[-8000:])
        )
    return cp.stdout.strip()


def git(root: pathlib.Path, *args: str) -> str:
    return run(["git", "-C", str(root), *args])


def atomic_json(path: pathlib.Path, obj: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(obj, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    os.replace(tmp, path)


def job_object_status() -> dict[str, Any]:
    if os.name != "nt":
        return {"platform": os.name, "in_job": False, "memory_limit_flags": False}

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    get_current_process = kernel32.GetCurrentProcess
    get_current_process.restype = wintypes.HANDLE
    is_process_in_job = kernel32.IsProcessInJob
    is_process_in_job.argtypes = [wintypes.HANDLE, wintypes.HANDLE, ctypes.POINTER(wintypes.BOOL)]
    is_process_in_job.restype = wintypes.BOOL
    query_information_job_object = kernel32.QueryInformationJobObject
    query_information_job_object.argtypes = [
        wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)
    ]
    query_information_job_object.restype = wintypes.BOOL

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
    if not is_process_in_job(get_current_process(), None, ctypes.byref(in_job)):
        raise OSError(ctypes.get_last_error())

    out: dict[str, Any] = {"platform": "windows", "in_job": bool(in_job.value)}
    if not in_job.value:
        out["memory_limit_flags"] = False
        return out

    info = EXT_LIMIT()
    returned = wintypes.DWORD()
    if not query_information_job_object(None, 9, ctypes.byref(info), ctypes.sizeof(info), ctypes.byref(returned)):
        raise OSError(ctypes.get_last_error())
    flags = int(info.BasicLimitInformation.LimitFlags)
    out.update({
        "limit_flags": flags,
        "process_memory_limit": int(info.ProcessMemoryLimit),
        "job_memory_limit": int(info.JobMemoryLimit),
        "process_memory_limited": bool(flags & 0x100),
        "job_memory_limited": bool(flags & 0x200),
        "working_set_limited": bool(flags & 0x1),
        "memory_limit_flags": bool(flags & (0x100 | 0x200 | 0x1)),
    })
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--token-xray-root", required=True)
    ap.add_argument("--expected-implementation-head", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    tx_root = pathlib.Path(args.token_xray_root).resolve()
    head = git(ROOT, "rev-parse", "HEAD")
    if head != args.expected_implementation_head:
        raise RuntimeError(f"ArcLLM HEAD mismatch: {head}")
    if git(ROOT, "status", "--porcelain", "--untracked-files=no"):
        raise RuntimeError("ArcLLM tracked worktree dirty")

    tx_head = git(tx_root, "rev-parse", "HEAD")
    tx_ref = git(tx_root, "rev-parse", "freeze/representation-trajectory-r1-validator-v0.1")
    if tx_head != tx_ref or tx_head != "2e1680fa9187b166a8fb6c52a5c8677a86766c39":
        raise RuntimeError("Token-XRay validator identity mismatch")
    if git(tx_root, "status", "--porcelain", "--untracked-files=no"):
        raise RuntimeError("Token-XRay tracked worktree dirty")

    science_dir = ROOT / "results" / "token_xray_r1r_one_shot_lock_v0.1"
    if science_dir.exists():
        raise RuntimeError("R1R science namespace already exists")

    job = job_object_status()
    if job.get("memory_limit_flags"):
        raise RuntimeError("managed route has effective memory limits")

    assets = {
        "orchestrator": ROOT / "tools" / "run_token_xray_r1r_one_shot.py",
        "harness": ROOT / "token_xray_r1_capture_harness.exe",
        "tokenizer_probe": ROOT / "token_xray_r1r_tokenizer_probe.exe",
        "capture_shader": ROOT / "compiled_shaders" / "token_xray_r1_capture_row.spv",
    }
    for name, path in assets.items():
        if not path.is_file():
            raise RuntimeError(f"missing execution asset {name}: {path}")

    inventory = [
        {"name": p.name, "sha256": sha256_file(p), "bytes": p.stat().st_size}
        for p in sorted((ROOT / "compiled_shaders").glob("*.spv"), key=lambda x: x.name.lower())
    ]
    if not inventory:
        raise RuntimeError("compiled shader inventory empty")

    result = {
        "schema": "arcllm.token_xray_r1r.execution_lock_candidate.v0.1",
        "status": "PASS_R1R_LOCK_CANDIDATE_ZERO_SCIENCE",
        "science_execution": False,
        "model_inference": False,
        "arcllm_implementation_head": head,
        "token_xray_head": tx_head,
        "canonical_namespace": "results/token_xray_r1r_one_shot_lock_v0.1",
        "science_namespace_absent": True,
        "job_object": job,
        "hashes": {
            "orchestrator_sha256": sha256_file(assets["orchestrator"]),
            "harness_sha256": sha256_file(assets["harness"]),
            "tokenizer_probe_sha256": sha256_file(assets["tokenizer_probe"]),
            "capture_shader_sha256": sha256_file(assets["capture_shader"]),
            "compiled_shaders": inventory,
        },
    }
    out = pathlib.Path(args.out).resolve()
    atomic_json(out, result)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
