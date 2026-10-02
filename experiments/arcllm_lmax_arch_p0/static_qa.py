#!/usr/bin/env python3
"""Zero-science static QA for ARCLLM_LMAX_ARCH_P0 implementation."""

from __future__ import annotations

import importlib.util
import json
import pathlib
import subprocess
import sys

BASE_COMMIT = "c6c9b2e0a198b8af43b120e2b269e2ddd0429ef1"
EXPECTED_RUNTIME_BLOB = "0c613f6f740931a88ddd3ee5b904533a58001a06"
EXPECTED_API_BLOB = "d7821270ed253e1d1d96e3916b22b3daee50f5bc"

ALLOWED_PREFIXES = (
    "docs/research/arcllm-v1/ARCLLM_LMAX_ARCH_P0_",
    "config/arcllm_lmax_arch_p0_",
    "experiments/arcllm_lmax_arch_p0/",
    "results/arcllm_lmax_arch_p0_",
)


def run(root: pathlib.Path, *argv: str) -> str:
    return subprocess.check_output(
        list(argv), cwd=root, text=True, stderr=subprocess.STDOUT
    ).strip()


def git_blob(root: pathlib.Path, rel: str) -> str:
    return run(root, "git", "hash-object", rel)


def fail(msg: str) -> None:
    raise RuntimeError(msg)


