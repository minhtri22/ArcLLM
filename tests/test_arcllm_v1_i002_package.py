from pathlib import Path
import json, subprocess

ROOT=Path(__file__).resolve().parents[1]
def txt(p): return (ROOT/p).read_text(encoding="utf-8")
def js(p): return json.loads(txt(p))
def between(s,a,b):
    i=s.find(a); j=s.find(b,i)
    if i<0 or j<0: raise AssertionError(f"missing region {a}->{b}")
    return s[i:j]

q2=txt("src/q2_benchmark.cpp")
t1=txt("src/arcllm_v1_i002_t1_transfer.cpp")
t3=txt("src/arcllm_v1_i002_t3_paired.cpp")
sa=txt("shaders/sa1_q4k_subgroup_splitk.comp")
sa0=js("artifacts/SA0/SA0_CAP_ADJUDICATION_v0.1.json")
sa1=js("artifacts/SA1/SA1K1_Q4_ADJUDICATION_v0.1.json")
lock=js("config/arcllm_v1_i002_execution_lock_v0.1.json")

# Exact candidate provenance.
blob=subprocess.check_output(["git","-C",str(ROOT),"rev-parse","HEAD:shaders/sa1_q4k_subgroup_splitk.comp"],text=True).strip()
assert blob=="56999d88dc1bef6486e7e1908982f6de4b0f9f6a"

# Historical exact-device capability.
assert sa0["result"]=="SA0_CAP_PASS"
assert sa0["environment"]["gpu"]=="Intel(R) Arc(TM) 140V GPU (16GB)"
assert sa0["environment"]["windows_driver"]=="32.0.101.8860"
assert sa0["baseline_capabilities"]["subgroup_size"]==32
assert sa0["optional_capabilities"]["compute_full_subgroups"] is True

# Exact-shape SA1 mechanism evidence.
cell="Q4_H3584_R18944_NOBIAS"
assert sa1["result"]=="Q4_STAGE_PASS"
assert sa1["process_A"]["cells"][cell]["speedup"]>5.0
assert sa1["process_B"]["cells"][cell]["speedup"]>5.0

# Candidate arithmetic/geometry is the frozen SA1 mechanism.
for x in [
    "#extension GL_KHR_shader_subgroup_arithmetic : require",
    "layout(local_size_x = 128",
    "uint subgroup = gl_SubgroupID;",
    "uint lane = gl_SubgroupInvocationID;",
    "uint row = gl_WorkGroupID.x * 4u + subgroup;",
    "for (uint k = lane; k < pc.n; k += 32u)",
    "float sum = subgroupAdd(partial);",
]:
    assert x in sa, f"candidate mechanism drift: {x}"

# Prefill stays byte-identical to Q2 in both new harnesses.
a="        auto build_prefill=[&](uint32_t seq){"
b="        auto build_decode="
assert between(q2,a,b)==between(t1,a,b)
assert between(q2,a,b)==between(t3,a,b)

# T3: candidate occurs only at the two syntactic gate/up addops in the decode loop.
assert t3.count("sa1_q4k_subgroup_splitk.spv")==2
prefill=between(t3,"        auto build_prefill=[&](uint32_t seq){","        auto build_decode=")
assert "sa1_q4k_subgroup_splitk.spv" not in prefill
for forbidden in [
    'p+"q_proj",candidate?', 'p+"k_proj",candidate?', 'p+"v_proj",candidate?',
    'p+"o_proj",candidate?', 'p+"ffn_down",candidate?', '"lm_head",candidate?'
]:
    assert forbidden not in t3
assert 'p+"ffn_gate",candidate?' in t3
assert 'p+"ffn_up",candidate?' in t3
assert "candidate_gate_up_nodes_per_step\\":56" in t3
assert "T2 full-model token semantic guard failed before T3 measurement" in t3
assert "t2_semantic_guard_pass" in t3
assert "EXPECT_PREFILL=441,EXPECT_DECODE=469" in t3

# T1 exact sample matrix and correctness gates.
for x in [
    "target_layers={0u,13u,27u}",
    "target_di={0u,15u,30u}",
    'compare_vec(baseline,candidate,0.02,0.005)',
    "obs.size()!=18u",
    "execute_profiled",
    "baseline_ticks",
    "candidate_ticks",
    "restore pair census",
]:
    assert x in t1, f"T1 contract missing: {x}"
assert t1.count("sa1_q4k_subgroup_splitk.spv")==2  # source use + output identity
assert "arcllm_v1_i001r_p8c_profile_runtime.cpp" in t1

# T3 paired/counterbalanced and semantic guard.
for x in [
    'session!="A"&&session!="B"',
    "for(int i=0;i<5;++i)",
    "baseline_first",
    "semantic_equal=p.baseline.success&&p.candidate.success&&p.baseline.generated==p.candidate.generated",
    "dtemplate_baseline",
    "dtemplate_candidate",
]:
    assert x in t3, f"T3 contract missing: {x}"

# No fresh execution is authorized by this package.
assert lock["fresh_target_model_execution_authorized"] is False
assert lock["implementation_scope"]["candidate_decode_nodes_per_step"]==56
assert lock["implementation_scope"]["prefill_modified"] is False

print("ARCLLM_V1_I002_STATIC_QA=PASS")
