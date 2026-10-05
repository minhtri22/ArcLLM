#!/usr/bin/env python3
from __future__ import annotations
import json, pathlib, sys

ROOT=pathlib.Path(__file__).resolve().parents[2]
EXP=ROOT/"experiments"/"arcllm_lmax_arch_p1"

def fail(msg): raise RuntimeError(msg)
def need(text,token,where):
    if token not in text: fail(f"missing {token!r} in {where}")

def main():
    p=json.loads((ROOT/"config"/"arcllm_lmax_arch_p1_preregistration_v0.4.json").read_text())
    runner=(EXP/"p1_runner.cpp").read_text()
    control=(EXP/"p1_control_path.cpp").read_text()
    driver=(EXP/"p1_e_driver.py").read_text()

    required=p["inference_evidence_required_raw_primitives"]
    mapping={
      "request_start_ns":"\\\"request_start_ns\\\"",
      "request_end_ns":"\\\"request_end_ns\\\"",
      "first_token_ready_ns":"\\\"first_token_ready_ns\\\"",
      "token_ready_ns":"\\\"token_ready_ns\\\"",
      "cpu_start_100ns":"\\\"cpu_start_100ns\\\"",
      "cpu_end_100ns":"\\\"cpu_end_100ns\\\"",
      "logical_processor_count":"\\\"logical_processor_count\\\"",
      "request_allocation_counter_start":"\\\"request_allocation_counter_start\\\"",
      "request_allocation_counter_end":"\\\"request_allocation_counter_end\\\"",
      "decode_allocation_counter_start":"\\\"decode_allocation_counter_start\\\"",
      "decode_allocation_counter_end":"\\\"decode_allocation_counter_end\\\"",
      "generated_token_ids":"\\\"generated_token_ids\\\"",
      "generated_token_count":"generated_token_ids.size()",
      "runtime_stats":"\\\"runtime_stats\\\"",
      "finite":"\\\"finite\\\"",
      "error":"\\\"error\\\"",
      "ring_slot_spin_count":"slot_spin_count",
      "ring_slot_switch_to_thread_count":"slot_switch_to_thread_count",
      "ring_slot_wait_on_address_count":"slot_wait_on_address_count",
      "ring_slot_wake_count":"slot_wake_count",
      "completion_spin_count":"completion_wait",
      "completion_switch_to_thread_count":"completion_wait",
      "completion_wait_on_address_count":"completion_wait",
      "completion_wake_count":"completion_wait",
      "consumer_ready_spin_count":"consumer_ready_wait",
      "consumer_ready_switch_to_thread_count":"consumer_ready_wait",
      "consumer_ready_wait_on_address_count":"consumer_ready_wait",
      "consumer_ready_wake_count":"consumer_ready_wait",
    }
    missing=[]
    for field in required:
        token=mapping.get(field)
        if token is None or token not in runner: missing.append(field)
    if missing: fail("runner evidence fields incomplete: "+repr(missing))

    for token in (
      '"identity"', '"runner_sha256"', '"model_sha256"',
      '"shader_manifest_sha256"', '"authorization_sha256"',
      '"evidence_contract_sha256"', '"raw_primitives"',
      '"completion_wait"', '"consumer_ready_wait"', '"ring"'
    ):
        # source emits escaped literals
        need(runner, token.replace('"','\\\"'), "p1_runner.cpp")

    for token in (
      '"steady_state_allocation_count"',
      '"full_backpressure_observation_rate"',
      '"producer_condition_variable_wait_count"',
      '"consumer_condition_variable_wait_count"',
      '"first_ordering_mismatch"',
      'write_u64le(sequence_out, consumed)',
      'write_u64le(latency_out, latency)',
      'events != 1000000u'
    ):
        t=token if not token.startswith('"') else token.replace('"','\\\"')
        need(control,t,"p1_control_path.cpp")

    for token in (
      'expected_u64_entries"]=1000000',
      'expected_u64_bytes"]=8000000',
      'consumed_sequence_ids_sha256',
      'publish_to_consume_latency_ns_sha256',
      'planned_measured_inference_runs":48',
      'planned_control_runs":6',
      'control_events_per_run":1000000',
      '"--phase",item["phase"]',
      '"--pair-index",str(pair_index)',
      '"--pair-position",str(pair_position)'
    ):
        need(driver,token,"p1_e_driver.py")

    print("ARCLLM_LMAX_ARCH_P1_EVIDENCE_SCHEMA_QA=PASS")
    print("INFERENCE_REQUIRED_FIELDS="+str(len(required)))
    print("CONTROL_BINARY_STREAMS=2_PER_RUN")
    print("CONTROL_U64_ENTRIES_PER_STREAM=1000000")
    print("MODEL_EXECUTED=false")
    print("VULKAN_INITIALIZED=false")
    print("FRESH_OUTCOME_OPENED=false")
    return 0

if __name__=="__main__":
    try: raise SystemExit(main())
    except Exception as exc:
        print("ARCLLM_LMAX_ARCH_P1_EVIDENCE_SCHEMA_QA=FAIL: "+str(exc),file=sys.stderr)
        raise
