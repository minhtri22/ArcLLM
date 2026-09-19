from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]

for p in [
    'docs/P8G1_CONTRACT.md',
    'docs/P8G1_IMPLEMENTATION.md',
    'src/p8g1_causal_decomposition.cpp',
    'tools/compile_p8g1_shaders.ps1',
    'tools/build_p8g1.ps1',
    'run_p8g1.ps1',
    'inputs/p8g_shader_provenance.authoritative.raw',
    'inputs/p8g_four_layer_results.authoritative.raw',
    'inputs/p8g_summary.authoritative.raw',
]:
    assert (ROOT/p).is_file(), p

assert hashlib.sha256((ROOT/'inputs/p8g_shader_provenance.authoritative.raw').read_bytes()).hexdigest().upper() == 'C258C76816BDE57CC9D56A3C73955019716A1F01637A11D23197129CC53EA1B9'
assert hashlib.sha256((ROOT/'inputs/p8g_four_layer_results.authoritative.raw').read_bytes()).hexdigest().upper() == 'BDB7F4C1480CF960A89EBB29FACE58FC751BA8EF6AF76812F23A7C5ED0E94129'
assert hashlib.sha256((ROOT/'inputs/p8g_summary.authoritative.raw').read_bytes()).hexdigest().upper() == '0B1D4CF0355FBB7987217CC1DA8133BD81CA5332B37D5542E69A427A7C196BFE'

parent=json.loads((ROOT/'inputs/p8g_four_layer_results.authoritative.raw').read_text(encoding='utf-8'))
assert parent['status']=='FAIL'
assert parent['first_failing_checkpoint']=='L3.ffn_down'
assert parent['execution']['dispatches']==60
assert parent['execution']['submits']==1
assert parent['execution']['executed_layers']==[0,1,2,3]
assert parent['scope']['direct_gpu_handoff'] is True
assert parent['formats']['layer3']['ffn_down']=='Q4_K'
assert parent['checkpoints']['L3.swiglu']['pass'] is True
assert parent['checkpoints']['L3.ffn_down']['pass'] is False

s=(ROOT/'src/p8g1_causal_decomposition.cpp').read_text(encoding='utf-8')

for x in [
    'arcllm.p8g.four_layer_prefix_correctness.v1',
    'pr.find("\\\"first_failing_checkpoint\\\":\\\"L3.ffn_down\\\"")',
    'blk.3.ffn_down.weight',
    'focus_span_exact',
    'lt[3].dw->ggml_type!=Q4',
    'const std::vector<float>&X_CPU=ref[3].s;',
    'const std::vector<float> X_GPU=readf(gpu[3].s,X_CPU.size());',
    'const std::vector<float>&Y_CC=ref[3].d;',
    'const std::vector<float> Y_CG=matmul_q4_cpu',
    'Y_GC_P7G','Y_GG_P7G','Y_GC_P7C','Y_GG_P7C',
    'p7g_ffn_q4k_tiled16.spv',
    'p7c_ffn_q4k_tiled.spv',
    'if(prefix_ops.size()!=58u)',
    'if(diag_ops.size()!=4u)',
    'prefix_stats.dispatch_count==58u',
    'prefix_stats.submit_count==1u',
    'diag_stats.dispatch_count==4u',
    'diag_stats.submit_count==1u',
    'compare_vec(a,b,0.02,0.005)',
    'H-KERNEL','H-COMMON-Q4','H-AMPLIFICATION','H-INTERACTION',
    'INPUT_DEPENDENT_TILED_KERNEL_SENSITIVITY',
    'PARENT_REPRODUCTION_FAILED',
    'arcllm.p8g1.causal_decomposition.v1',
]:
    assert x in s, x

# Stage 1 must be exactly 3 full layers plus L3 through SwiGLU.
for call in [
    'append_prefix_layer(0u,&b_x,gpu[0],true);',
    'append_prefix_layer(1u,layer1_input,gpu[1],true);',
    'append_prefix_layer(2u,layer2_input,gpu[2],true);',
    'append_prefix_layer(3u,layer3_input,gpu[3],false);',
]:
    assert call in s, call
assert 'append_prefix_layer(4u' not in s

# The diagnostic stage has exactly four GPU calls: P7-G/P7-C x CPU/GPU input.
diag0=s.index('std::vector<DispatchOp> diag_ops;')
diag1=s.index('if(diag_ops.size()!=4u)',diag0)
diag=s[diag0:diag1]
assert diag.count('add_diag(')==4
assert diag.count('"p7g_ffn_q4k_tiled16.spv"')==2
assert diag.count('"p7c_ffn_q4k_tiled.spv"')==2

# Diagnostic CPU teacher forcing must never enter the prefix stage.
prefix0=s.index('std::vector<DispatchOp> prefix_ops;')
prefix1=s.index('if(prefix_ops.size()!=58u)',prefix0)
prefix=s[prefix0:prefix1]
assert 'b_x_cpu' not in prefix

# P8-G verdict cannot be modified by P8-G1.
assert '\\"p8g_verdict_frozen\\":\\"FAIL\\"' in s
assert '\\"p8g_verdict_changed\\":false' in s
assert '\\"p8h_permitted\\":false' in s
assert '\\"full_inference_permitted\\":false' in s
assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s

# Cleanup helper must exist before its invocation.
cleanup_def=s.index('auto destroy_layer=[&](P8GLayerBuffers&b)')
cleanup_call=s.index('for(int l=int(EXEC_LAYERS)-1;l>=0;--l)destroy_layer',cleanup_def)
assert cleanup_def>=0 and cleanup_call>cleanup_def

c=(ROOT/'tools/compile_p8g1_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p8g1.shader_provenance.v1' in c
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert c.count('.comp"')==11

b=(ROOT/'tools/build_p8g1.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8g1_causal_decomposition.cpp','gguf.cpp','tensor_store.cpp'], cpp_refs
assert 'arcllm_p8g1.exe' in b

r=(ROOT/'run_p8g1.ps1').read_text(encoding='ascii')
for h in [
    'C258C76816BDE57CC9D56A3C73955019716A1F01637A11D23197129CC53EA1B9',
    'BDB7F4C1480CF960A89EBB29FACE58FC751BA8EF6AF76812F23A7C5ED0E94129',
    '0B1D4CF0355FBB7987217CC1DA8133BD81CA5332B37D5542E69A427A7C196BFE',
]:
    assert h in r
assert 'p8g1_causal_decomposition_results.json' in r
assert 'p8g1.summary.v1' in r
assert 'p8g_verdict_frozen="FAIL"' in r
assert 'p8g_verdict_changed=$false' in r
assert 'p8h_permitted=$false' in r
assert 'full_inference_permitted=$false' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-G1'
assert m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8g_verdict_frozen']=='FAIL'
assert m['p8h_blocked'] is True
assert m['full_inference_forbidden'] is True
assert m['diagnosis']['focus_tensor']=='blk.3.ffn_down.weight'
assert m['diagnosis']['required_outputs']==['Y_CC','Y_CG','Y_GC_P7G','Y_GG_P7G','Y_GC_P7C','Y_GG_P7C']
assert m['implementation']['prefix_dispatches']==58
assert m['implementation']['prefix_submits']==1
assert m['implementation']['diagnostic_dispatches']==4
assert m['implementation']['diagnostic_submits']==1

print('ArcLLM P8-G1 static contract PASS')
