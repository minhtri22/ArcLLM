#!/usr/bin/env python3
from __future__ import annotations
import json, pathlib, subprocess, sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
EXP = ROOT / "experiments" / "arcllm_lmax_arch_p1"
BASE = "2cc2d788aa2782cf9fd6b723f9daa183a476d755"
EXPECTED_RUNTIME_BLOB = "0c613f6f740931a88ddd3ee5b904533a58001a06"
EXPECTED_API_BLOB = "d7821270ed253e1d1d96e3916b22b3daee50f5bc"

ALLOWED = (
    "docs/research/arcllm-v1/ARCLLM_LMAX_ARCH_P1_",
    "config/arcllm_lmax_arch_p1_",
    "experiments/arcllm_lmax_arch_p1/",
    "results/arcllm_lmax_arch_p1_",
)

def run(*argv: str) -> str:
    return subprocess.check_output(list(argv), cwd=ROOT, text=True, stderr=subprocess.STDOUT).strip()

def fail(msg: str) -> None:
    raise RuntimeError(msg)

def must(text: str, token: str, where: str) -> None:
    if token not in text:
        fail(f"missing {token!r} in {where}")

def main() -> int:
    p = json.loads((ROOT/"config"/"arcllm_lmax_arch_p1_preregistration_v0.4.json").read_text())
    if p["schema"] != "arcllm.lmax_arch_p1.preregistration.v0.4":
        fail("v0.4 prereg schema drift")
    if p["science_execution_authorized"] is not False:
        fail("fresh P1 execution became authorized during R")
    if p["control_path_subtest"]["arm_definitions"]["A"]["id"] != "DIRECT_LOCKED_SPSC64":
        fail("control comparator drift")
    if p["control_path_subtest"]["service_delay_mechanism"]["id"] != "MONOTONIC_BUSY_DELAY":
        fail("service-delay mechanism drift")

    runtime_blob = run("git","hash-object","src/arcllm_v1_runtime.cpp")
    api_blob = run("git","hash-object","include/arcllm/v1/runtime.h")
    if runtime_blob != EXPECTED_RUNTIME_BLOB:
        fail("canonical runtime changed")
    if api_blob != EXPECTED_API_BLOB:
        fail("canonical API changed")

    changed = [x for x in run("git","diff","--name-only",BASE,"HEAD").splitlines() if x]
    disallowed = [x for x in changed if not x.startswith(ALLOWED)]
    if disallowed:
        fail(f"disallowed P1 paths: {disallowed}")
    if "lineage.md" in changed:
        fail("lineage must not change before A adjudication")
    if any(x.startswith(("src/","include/","shaders/")) for x in changed):
        fail("canonical runtime/kernel surface changed")

    ring = (EXP/"p1_ring.h").read_text()
    runner = (EXP/"p1_runner.cpp").read_text()
    control = (EXP/"p1_control_path.cpp").read_text()
    gen = (EXP/"generate_instrumented_runtime.py").read_text()
    driver = (EXP/"p1_e_driver.py").read_text()

    # Frozen ring wait semantics.
    for token in (
        "active_spins < 16u",
        "YieldProcessor()",
        "SwitchToThread()",
        "WaitOnAddress(",
        "WakeByAddressSingle(",
        "slot_wait_on_address_count",
        "full_backpressure_observations",
    ):
        must(ring, token, "p1_ring.h")
    if "Sleep(" in ring or "sleep_for" in ring:
        fail("ring wait policy contains forbidden sleep")

    # Completion + consumer-ready semantics and evidence-complete primitives.
    for token in (
        "wait_until_true(done, completion_counters)",
        "publish_true_and_wake(*event.done, completion_counters)",
        "wait_until_true(consumer_ready, ready_counters)",
        "o.consumer_ready_wait = ready_counters.snapshot()",
        "\"request_start_ns\"",
        "\"request_end_ns\"",
        "\"first_token_ready_ns\"",
        "\"cpu_start_100ns\"",
        "\"cpu_end_100ns\"",
        "\"logical_processor_count\"",
        "\"request_allocation_counter_start\"",
        "\"request_allocation_counter_end\"",
        "\"decode_allocation_counter_start\"",
        "\"decode_allocation_counter_end\"",
        "\"completion_wait\"",
        "\"consumer_ready_wait\"",
        "ARCLLM_LMAX_ARCH_P1_E_EXECUTION_AUTHORIZED",
    ):
        must(runner, token, "p1_runner.cpp")
    if "while (!done.load" in runner or "while (!consumer_ready.load" in runner:
        fail("P0 unbounded yield loop survived in P1 runner")
    if runner.count("arcllm::v1::runtime::generate(request)") != 1:
        fail("runner must have exactly one canonical generate call site")

    # Control comparator and common timing mechanics.
    for token in (
        "class DirectLockedSpsc",
        "std::condition_variable not_full_",
        "std::condition_variable not_empty_",
        "count_ == Capacity",
        "MONOTONIC_BUSY_DELAY",
        "deadline_ns = consume_timestamp_ns + service_delay_ns",
        "YieldProcessor()",
        "events != 1000000u",
        "delay_ns != 0u && delay_ns != 10000u && delay_ns != 100000u",
        "DIRECT_LOCKED_SPSC64",
        "LMAX_RING_P1",
        "ARCLLM_LMAX_ARCH_P1_E_EXECUTION_AUTHORIZED",
        "write_u64le(sequence_out, consumed)",
        "write_u64le(latency_out, latency)",
    ):
        must(control, token, "p1_control_path.cpp")
    delay_start = control.index("void monotonic_busy_delay")
    delay_end = control.index("std::string read_text_file", delay_start)
    delay_block = control[delay_start:delay_end]
    for forbidden in ("SwitchToThread", "Sleep(", "sleep_for", "WaitOnAddress", "condition_variable"):
        if forbidden in delay_block:
            fail(f"forbidden delay primitive: {forbidden}")

    # Fresh-E driver is frozen, fail-closed and schedule-complete.
    for token in (
        'CELLS=["W1","W2","W3","W4","W5","W6"]',
        'PAIR_ORDERS=[["direct","ring"],["ring","direct"],["direct","ring"],["ring","direct"]]',
        '(0,"direct_locked_spsc64"),(0,"lmax_ring_p1")',
        '(10000,"lmax_ring_p1"),(10000,"direct_locked_spsc64")',
        '(100000,"direct_locked_spsc64"),(100000,"lmax_ring_p1")',
        'ARCLLM_LMAX_ARCH_P1_E_EXECUTION_AUTHORIZED',
        '"planned_excluded_warmups":4',
        '"planned_measured_inference_runs":48',
        '"planned_control_runs":6',
        '"control_events_per_run":1000000',
        'm["consumed_sequence_ids_bytes"]=seq.stat().st_size',
        'm["publish_to_consume_latency_ns_bytes"]=lat.stat().st_size',
        'm["expected_u64_bytes"]=8000000',
        '"rerun_forbidden":True',
        '"adjudication_performed":False',
    ):
        must(driver, token, "p1_e_driver.py")
    if "selective" in driver.lower() and "selective_rerun" in driver.lower():
        fail("driver contains unexpected selective-rerun path")
    if 'if not all(conditions.values())' not in driver or 'E_STARTED.json' not in driver:
        fail("driver prelaunch/marker fail-close missing")

    # Runtime instrumentation remains reversible and P1-only.
    for token in (
        "ARCLLM_LMAX_P1_INSTRUMENTATION_BEGIN",
        "arcllm_lmax_p1_emit_token",
        "EXPECTED_RUNTIME_BLOB",
    ):
        must(gen, token, "generate_instrumented_runtime.py")

    print("ARCLLM_LMAX_ARCH_P1_STATIC_QA=PASS")
    print("CANONICAL_RUNTIME_BLOB="+runtime_blob)
    print("CANONICAL_API_BLOB="+api_blob)
    print("CHANGED_PATHS="+str(len(changed)))
    print("MODEL_EXECUTED=false")
    print("VULKAN_INITIALIZED=false")
    print("CONTROL_PATH_1M_EXECUTED=false")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print("ARCLLM_LMAX_ARCH_P1_STATIC_QA=FAIL: "+str(exc), file=sys.stderr)
        raise
