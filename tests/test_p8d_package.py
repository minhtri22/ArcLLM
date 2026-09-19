from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]
REQ=[
    'docs/P8D_CONTRACT.md','docs/P8D_IMPLEMENTATION.md',
    'src/p8d_graph_binding.cpp',
    'shaders/p8c_embedding_q4k_segmented_probe.comp',
    'shaders/p8c_lmhead_q6k_segmented_probe.comp',
    'tools/compile_p8d_shaders.ps1','tools/build_p8d.ps1',
    'run_p8d.ps1','manifest.json','P8C_SHA256SUMS.txt',
    'inputs/p8a2_segment_plan.authoritative.json',
    'inputs/p8c_access_results.authoritative.json',
    'inputs/p8c_summary.authoritative.json'
]
for r in REQ:
    assert (ROOT/r).is_file(),r

assert hashlib.sha256((ROOT/'inputs/p8a2_segment_plan.authoritative.json').read_bytes()).hexdigest().upper()=='7390D579937EDD8748A08D7D72E7DFEC53D7470FA9C86208C2412A33E231D481'
assert hashlib.sha256((ROOT/'inputs/p8c_access_results.authoritative.json').read_bytes()).hexdigest().upper()=='9773C48A7D895EB3D22B993132854E58BC0668288725E5186E80D3462D4D5340'
assert hashlib.sha256((ROOT/'inputs/p8c_summary.authoritative.json').read_bytes()).hexdigest().upper()=='8620A9089CF9066088F5E30543864D7A17B87F4DC55CEFE1CD10320C01574CD4'
assert '25D7E7D01F033B85692E83C102D269A37E21988D229191BCA5FF51DBC0E118E3' in (ROOT/'P8C_SHA256SUMS.txt').read_text(encoding='utf-8')

p=json.loads((ROOT/'inputs/p8c_access_results.authoritative.json').read_text(encoding='utf-8'))
assert p['status']=='PASS' and p['gate']['p8c_pass'] and p['mapping_equivalence']['pass']

s=(ROOT/'src/p8d_graph_binding.cpp').read_text(encoding='utf-8')
for x in [
    '#define main p8c_main_disabled',
    'struct TensorBindingDescriptor',
    'p8d_build_graph_bindings',
    'names.size()!=339u',
    'piece_count!=341u',
    'multi_count!=2u',
    'bindings.at("token_embd.weight")',
    'bindings.at("output.weight")',
    'decoder_layer_dispatches=0u',
    'stats.dispatch_count==2u',
    'stats.submit_count==1u',
    'arcllm.p8d.graph_binding.v1'
]:
    assert x in s,x
assert 'payload+emb0_bytes' not in s
assert 'payload+out0_start' not in s
assert 'payload+out1_start' not in s
assert 'build_prefill' not in s and 'build_decode' not in s
assert s.count('p8c_embedding_q4k_segmented_probe.spv')==1
assert s.count('p8c_lmhead_q6k_segmented_probe.spv')==1

c=(ROOT/'tools/compile_p8d_shaders.ps1').read_text(encoding='ascii')
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert 'arcllm.p8d.shader_provenance.v1' in c
assert 'reused_unchanged_p8c_shader_sources=$true' in c

b=(ROOT/'tools/build_p8d.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8d_graph_binding.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs

r=(ROOT/'run_p8d.ps1').read_text(encoding='ascii')
assert 'test_p8d_package.py' in r and 'compile_p8d_shaders.ps1' in r and 'arcllm_p8d.exe' in r
assert 'full_inference_permitted=$false' in r
assert 'single_layer_bringup_permitted=($Obj.status -eq "PASS")' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-D' and m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['gate']['tensor_census']==339
assert m['gate']['arena_count']==19
assert m['gate']['physical_piece_count']==341
assert m['gate']['exact_segmented_logical_tensor_count']==2
assert m['gate']['exact_dispatches']==2
assert m['gate']['exact_submits']==1
assert m['gate']['decoder_layer_dispatches']==0
assert m['gate']['full_inference_forbidden'] is True

print('ArcLLM P8-D static contract PASS')
