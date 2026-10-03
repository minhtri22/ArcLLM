#!/usr/bin/env python3
"""Windows BuildOnly/static-preflight orchestrator for ARCLLM_LMAX_ARCH_P0.

This launcher is intentionally zero-science: it performs static QA, generates the
reversible observer-instrumented source, builds binaries, runs only the no-model
ring invariant selftest and the runner's --describe / authorization guard paths,
then writes hash/provenance evidence. It never loads the model or initializes
Vulkan.
"""

from __future__ import annotations

import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys


def run(argv, *, cwd: pathlib.Path, env=None, check=True):
    cp = subprocess.run(
        [str(x) for x in argv],
        cwd=str(cwd),
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        shell=False,
    )
    if check and cp.returncode != 0:
        raise RuntimeError(
            f"command failed rc={cp.returncode}: {argv}\n{cp.stdout}"
        )
    return cp


def sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def git_blob(root: pathlib.Path, rel: str) -> str:
    return run(["git", "hash-object", rel], cwd=root).stdout.strip()


def vc_environment() -> tuple[dict[str, str], str]:
    pf86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = pathlib.Path(pf86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.is_file():
        raise RuntimeError(f"vswhere.exe not found: {vswhere}")

    cp = subprocess.run(
        [
            str(vswhere), "-latest", "-products", "*",
            "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property", "installationPath",
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        shell=False,
    )
    if cp.returncode != 0 or not cp.stdout.strip():
        raise RuntimeError(f"Visual Studio Build Tools not found\n{cp.stdout}")

    install = pathlib.Path(cp.stdout.strip())
    devcmd = install / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    if not devcmd.is_file():
        raise RuntimeError(f"vcvars64.bat not found: {devcmd}")

    env_cp = subprocess.run(
        f'call "{devcmd}" >nul && set',
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        shell=True,
    )
    if env_cp.returncode != 0:
        raise RuntimeError(f"vcvars64 environment capture failed\n{env_cp.stdout}")

    env = dict(os.environ)
    for line in env_cp.stdout.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            env[k] = v
    return env, str(install)


def main() -> int:
    here = pathlib.Path(__file__).resolve().parent
    root = here.parents[1]
    evidence_dir = root / "results" / "arcllm_lmax_arch_p0_preflight"
    build_dir = evidence_dir / "build"
    evidence_dir.mkdir(parents=True, exist_ok=True)
    build_dir.mkdir(parents=True, exist_ok=True)

    static_qa = here / "static_qa.py"
    generator = here / "generate_instrumented_runtime.py"
    generated = build_dir / "arcllm_v1_runtime_p0_instrumented.cpp"
    transform_manifest = evidence_dir / "RUNTIME_TRANSFORM.json"
    selftest_exe = build_dir / "arcllm_lmax_p0_ring_selftest.exe"
    runner_exe = build_dir / "arcllm_lmax_p0_runner.exe"
    describe_out = evidence_dir / "RUNNER_DESCRIBE.txt"
    guard_out = evidence_dir / "RUNNER_AUTH_GUARD.txt"
    evidence_out = evidence_dir / "BUILDONLY_EVIDENCE.json"

    qa = run([sys.executable, static_qa], cwd=root)
    sys.stdout.write(qa.stdout)

    gen = run(
        [
            sys.executable, generator,
            "--root", root,
            "--out", generated,
            "--manifest", transform_manifest,
        ],
        cwd=root,
    )
    sys.stdout.write(gen.stdout)

    vc_env, vs_install = vc_environment()

    self_src = here / "p0_ring_selftest.cpp"
    self_cmd = [
        "cl.exe", "/nologo", "/std:c++17", "/O2", "/EHsc", "/W4", "/Brepro",
        f"/I{here}",
        f"/Fe:{selftest_exe}",
        str(self_src),
    ]
    self_build = run(self_cmd, cwd=build_dir, env=vc_env)
    sys.stdout.write(self_build.stdout)
    if not selftest_exe.is_file():
        raise RuntimeError("ring selftest executable missing after build")

    self_run = run([selftest_exe], cwd=build_dir, env=vc_env)
    sys.stdout.write(self_run.stdout)
    if "ARCLLM_LMAX_P0_RING_SELFTEST=PASS" not in self_run.stdout:
        raise RuntimeError("ring invariant selftest did not report PASS")

    sources = [
        generated,
        here / "p0_runner.cpp",
        root / "src" / "gguf.cpp",
        root / "src" / "tensor_store.cpp",
        root / "src" / "arcllm_v1_primitive_registry_v2.cpp",
        root / "src" / "arcllm_v1_generic_policy_engine_v4.cpp",
        root / "src" / "arcllm_v1_generic_backend_binding_v4.cpp",
        root / "src" / "registrations" / "arcllm_v1_q4k_down_reference_registration_v2.cpp",
    ]
    runner_cmd = [
        "cl.exe", "/nologo", "/std:c++17", "/O2", "/EHsc", "/W4", "/bigobj", "/Brepro",
        f"/I{root / 'src'}",
        f"/I{root / 'include'}",
        f"/I{here}",
        f"/Fe:{runner_exe}",
        *[str(p) for p in sources],
    ]
    runner_build = run(runner_cmd, cwd=build_dir, env=vc_env)
    sys.stdout.write(runner_build.stdout)
    if not runner_exe.is_file():
        raise RuntimeError("matched-arm runner executable missing after BuildOnly")

    describe = run([runner_exe, "--describe"], cwd=build_dir, env=vc_env)
    describe_out.write_text(describe.stdout, encoding="utf-8")
    if (
        "MODEL_EXECUTED=false" not in describe.stdout
        or "VULKAN_INITIALIZED=false" not in describe.stdout
    ):
        raise RuntimeError("BuildOnly describe path did not prove no-model/no-Vulkan")

    guard = run([runner_exe], cwd=build_dir, env=vc_env, check=False)
    guard_out.write_text(guard.stdout, encoding="utf-8")
    if guard.returncode != 2 or "outcome execution blocked" not in guard.stdout:
        raise RuntimeError(
            f"execution guard did not fail closed rc={guard.returncode}\n{guard.stdout}"
        )

    compiler = run(["cl.exe", "/Bv"], cwd=build_dir, env=vc_env, check=False)

    tracked = [
        "experiments/arcllm_lmax_arch_p0/p0_ring.h",
        "experiments/arcllm_lmax_arch_p0/p0_ring_selftest.cpp",
        "experiments/arcllm_lmax_arch_p0/p0_runner.cpp",
        "experiments/arcllm_lmax_arch_p0/generate_instrumented_runtime.py",
        "experiments/arcllm_lmax_arch_p0/static_qa.py",
        "experiments/arcllm_lmax_arch_p0/buildonly.py",
        "config/arcllm_lmax_arch_p0_preregistration_v0.1.json",
        "docs/research/arcllm-v1/ARCLLM_LMAX_ARCH_P0_PREREGISTRATION.md",
    ]
    source_sha256 = {rel: sha256(root / rel) for rel in tracked}
    source_git_blobs = {rel: git_blob(root, rel) for rel in tracked}

    evidence = {
        "schema": "arcllm.lmax_arch_p0.buildonly_evidence.v0.1",
        "status": "PASS_IMPLEMENTATION_STATIC_PREFLIGHT_BUILDONLY",
        "git_head": run(["git", "rev-parse", "HEAD"], cwd=root).stdout.strip(),
        "canonical_runtime_git_blob": git_blob(root, "src/arcllm_v1_runtime.cpp"),
        "canonical_api_git_blob": git_blob(root, "include/arcllm/v1/runtime.h"),
        "source_sha256": source_sha256,
        "source_git_blobs": source_git_blobs,
        "generated_runtime_sha256": sha256(generated),
        "transform_manifest_sha256": sha256(transform_manifest),
        "ring_selftest_exe_sha256": sha256(selftest_exe),
        "runner_exe_sha256": sha256(runner_exe),
        "runner_exe_bytes": runner_exe.stat().st_size,
        "visual_studio_installation": vs_install,
        "compiler_info": compiler.stdout.strip(),
        "static_qa": "PASS",
        "reversible_runtime_transform": "PASS",
        "ring_ordering": "PASS",
        "forced_backpressure_invariant": "PASS",
        "ring_steady_state_allocation_count": 0,
        "runner_authorization_guard": "PASS_FAIL_CLOSED",
        "model_executed": False,
        "vulkan_initialized": False,
        "shader_execution": False,
        "outcome_performance_measured": False,
        "measured_inference_runs": 0,
        "preregistered_control_path_performance_subtest_executed": False,
        "lineage_modified": False,
    }
    evidence_out.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")

    for obj in build_dir.glob("*.obj"):
        try:
            obj.unlink()
        except OSError:
            pass

    print("ARCLLM_LMAX_ARCH_P0_BUILDONLY=PASS")
    print(f"RUNNER_SHA256={evidence['runner_exe_sha256']}")
    print(f"SELFTEST_SHA256={evidence['ring_selftest_exe_sha256']}")
    print("MODEL_EXECUTED=false")
    print("MEASURED_INFERENCE_RUNS=0")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ARCLLM_LMAX_ARCH_P0_BUILDONLY=FAIL: {exc}", file=sys.stderr)
        raise
