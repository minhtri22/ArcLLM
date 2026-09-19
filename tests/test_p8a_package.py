from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[1]
REQ=['config/p8_target.json','docs/P8A_CONTRACT.md','docs/P8A_IMPLEMENTATION.md','src/p8a_memory_plan.cpp','tools/build_p8a.ps1','tools/fetch_p8_target.ps1','run_p8a.ps1','manifest.json','.gitignore']
for r in REQ: assert (ROOT/r).is_file(),r
t=json.loads((ROOT/'config/p8_target.json').read_text(encoding='utf-8'))
assert t['schema']=='arcllm.p8.target.v1'
assert t['repo_id']=='Qwen/Qwen2.5-Coder-7B-Instruct-GGUF'
assert t['revision']=='13fb94bfda8c8cf22497dc57b78f391a9acb426a'
assert t['filename']=='qwen2.5-coder-7b-instruct-q4_k_m.gguf'
assert t['sha256']=='509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C'
assert t['size_bytes']==4683073536
assert t['max_context_initial']==4096 and t['prefill_chunk_initial']==512
assert t['arena_cap_bytes']==268435456
assert t['observed_target_vulkan_budget_bytes']==18522046464
assert t['safety_margin_bytes']==2147483648
assert t['usable_planned_bytes']==16374562816
s=(ROOT/'src/p8a_memory_plan.cpp').read_text(encoding='utf-8')
for x in ['EXPECTED_FILE_BYTES=4683073536ull','ARENA_CAP=268435456ull','MAX_CTX=4096ull','PREFILL=512ull','BUDGET=18522046464ull','SAFETY=2147483648ull','arcllm.p8a.memory_plan.v1','oversize_tensors','arena_pack_pass','qwen2.block_count','qwen2.embedding_length','qwen2.attention.head_count','qwen2.attention.head_count_kv','qwen2.feed_forward_length','qwen2.context_length']:
    assert x in s,x
assert 'VkRuntime' not in s and 'vkCreate' not in s and '#include <vulkan' not in s
assert 'TensorStore' not in s
assert 'std::filesystem::file_size' in s
assert 'checked_mul(checked_mul(checked_mul(layers,MAX_CTX),kv_dim),2)' in s
assert 'checked_mul(11,hidden)' in s and 'checked_mul(3,kv_dim)' in s and 'checked_mul(3,ffn)' in s
assert 'total_planned<=USABLE' in s
assert 'b>ARENA_CAP' in s
b=(ROOT/'tools/build_p8a.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8a_memory_plan.cpp','gguf.cpp'],cpp_refs
f=(ROOT/'tools/fetch_p8_target.ps1').read_text(encoding='ascii')
assert '13fb94bfda8c8cf22497dc57b78f391a9acb426a' in f
assert '509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C' in f
assert '$ExpectedSize=4683073536' in f and 'Get-FileHash' in f and 'curl.exe' in f
r=(ROOT/'run_p8a.ps1').read_text(encoding='ascii')
assert 'Get-FileHash' in r and 'test_p8a_package.py' in r and 'arcllm_p8a.exe' in r
assert 'full_inference_permitted' in r and 'P8-B residency allocation bring-up' in r
assert '.models\\qwen2.5-coder-7b-instruct-q4_k_m.gguf' in r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-A' and m['status']=='READY_TO_RUN'
assert m['gate']['performance_gate'] is False
assert m['gate']['full_inference_forbidden_until_pass'] is True
assert m['target']['sha256']==t['sha256'] and m['target']['size_bytes']==t['size_bytes']
gi=(ROOT/'.gitignore').read_text(encoding='utf-8')
assert '.models/' in gi
print('ArcLLM P8-A static contract PASS')
