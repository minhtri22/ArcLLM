from pathlib import Path
import re, sys

ROOT=Path(__file__).resolve().parents[1]
q2=(ROOT/"src/q2_benchmark.cpp").read_text(encoding="utf-8")
prof=(ROOT/"src/arcllm_v1_i001r_7b_decode_profile.cpp").read_text(encoding="utf-8")
base=(ROOT/"src/p8c_segmented_access_correctness.cpp").read_text(encoding="utf-8")
rt=(ROOT/"src/arcllm_v1_i001r_p8c_profile_runtime.cpp").read_text(encoding="utf-8")

def between(s,a,b):
    i=s.find(a); j=s.find(b,i)
    if i<0 or j<0: raise AssertionError(f"missing region {a} -> {b}")
    return s[i:j]

# Exact semantic graph builder inherited from Q2.
graph_a="        auto build_prefill=[&](uint32_t seq){"
graph_b="        auto ppops=build_prefill(seq);auto dtemplate=build_decode(seq);"
assert between(q2,graph_a,graph_b)==between(prof,graph_a,graph_b), "I001R graph-builder drift from Q2"

# Exact model/dispatch invariants.
for literal in [
    "const uint32_t H=3584,QH=28,KVH=4,HD=128,KV=512,FFN=18944,MAXSEQ=256,MAXCTX=4096,VOC=152064;",
    "const uint32_t LAYERS=28,F32=0,Q4=12,Q6=14;",
    "const uint32_t EXPECT_PREFILL=441,EXPECT_DECODE=469;",
    'if(workload=="W-S")',
    '}else if(workload=="W-C"){',
    'if(warmups!=1||measured!=5)'
]:
    assert literal in prof, f"missing frozen invariant: {literal}"

# Normal prepared execution path must remain byte-identical to the historical runtime.
start="    ChainStats execute_prepared("
base_exec=between(base,start,"    void destroy_prepared")
rt_exec=between(rt,start,"    ProfileStats execute_profiled")
assert base_exec.rstrip()==rt_exec.rstrip(), "normal execute_prepared drifted"

# Instrumentation only: timestamp query path plus three fixed decode probes.
for literal in [
    "ProfileStats execute_profiled",
    "vkGetPhysicalDeviceProperties",
    "timestamp_period_ns",
    "probe_di={0u,15u,30u}",
    "normal_lifecycle_steps_per_measured_attempt",
    "model_weight_bytes_per_decode_step",
    "not measured DRAM traffic",
    # C++ source contains escaped quotes because this literal is emitted inside a JSON string.
    r'\"pdep_implementation\":false'
]:
    assert literal in (rt+prof), f"missing I001R instrumentation contract: {literal}"

# No successor/optimization payload may enter the exact Q2-safe graph.
for forbidden in ["anl64_q4_fast.spv","ANL64_P4_LOCKED","persistent_kernel","graph_compression_kernel"]:
    assert forbidden not in prof, f"forbidden optimization payload present: {forbidden}"

# Profiled op names are emitted raw for independent family reclassification.
assert r'\"decode_op_names\"' in prof
assert r'\"op_ticks\"' in prof
assert "i001r_family" in prof

print("ARCLLM_V1_I001R_STATIC_QA=PASS")
