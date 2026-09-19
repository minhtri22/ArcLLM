from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]

for p in [
    'docs/P8G3_CONTRACT.md',
    'docs/P8G3_IMPLEMENTATION.md',
    'src/p8g3_arithmetic_precision_attribution.cpp',
    'tools/compile_p8g3_shaders.ps1',
    'tools/build_p8g3.ps1',
    'run_p8g3.ps1',
    'inputs/p8g2_shader_provenance.authoritative.raw',
    'inputs/p8g2_amplification_geometry_results.authoritative.raw',
    'inputs/p8g2_summary.authoritative.raw',
]:
    assert (ROOT/p).is_file(), p

assert hashlib.sha256((ROOT/'inputs/p8g2_shader_provenance.authoritative.raw').read_bytes()).hexdigest().upper() == '43A8DEA2AD14FF516B7FDF3EB69EC3D807C455F84D53E67C7BBF00A20C4993F4'
assert hashlib.sha256((ROOT/'inputs/p8g2_amplification_geometry_results.authoritative.raw').read_bytes()).hexdigest().upper() == '2D183D80DD4A63CC75E10D2DC42BE08D7606B147A39FC15021E1D52E569CA168'
assert hashlib.sha256((ROOT/'inputs/p8g2_summary.authoritative.raw').read_bytes()).hexdigest().upper() == 'B48FC0B48CC94363C24C0FD772058AC46C8BD3ED32203039F7FDEE230B0CB8A6'

parent=json.loads((ROOT/'inputs/p8g2_amplification_geometry_results.authoritative.raw').read_text(encoding='utf-8'))
assert parent['status']=='COMPLETE'
assert parent['diagnostic_valid'] is True
assert parent['adjudication']['classification']=='H-NONLINEAR/UNEXPLAINED'
assert parent['A0_parent_reproduction']['pass'] is True
assert parent['A2_linear_closure']['pass'] is False
assert parent['A3_kernel_residual']['pass'] is True
assert parent['A4_linearity']['pass'] is False
assert parent['execution']['prefix_dispatches']==58
assert parent['execution']['prefix_submits']==1
assert parent['execution']['layer4_plus_executed'] is False

s=(ROOT/'src/p8g3_arithmetic_precision_attribution.cpp').read_text(encoding='utf-8')

for x in [
    'arcllm.p8g2.amplification_geometry.v1',
    'H-NONLINEAR/UNEXPLAINED',
    'blk.3.ffn_down.weight',
    'focus_span_exact',
    'lt[3].dw->ggml_type!=Q4',
    'p8g3_eval_q4',
    'float s0[7]',
    'double s1[7]',
    'double s2[7]',
    'double s3[5]',
    'const float prod=wf*xv;',
    's0[j]+=prod',
    's1[j]+=double(prod)',
    's2[j]+=double(wf)*double(xv)',
    's3[j]+=double(wf)*xd[j][xi]',
    'out.r1[j][oi]=float(s1[j])',
    'dXf[i]=X_GPU[i]-X_CPU[i]',
    'dXd[i]=double(X_GPU[i])-double(X_CPU[i])',
    'xd[1+ai][i]=double(X_CPU[i])+a*dXd[i]',
    'const std::array<double,4> alphas={0.25,0.50,0.75,1.00};',
    'R0_FP32_BASELINE',
    'R1_DOUBLE_ACCUM_FLOAT_PRODUCT_FLOAT_OUTPUT',
    'R2_DOUBLE_DOT_FLOAT_INPUT_ALGEBRA',
    'R3_DOUBLE_INPUT_ALGEBRA',
    'H-FP32-ACCUMULATION',
    'H-DOT-ROUNDING',
    'H-INPUT-ROUNDING',
    'H-MIXED-FINITE-PRECISION',
    'H-HIGH-PRECISION-UNEXPLAINED',
    'PARENT-REPRODUCTION-FAILED',
    'arcllm.p8g3.arithmetic_precision_attribution.v1',
]:
    assert x in s, x

# Frozen prefix only.
for call in [
    'append_prefix_layer(0u,&b_x,gpu[0],true);',
    'append_prefix_layer(1u,layer1_input,gpu[1],true);',
    'append_prefix_layer(2u,layer2_input,gpu[2],true);',
    'append_prefix_layer(3u,layer3_input,gpu[3],false);',
]:
    assert call in s, call
