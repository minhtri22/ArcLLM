from pathlib import Path
import json,subprocess

ROOT=Path(__file__).resolve().parents[1]
def txt(p): return (ROOT/p).read_text(encoding="utf-8")
def js(p): return json.loads(txt(p))
def between(s,a,b):
    i=s.find(a);j=s.find(b,i)
    assert i>=0 and j>=0, f"missing region {a}->{b}"
    return s[i:j]

q2=txt("src/q2_benchmark.cpp")
cand=txt("src/arcllm_v1_i003_candidate_benchmark.cpp")
base=txt("baseline/i003_llama_adapter.cpp")
cmake=txt("baseline/CMakeLists.txt")
runner=txt("run_arcllm_v1_i003.ps1")
spec=txt("docs/research/arcllm-v1/ARCLLM_V1_I003_MATCHED_EXTERNAL_BASELINE_SPEC.md")
lock=js("config/arcllm_v1_i003_execution_lock_v0.1.3.json")
qualify_baseline=txt("tools/qualify_arcllm_v1_i003_baseline.ps1")
preflight=txt("run_arcllm_v1_i003_preflight.ps1")

# Closed I002 candidate identity.
blob=subprocess.check_output(["git","-C",str(ROOT),"rev-parse","HEAD:shaders/sa1_q4k_subgroup_splitk.comp"],text=True).strip()
assert blob=="56999d88dc1bef6486e7e1908982f6de4b0f9f6a"
assert lock["candidate"]["source_blob"]==blob
assert lock["candidate"]["spv_sha256"]=="B16868A807C4AE46EC2EE08457D8A3208D3D1CC2C856737CE109F010391A7569"

# Candidate prefill must be byte-identical to Q2.
a='        auto build_prefill=[&](uint32_t seq){'
b='        auto build_decode='
assert between(q2,a,b)==between(cand,a,b)

# Decode delta is exactly gate/up candidate substitution.
assert cand.count('"sa1_q4k_subgroup_splitk.spv"')==2  # exact unescaped literals: gate + up dispatch sites
metadata_lines=[line for line in cand.splitlines() if "candidate_shader" in line]
assert len(metadata_lines)==1
assert "sa1_q4k_subgroup_splitk.spv" in metadata_lines[0]
assert 'p+"ffn_gate","sa1_q4k_subgroup_splitk.spv"' in cand
assert 'p+"ffn_up","sa1_q4k_subgroup_splitk.spv"' in cand
for forbidden in ['p+"q_proj","sa1_', 'p+"k_proj","sa1_', 'p+"v_proj","sa1_', 'p+"o_proj","sa1_', 'p+"ffn_down","sa1_', '"lm_head","sa1_']:
    assert forbidden not in cand
assert 'candidate_gate_up_nodes_per_step' in cand
assert 'warmups!=1||measured!=1' in cand
assert 'for(int i=0;i<measured;++i)' in cand

# Baseline build-stage status must be explicit, not inherited from an internal native command.
assert qualify_baseline.rstrip().endswith("exit 0")
assert '$CMakeVersionLines=@(& $CMakeExe --version)' in qualify_baseline
assert 'I003 baseline CMake version query failed' in qualify_baseline
assert 'I003 baseline qualification artifact mismatch' in preflight
assert 'I003 baseline adapter hash mismatch after qualification' in preflight

# External baseline remains exact Q2 runtime contract except 1 measured attempt/process.
for x in [
    'mp.n_gpu_layers=-1;',
    'cp.n_ctx=4096;',
    'cp.n_batch=256;',
    'cp.n_ubatch=256;',
    'cp.n_threads=8;',
    'cp.n_threads_batch=8;',
    'cp.type_k=GGML_TYPE_F32;',
    'cp.type_v=GGML_TYPE_F32;',
    'cp.offload_kqv=true;',
    'b29c606e28a01b1bc8c1351026a0fa6e616bf6c4',
    'warmups!=1||measured!=1',
    'for(int i=0;i<measured;++i)'
]:
    assert x in base, f"baseline contract drift: {x}"
assert 'add_executable(i003_llama_adapter i003_llama_adapter.cpp)' in cmake

# Exact raw workloads remain inherited. Candidate and llama adapters express
# the same token IDs with different C++ container/type syntax, so validate each
# representation explicitly instead of requiring byte-identical source text.
assert 'input_ids={1u,133151u,133152u,152062u};' in cand
assert 'return {1,133151,133152,152062};' in base
assert 'input_ids[0]=1u;input_ids[1]=133151u;input_ids[2]=133152u;input_ids[3]=152062u;' in cand
assert 'v[0]=1;v[1]=133151;v[2]=133152;v[3]=152062;' in base
assert '1u+((104729u+7919u*i)%152063u)' in cand
assert '1u+((104729u+7919u*i)%152063u)' in base

# Paired DEV_HOST design.
for x in [
    'for($p=0;$p-lt5;$p++)',
    '$CandidateFirst',
    '@("candidate","llama")',
    '@("llama","candidate")',
    'manual_full_collection_rerun_permitted=$true',
    'selective_pair_or_cell_rerun_permitted=$false',
    'primary_dataset_rule="FIRST_COMPLETE_VALID_COLLECTION"',
    'ambient_load_is_blocker=$false'
]:
    assert x in runner, f"runner contract missing: {x}"

# Study is characterization, not a hidden winner threshold.
assert "matched characterization study" in spec
assert "does not optimize another kernel" in spec
assert "No other classification is permitted." in spec
assert lock["fresh_measurement_authorized"] is False
assert lock["measured_inferences_authorized"]==0

# No committed I003 science authorization at package stage.
assert not (ROOT/"config/arcllm_v1_i003_science_authorization.json").exists()

print("ARCLLM_V1_I003_STATIC_QA=PASS")
