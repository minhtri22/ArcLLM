from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]

for p in [
    'docs/P8G2_CONTRACT.md',
    'docs/P8G2_IMPLEMENTATION.md',
    'src/p8g2_amplification_geometry.cpp',
    'tools/compile_p8g2_shaders.ps1',
    'tools/build_p8g2.ps1',
    'run_p8g2.ps1',
    'inputs/p8g1_shader_provenance.authoritative.raw',
    'inputs/p8g1_causal_decomposition_results.authoritative.raw',
    'inputs/p8g1_summary.authoritative.raw',
]:
    assert (ROOT/p).is_file(), p

assert hashlib.sha256((ROOT/'inputs/p8g1_shader_provenance.authoritative.raw').read_bytes()).hexdigest().upper() == 'A4B095E1F2E7CD4AD78EE74C06A96323DF258C2ADB2CCA7DE827816191019740'
assert hashlib.sha256((ROOT/'inputs/p8g1_causal_decomposition_results.authoritative.raw').read_bytes()).hexdigest().upper() == '416A97CD13A585BD4CAB397A3B3503BA8A87BD94F13BB1F5664F384603359499'
assert hashlib.sha256((ROOT/'inputs/p8g1_summary.authoritative.raw').read_bytes()).hexdigest().upper() == '1E04C9BF94DA57258D9DEA36FAE3F71B896D9F0CA773BF360305742397C27B1C'

parent=json.loads((ROOT/'inputs/p8g1_causal_decomposition_results.authoritative.raw').read_text(encoding='utf-8'))
assert parent['status']=='COMPLETE'
assert parent['diagnostic_valid'] is True
assert parent['adjudication']['classification']=='H-AMPLIFICATION'
assert parent['p8g_verdict_frozen']=='FAIL'
assert parent['reproduction']['parent_reproduction_pass'] is True

s=(ROOT/'src/p8g2_amplification_geometry.cpp').read_text(encoding='utf-8')

for x in [
    'arcllm.p8g1.causal_decomposition.v1',
    'pr.find("\\\"classification\\\":\\\"H-AMPLIFICATION\\\"")',
    'blk.3.ffn_down.weight',
    'focus_span_exact',
    'lt[3].dw->ggml_type!=Q4',
    'const std::vector<float>&X_CPU=ref[3].s;',
    'const std::vector<float> X_GPU=readf(gpu[3].s,X_CPU.size());',
    'const std::vector<float>&Y_CC=ref[3].d;',
    'const std::vector<float> dX=diff(X_GPU,X_CPU);',
    'const std::vector<float> Y_CG=matmul_q4_cpu',
    'const std::vector<float> dY_prop=diff(Y_CG,Y_CC);',
    'const std::vector<float> dY_direct=matmul_q4_cpu',
    'if(prefix_ops.size()!=58u)',
    'prefix_stats.dispatch_count==58u',
    'prefix_stats.submit_count==1u',
    'kernel_ops.size()!=1u',
    'kernel_stats.dispatch_count==1u',
    'kernel_stats.submit_count==1u',
    'compare_vec(dY_prop,dY_direct,1e-4,1e-6)',
    'kernel_max_ratio<=0.01',
    'kernel_rms_ratio<=0.01',
    'const std::array<double,4> alphas={0.25,0.50,0.75,1.00};',
    'if(emax>0.01||erms>0.01)a4_pass=false;',
    'H-GEOMETRIC-AMPLIFICATION',
    'H-NONLINEAR/UNEXPLAINED',
    'H-KERNEL-CONTRIBUTION',
    'PARENT-REPRODUCTION-FAILED',
    'arcllm.p8g2.amplification_geometry.v1',
]:
    assert x in s, x

for call in [
    'append_prefix_layer(0u,&b_x,gpu[0],true);',
    'append_prefix_layer(1u,layer1_input,gpu[1],true);',
    'append_prefix_layer(2u,layer2_input,gpu[2],true);',
    'append_prefix_layer(3u,layer3_input,gpu[3],false);',
]:
    assert call in s, call
assert 'append_prefix_layer(4u' not in s

assert s.count('kernel_ops.push_back(')==1
assert 'p7g_ffn_q4k_tiled16.spv' in s

assert '\\"replacement_gate_defined\\":false' in s
assert '\\"p8g_verdict_changed\\":false' in s
assert '\\"p8h_permitted\\":false' in s
assert '\\"full_inference_permitted\\":false' in s
assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s

cleanup_def=s.index('auto destroy_layer=[&](P8GLayerBuffers&b)')
cleanup_call=s.index('for(int l=int(EXEC_LAYERS)-1;l>=0;--l)destroy_layer',cleanup_def)
assert cleanup_def>=0 and cleanup_call>cleanup_def

c=(ROOT/'tools/compile_p8g2_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p8g2.shader_provenance.v1' in c
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert c.count('.comp"')==11

b=(ROOT/'tools/build_p8g2.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8g2_amplification_geometry.cpp','gguf.cpp','tensor_store.cpp'], cpp_refs
assert 'arcllm_p8g2.exe' in b

r=(ROOT/'run_p8g2.ps1').read_text(encoding='ascii')
for h in [
    'A4B095E1F2E7CD4AD78EE74C06A96323DF258C2ADB2CCA7DE827816191019740',
    '416A97CD13A585BD4CAB397A3B3503BA8A87BD94F13BB1F5664F384603359499',
    '1E04C9BF94DA57258D9DEA36FAE3F71B896D9F0CA773BF360305742397C27B1C',
]:
    assert h in r
assert 'p8g2_amplification_geometry_results.json' in r
assert 'arcllm.p8g2.summary.v1' in r
assert 'p8g_verdict_frozen="FAIL"' in r
assert 'p8g1_classification_frozen="H-AMPLIFICATION"' in r
assert 'replacement_gate_defined=$false' in r
assert 'p8h_permitted=$false' in r
assert 'full_inference_permitted=$false' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-G2'
assert m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8g_verdict_frozen']=='FAIL'
assert m['p8g1_classification_frozen']=='H-AMPLIFICATION'
assert m['p8h_blocked'] is True
assert m['full_inference_forbidden'] is True
assert m['p8g2']['alpha_series']==[0.25,0.5,0.75,1]
assert m['p8g2']['linear_closure_max_abs']==0.0001
assert m['p8g2']['linear_closure_rmse']==0.000001
assert m['p8g2']['kernel_contribution_ratio_max']==0.01
assert m['p8g2']['scaling_ratio_tolerance_fraction']==0.01
assert m['implementation']['prefix_dispatches']==58
assert m['implementation']['prefix_submits']==1
assert m['implementation']['kernel_residual_dispatches']==1
assert m['implementation']['kernel_residual_submits']==1
assert m['implementation']['replacement_gate_definition_permitted'] is False

print('ArcLLM P8-G2 static contract PASS')