assert 'append_prefix_layer(4u' not in s
assert 'if(prefix_ops.size()!=58u)' in s
assert 'prefix_stats.dispatch_count==58u' in s
assert 'prefix_stats.submit_count==1u' in s

# Precision attribution is CPU-only; no second diagnostic GPU chain exists.
assert 'kernel_ops' not in s
assert 'kernel_chain' not in s
assert s.count('vk.execute_prepared(')==1

# Frozen P8-G2 criteria remain exact.
assert s.count('1e-4,1e-6')>=4
assert 'if(em>0.01||er>0.01)pass=false;' in s
assert 'if(em>0.001||er>0.001)r0_reproduced=false;' in s
for v in [
    '0.000383861362934','0.0000055805988593',
    '0.00718688964844','0.0143051147461','0.0212173461914','0.0283355712891',
]:
    assert v in s

# Fused evaluator may parallelize independent output coordinates only.
assert '#include <thread>' in s
assert 'out.cpu_workers=(std::min)(8u,hw);' in s
assert 'for(uint32_t ib=0;ib<nb;++ib)' in s
assert 'for(uint32_t k=0;k<256u;++k)' in s

# Historical governance is immutable.
for x in [
    '\\"replacement_gate_defined\\":false',
    '\\"p8g_verdict_changed\\":false',
    '\\"p8g1_verdict_changed\\":false',
    '\\"p8g2_verdict_changed\\":false',
    '\\"p8h_permitted\\":false',
    '\\"full_inference_permitted\\":false',
]:
    assert x in s, x
assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s

cleanup_def=s.index('auto destroy_layer=[&](P8GLayerBuffers&b)')
cleanup_call=s.index('for(int l=int(EXEC_LAYERS)-1;l>=0;--l)destroy_layer',cleanup_def)
assert cleanup_def>=0 and cleanup_call>cleanup_def

c=(ROOT/'tools/compile_p8g3_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p8g3.shader_provenance.v1' in c
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert c.count('.comp"')==11

b=(ROOT/'tools/build_p8g3.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8g3_arithmetic_precision_attribution.cpp','gguf.cpp','tensor_store.cpp'], cpp_refs
assert 'arcllm_p8g3.exe' in b

r=(ROOT/'run_p8g3.ps1').read_text(encoding='ascii')
for h in [
    '43A8DEA2AD14FF516B7FDF3EB69EC3D807C455F84D53E67C7BBF00A20C4993F4',
    '2D183D80DD4A63CC75E10D2DC42BE08D7606B147A39FC15021E1D52E569CA168',
    'B48FC0B48CC94363C24C0FD772058AC46C8BD3ED32203039F7FDEE230B0CB8A6',
]:
    assert h in r
assert 'p8g3_arithmetic_precision_results.json' in r
assert 'arcllm.p8g3.summary.v1' in r
assert 'p8g_verdict_frozen="FAIL"' in r
assert 'p8g1_classification_frozen="H-AMPLIFICATION"' in r
assert 'p8g2_classification_frozen="H-NONLINEAR/UNEXPLAINED"' in r
assert 'replacement_gate_defined=$false' in r
assert 'p8h_permitted=$false' in r
assert 'full_inference_permitted=$false' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-G3'
assert m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8g_verdict_frozen']=='FAIL'
assert m['p8g1_classification_frozen']=='H-AMPLIFICATION'
assert m['p8g2_classification_frozen']=='H-NONLINEAR/UNEXPLAINED'
assert m['p8h_blocked'] is True
assert m['full_inference_forbidden'] is True
assert m['p8g3']['alpha_series']==[0.25,0.5,0.75,1]
assert m['p8g3']['replacement_gate_definition_permitted'] is False
assert m['implementation']['prefix_dispatches']==58
assert m['implementation']['prefix_submits']==1
assert m['implementation']['precision_controls_cpu_only'] is True
assert m['implementation']['target_experiment_run_in_lock_commit'] is False

print('ArcLLM P8-G3 static contract PASS')
