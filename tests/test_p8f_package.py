from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]
REQ=[
 'docs/P8F_CONTRACT.md','docs/P8F_IMPLEMENTATION.md',
 'src/p8f_two_layer_prefix_correctness.cpp',
 'tools/compile_p8f_shaders.ps1','tools/build_p8f.ps1','run_p8f.ps1',
 'manifest.json','P8E_SHA256SUMS.txt',
 'inputs/p8e_shader_provenance.authoritative.raw',
 'inputs/p8e_single_layer_results.authoritative.raw',
 'inputs/p8e_summary.authoritative.raw'
]
for r in REQ:
    assert (ROOT/r).is_file(),r

assert hashlib.sha256((ROOT/'inputs/p8e_shader_provenance.authoritative.raw').read_bytes()).hexdigest().upper()=='73916DE149A413B835541B95F01FEAF2B87DDDE03C039C3D1ABC0E2A3A115861'
assert hashlib.sha256((ROOT/'inputs/p8e_single_layer_results.authoritative.raw').read_bytes()).hexdigest().upper()=='992A986081FAFC81AC2E6E1A38063384434DE3138CAE469A04A57FED57BDA52B'
assert hashlib.sha256((ROOT/'inputs/p8e_summary.authoritative.raw').read_bytes()).hexdigest().upper()=='EBC4C8088B0992A30D72973DC7485CCF7AA0618B354509A506CC9FDE294B5B9D'

pe=json.loads((ROOT/'inputs/p8e_single_layer_results.authoritative.raw').read_text(encoding='utf-8'))
assert pe['status']=='PASS' and pe['gate']['p8e_pass']
assert pe['execution']['dispatches']==15 and pe['execution']['submits']==1
assert pe['execution']['executed_layer']==0 and pe['execution']['executed_layer_count']==1
assert len(pe['checkpoints'])==17 and all(v['pass'] and v['finite_pass'] for v in pe['checkpoints'].values())

s=(ROOT/'src/p8f_two_layer_prefix_correctness.cpp').read_text(encoding='utf-8')
for x in [
 '#include <array>',
 'EXEC_LAYERS=2',
 'SEQ=4','MAXCTX=4096',
 'p8f_build_bindings',
 'bindings.size()!=339u||piece_count!=341u||multi_count!=2u',
 'ref[0]=build_ref(0u,x);',
 'ref[1]=build_ref(1u,ref[0].out);',
 'append_layer(0u,&b_x,gpu[0]);',
 'Buffer* layer1_input=&gpu[0].out;',
 'const bool direct_gpu_handoff=(layer1_input==&gpu[0].out);',
 'append_layer(1u,layer1_input,gpu[1]);',
 'if(ops.size()!=30u)',
 'if(ck.size()!=34u)',
 'stats.dispatch_count==30u',
 'stats.submit_count==1u',
 'arcllm.p8f.two_layer_prefix_correctness.v1'
]:
    assert x in s,x

assert s.count('append_layer(')==2
assert 'append_layer(2' not in s
assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s

gn0=s.index('static std::vector<std::string> p8f_graph_names()')
gn1=s.index('static std::map<std::string,P8FBinding> p8f_build_bindings',gn0)
assert gn0>=0 and gn1>gn0
assert 'for(uint32_t l=0;l<28u;++l)' in s[gn0:gn1]

ab0=s.index('auto append_layer=[&]')
ab1=s.index('append_layer(0u,&b_x,gpu[0]);',ab0)
assert ab0>=0 and ab1>ab0
builder=s[ab0:ab1]
assert builder.count('addop(')==15
assert 'for(uint32_t l=' not in builder

calls=s[ab1:s.index('if(ops.size()!=30u)',ab1)]
assert calls.count('append_layer(')==2
assert 'for(' not in calls
assert calls.index('append_layer(0u') < calls.index('layer1_input') < calls.index('append_layer(1u')

c=(ROOT/'tools/compile_p8f_shaders.ps1').read_text(encoding='ascii')
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert 'arcllm.p8f.shader_provenance.v1' in c
for sh in [
 'p7_rmsnorm_seq.comp','p7c_ffn_q4k_tiled.comp','p7c_ffn_q6k_tiled.comp',
 'p7_rope_seq.comp','p7_kv_store.comp','p7_attention_prefill_online.comp',
 'p7_add.comp','p7l_ffn_q4k_gateup_fused.comp','p7_swiglu.comp',
 'p7g_ffn_q4k_tiled16.comp','p7g_ffn_q6k_tiled16.comp'
]:
    assert sh in c,sh

b=(ROOT/'tools/build_p8f.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8f_two_layer_prefix_correctness.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs

r=(ROOT/'run_p8f.ps1').read_text(encoding='ascii')
for x in [
 'test_p8f_package.py','compile_p8f_shaders.ps1','build_p8f.ps1','arcllm_p8f.exe',
 'p8e_shader_provenance.authoritative.raw','p8e_single_layer_results.authoritative.raw','p8e_summary.authoritative.raw',
 '73916DE149A413B835541B95F01FEAF2B87DDDE03C039C3D1ABC0E2A3A115861',
 '992A986081FAFC81AC2E6E1A38063384434DE3138CAE469A04A57FED57BDA52B',
 'EBC4C8088B0992A30D72973DC7485CCF7AA0618B354509A506CC9FDE294B5B9D',
 'full_inference_permitted=$false',
 'larger_prefix_design_permitted=($Obj.status -eq "PASS")'
]:
    assert x in r,x

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-F' and m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8f_scope']['executed_layers']==[0,1] and m['p8f_scope']['executed_layer_count']==2
assert m['p8f_scope']['sequence_length']==4 and m['p8f_scope']['direct_gpu_layer_handoff'] is True
assert m['p8f_scope']['checkpoint_count']==34
assert m['gate']['exact_dispatches']==30 and m['gate']['exact_submits']==1 and m['gate']['exact_decoder_layers']==2
assert m['gate']['executed_layers']==[0,1]
assert m['gate']['full_inference_forbidden'] is True

print('ArcLLM P8-F static contract PASS')
