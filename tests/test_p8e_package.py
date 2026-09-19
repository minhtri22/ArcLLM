from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]
REQ=[
 'docs/P8E_CONTRACT.md','docs/P8E_IMPLEMENTATION.md',
 'src/p8e_single_layer_correctness.cpp',
 'tools/compile_p8e_shaders.ps1','tools/build_p8e.ps1','run_p8e.ps1',
 'manifest.json','P8D_SHA256SUMS.txt',
 'inputs/p8d_shader_provenance.authoritative.json',
 'inputs/p8d_graph_binding_results.authoritative.json',
 'inputs/p8d_summary.authoritative.json'
]
for r in REQ:
    assert (ROOT/r).is_file(),r

assert hashlib.sha256((ROOT/'inputs/p8d_shader_provenance.authoritative.json').read_bytes()).hexdigest().upper()=='FABD3DAE027DB5AA69E037FE179BCD0C4F5A16C5D3AAFF0E08A938101AC8F454'
assert hashlib.sha256((ROOT/'inputs/p8d_graph_binding_results.authoritative.json').read_bytes()).hexdigest().upper()=='53C373BD3BA1A9BD31B45702CED08E2EB39C7F2B7B099E052EC8C5153A824ABD'
assert hashlib.sha256((ROOT/'inputs/p8d_summary.authoritative.json').read_bytes()).hexdigest().upper()=='74624BFAC44F4E5B9A6F08AC972508A353DAB96E0B6D4B92E8C658ABB1651D27'

pg=json.loads((ROOT/'inputs/p8d_graph_binding_results.authoritative.json').read_text(encoding='utf-8'))
assert pg['status']=='PASS' and pg['gate']['p8d_pass']
assert pg['resolver']['resolved_tensors']==339
assert pg['resolver']['physical_piece_count']==341
assert pg['resolver']['segmented_logical_tensor_count']==2
assert pg['resolver']['span_equivalence_pass'] and pg['resolver']['global_coverage_pass']

s=(ROOT/'src/p8e_single_layer_correctness.cpp').read_text(encoding='utf-8')
for x in [
 '#define main p8c_main_disabled',
 'SEQ=4','MAXCTX=4096',
 'p8e_build_bindings',
 'bindings.size()!=339u||piece_count!=341u||multi_count!=2u',
 'executed_layer=0,executed_layer_count=1',
 'if(ops.size()!=15u)',
 'stats.dispatch_count==15u',
 'stats.submit_count==1u',
 'cmp("final_layer_output"',
 'arcllm.p8e.single_layer_correctness.v1'
]:
    assert x in s,x
assert s.count('addop("L0.')==15
assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s
# The 28-layer loop is required only for graph tensor-name census metadata.
# Execution itself must remain a statically enumerated layer-0 chain with no layer loop.
gn0=s.index('static std::vector<std::string> p8e_graph_names()')
gn1=s.index('static std::map<std::string,P8EBinding> p8e_build_bindings',gn0)
assert gn0>=0 and gn1>gn0
graph_name_body=s[gn0:gn1]
assert 'for(uint32_t l=0;l<28u;++l)' in graph_name_body

ex0=s.index('std::vector<DispatchOp> ops;')
ex1=s.index('if(ops.size()!=15u)',ex0)
assert ex0>=0 and ex1>ex0
execution_builder=s[ex0:ex1]
assert 'for(uint32_t l=' not in execution_builder
assert execution_builder.count('addop("L0.')==15
for opname in [
 'L0.attn_rmsnorm','L0.q_proj','L0.k_proj','L0.v_proj','L0.q_rope','L0.k_rope',
 'L0.kv_store','L0.causal_gqa','L0.o_proj','L0.attn_residual','L0.ffn_rmsnorm',
 'L0.ffn_gate_up_fused','L0.swiglu','L0.ffn_down','L0.ffn_residual'
]:
    assert opname in s,opname

c=(ROOT/'tools/compile_p8e_shaders.ps1').read_text(encoding='ascii')
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert 'arcllm.p8e.shader_provenance.v1' in c
for sh in [
 'p7_rmsnorm_seq.comp','p7c_ffn_q4k_tiled.comp','p7c_ffn_q6k_tiled.comp',
 'p7_rope_seq.comp','p7_kv_store.comp','p7_attention_prefill_online.comp',
 'p7_add.comp','p7l_ffn_q4k_gateup_fused.comp','p7_swiglu.comp',
 'p7g_ffn_q4k_tiled16.comp','p7g_ffn_q6k_tiled16.comp'
]:
    assert sh in c,sh

b=(ROOT/'tools/build_p8e.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8e_single_layer_correctness.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs

r=(ROOT/'run_p8e.ps1').read_text(encoding='ascii')
assert 'test_p8e_package.py' in r and 'compile_p8e_shaders.ps1' in r and 'arcllm_p8e.exe' in r
assert 'full_inference_permitted=$false' in r
assert 'multi_layer_prefix_design_permitted=($Obj.status -eq "PASS")' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-E' and m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8e_scope']['executed_layer']==0 and m['p8e_scope']['executed_layer_count']==1
assert m['p8e_scope']['sequence_length']==4
assert m['gate']['exact_dispatches']==15 and m['gate']['exact_submits']==1
assert m['gate']['exact_decoder_layers']==1
assert m['gate']['full_inference_forbidden'] is True

print('ArcLLM P8-E static contract PASS')
