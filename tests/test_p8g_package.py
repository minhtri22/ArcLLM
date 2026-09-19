from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]

for p in [
    'docs/P8G_CONTRACT.md',
    'docs/P8G_IMPLEMENTATION.md',
    'src/p8g_four_layer_prefix_correctness.cpp',
    'tools/compile_p8g_shaders.ps1',
    'tools/build_p8g.ps1',
    'run_p8g.ps1',
    'inputs/p8f_shader_provenance.authoritative.raw',
    'inputs/p8f_two_layer_results.authoritative.raw',
    'inputs/p8f_summary.authoritative.raw',
]:
    assert (ROOT/p).is_file(), p

# Parent evidence: only byte-exact hashes and minimum semantic PASS facts.
assert hashlib.sha256((ROOT/'inputs/p8f_shader_provenance.authoritative.raw').read_bytes()).hexdigest().upper() == '9686F747B6A4D7380F4621B1A3EEC09B82DE7832461B1A9C80548D21D7D70A41'
assert hashlib.sha256((ROOT/'inputs/p8f_two_layer_results.authoritative.raw').read_bytes()).hexdigest().upper() == 'F172E1B0B00558BDE53EB6994BDC1E5BFFC417F96A33FA1C53B0983BB494AE98'
assert hashlib.sha256((ROOT/'inputs/p8f_summary.authoritative.raw').read_bytes()).hexdigest().upper() == '1F3EF56BC723FE2D8D8E567ADCB89BC23AE212CB0BBBB51CBC5B83E81DF9FC70'

parent=json.loads((ROOT/'inputs/p8f_two_layer_results.authoritative.raw').read_text(encoding='utf-8'))
assert parent['status']=='PASS'
assert parent['gate']['p8f_pass'] is True
assert parent['execution']['dispatches']==30
assert parent['execution']['submits']==1
assert parent['execution']['executed_layers']==[0,1]
assert len(parent['checkpoints'])==34
assert all(v['pass'] and v['finite_pass'] for v in parent['checkpoints'].values())

s=(ROOT/'src/p8g_four_layer_prefix_correctness.cpp').read_text(encoding='utf-8')

for x in [
    'EXEC_LAYERS=4',
    'SEQ=4','MAXCTX=4096',
    'p8g_build_bindings',
    'bindings.size()!=339u||piece_count!=341u||multi_count!=2u',
    'ref[0]=build_ref(0u,x);',
    'ref[1]=build_ref(1u,ref[0].out);',
    'ref[2]=build_ref(2u,ref[1].out);',
    'ref[3]=build_ref(3u,ref[2].out);',
    'append_layer(0u,&b_x,gpu[0]);',
    'append_layer(1u,layer1_input,gpu[1]);',
    'append_layer(2u,layer2_input,gpu[2]);',
    'append_layer(3u,layer3_input,gpu[3]);',
    'layer1_input=&gpu[0].out',
    'layer2_input=&gpu[1].out',
    'layer3_input=&gpu[2].out',
    'if(ops.size()!=60u)',
    'if(ck.size()!=68u)',
    'stats.dispatch_count==60u',
    'stats.submit_count==1u',
    'arcllm.p8g.four_layer_prefix_correctness.v1',
]:
    assert x in s, x

assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s

# Frozen per-layer builder remains exactly 15 operations.
ab0=s.index('auto append_layer=[&]')
ab1=s.index('append_layer(0u,&b_x,gpu[0]);',ab0)
builder=s[ab0:ab1]
assert builder.count('addop(')==15

# Execution calls are explicit and bounded to L0..L3.
exec_block=s[ab1:s.index('if(ops.size()!=60u)',ab1)]
assert exec_block.count('append_layer(')==4
assert 'append_layer(4' not in s

# Error and normal output belong to P8-G; P8-F schema remains only as parent check.
assert s.count('arcllm.p8g.four_layer_prefix_correctness.v1')==2
assert s.count('arcllm.p8f.two_layer_prefix_correctness.v1')==1

c=(ROOT/'tools/compile_p8g_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p8g.shader_provenance.v1' in c
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert c.count('.comp"')==11

b=(ROOT/'tools/build_p8g.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8g_four_layer_prefix_correctness.cpp','gguf.cpp','tensor_store.cpp'], cpp_refs

r=(ROOT/'run_p8g.ps1').read_text(encoding='ascii')
assert 'p8f_shader_provenance.authoritative.raw' in r
assert 'p8f_two_layer_results.authoritative.raw' in r
assert 'p8f_summary.authoritative.raw' in r
assert 'test_p8g_package.py' in r
assert 'compile_p8g_shaders.ps1' in r
assert 'build_p8g.ps1' in r
assert 'arcllm_p8g.exe' in r
assert 'full_inference_permitted=$false' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-G'
assert m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8g_scope']['executed_layers']==[0,1,2,3]
assert m['p8g_scope']['executed_layer_count']==4
assert m['p8g_scope']['checkpoint_count']==68
assert m['gate']['exact_dispatches']==60
assert m['gate']['exact_submits']==1
assert m['gate']['exact_decoder_layers']==4
assert m['gate']['full_inference_forbidden'] is True

print('ArcLLM P8-G static contract PASS')