def load_generator(path: pathlib.Path):
    spec = importlib.util.spec_from_file_location("p0_generator", path)
    if spec is None or spec.loader is None:
        fail("cannot load instrumentation generator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    root = pathlib.Path(__file__).resolve().parents[2]
    exp = root / "experiments" / "arcllm_lmax_arch_p0"

    prereg_path = root / "config" / "arcllm_lmax_arch_p0_preregistration_v0.1.json"
    prereg = json.loads(prereg_path.read_text(encoding="utf-8"))
    if prereg["status"] != "FROZEN_SPECIFICATION_ONLY":
        fail("preregistration status drift")
    if prereg["science_execution_authorized"] is not False:
        fail("science execution became authorized during implementation")
    if prereg["base_commit"] != BASE_COMMIT:
        fail("base commit drift")
    if prereg["runtime_source_blob"] != EXPECTED_RUNTIME_BLOB:
        fail("preregistered runtime blob drift")
    if prereg["runtime_api_blob"] != EXPECTED_API_BLOB:
        fail("preregistered API blob drift")

    runtime_blob = git_blob(root, "src/arcllm_v1_runtime.cpp")
    api_blob = git_blob(root, "include/arcllm/v1/runtime.h")
    if runtime_blob != EXPECTED_RUNTIME_BLOB:
        fail(f"canonical runtime changed: {runtime_blob}")
    if api_blob != EXPECTED_API_BLOB:
        fail(f"canonical API changed: {api_blob}")

    changed = [
        p for p in run(root, "git", "diff", "--name-only", BASE_COMMIT, "HEAD").splitlines()
        if p
    ]
    disallowed = [p for p in changed if not p.startswith(ALLOWED_PREFIXES)]
    if disallowed:
        fail(f"disallowed implementation paths: {disallowed}")
    if "lineage.md" in changed or "programs/arcllm_v1/lineage.md" in changed:
        fail("lineage must not change before outcome adjudication")
    if any(p.startswith(("src/", "include/", "shaders/")) for p in changed):
        fail("canonical runtime/kernel surface changed")

    runtime_text = (
        root / "src" / "arcllm_v1_runtime.cpp"
    ).read_text(encoding="utf-8").replace("\r\n", "\n")
    if "ARCLLM_LMAX_P0_INSTRUMENTATION_BEGIN" in runtime_text:
        fail("canonical runtime contains P0 instrumentation")

    generator = load_generator(exp / "generate_instrumented_runtime.py")
    generated = generator.transform(runtime_text)
    restored = generator.reverse_transform(generated)
    if restored != runtime_text:
        fail("instrumented source is not exactly reversible")
    if generated.count("ARCLLM_LMAX_P0_INSTRUMENTATION_BEGIN") != 3:
        fail("unexpected instrumentation block count")
    if generated.count("arcllm_lmax_p0_emit_token(") != 3:
        fail("expected helper definition plus exactly two token-ready calls")

    runner = (exp / "p0_runner.cpp").read_text(encoding="utf-8")
    ring = (exp / "p0_ring.h").read_text(encoding="utf-8")
    selftest = (exp / "p0_ring_selftest.cpp").read_text(encoding="utf-8")

    if runner.count("arcllm::v1::runtime::generate(request)") != 1:
        fail("both arms must converge on exactly one canonical generate call site")
    if runner.count("invoke_canonical_semantics(request, o.trace)") != 1:
        fail("direct arm does not use the common canonical invocation")
    if runner.count("invoke_canonical_semantics(*event.request, *event.trace)") != 1:
        fail("ring arm does not use the common canonical invocation")
    if "ARCLLM_LMAX_ARCH_P0_EXECUTION_AUTHORIZED" not in runner:
        fail("runner is not fail-closed on explicit execution authorization")
    if 'request.request_within_validated_domain = false;' not in runner:
        fail("validated-domain flag is not frozen false")
    if "EvidenceProfile::PROFILE_0" not in runner:
        fail("evidence profile is not frozen to PROFILE_0")
    if "7919ull * i + 104729ull * w.cell_index" not in runner:
        fail("frozen token formula missing")
    for cell, prefill, output in (
        ("W1", 8, 8), ("W2", 8, 32), ("W3", 64, 8),
        ("W4", 64, 32), ("W5", 256, 8), ("W6", 256, 32),
    ):
        expected = f'if (id == "{cell}") return {{"{cell}", '
        if expected not in runner:
            fail(f"missing workload cell {cell}")
        if f"{prefill}u, {output}u" not in runner:
            fail(f"workload dimensions missing for {cell}")

    if "SequencedRing<RuntimeEvent, 64>" not in runner:
        fail("inference ring capacity drift")
    if "static_assert((Capacity & (Capacity - 1)) == 0" not in ring:
        fail("power-of-two ring invariant missing")
    if "++local_spins == 128u" not in ring:
        fail("spin-then-yield threshold drift")
    if "slot.value = value;" not in ring:
        fail("preallocated slot publication missing")

    if "constexpr std::uint64_t kEvents = 8192;" not in selftest:
        fail("zero-science fixture size drift")
    if "steady_state_allocations=0" not in selftest:
        fail("no-allocation invariant missing")
    if "full_ring_observations > 0u" not in selftest:
        fail("forced-backpressure invariant missing")
    if "chrono" in selftest or "events_per_second" in selftest:
        fail("selftest must not become the preregistered performance subtest")

    build_script = (exp / "buildonly.py")
    if not build_script.exists():
        fail("direct-executable BuildOnly orchestrator missing")
    build_text = build_script.read_text(encoding="utf-8")
    forbidden = ('"--arm"', '"--model"', "'--arm'", "'--model'")
    if any(token in build_text for token in forbidden):
        fail("BuildOnly orchestrator contains outcome-bearing inference invocation")
    if '"--describe"' not in build_text:
        fail("BuildOnly runner smoke must use --describe only")
    if "powershell.exe" in build_text.lower():
        fail("BuildOnly must not depend on a managed-mode PowerShell outer launcher")

    print("ARCLLM_LMAX_ARCH_P0_STATIC_QA=PASS")
    print(f"CANONICAL_RUNTIME_BLOB={runtime_blob}")
    print(f"CANONICAL_API_BLOB={api_blob}")
    print(f"CHANGED_PATHS={len(changed)}")
    print("MODEL_EXECUTED=false")
    print("VULKAN_INITIALIZED=false")
    print("OUTCOME_PERFORMANCE_MEASURED=false")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ARCLLM_LMAX_ARCH_P0_STATIC_QA=FAIL: {exc}", file=sys.stderr)
        raise
