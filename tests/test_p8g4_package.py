from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]

for p in [
    'docs/P8G4_CONTRACT.md',
    'docs/P8G4_IMPLEMENTATION.md',
    'src/p8g4_fresh_compositional_decomposition.cpp',
    'tools/compile_p8g4_shaders.ps1',
    'tools/build_p8g4.ps1',
    'run_p8g4.ps1',
    'inputs/p8g3_shader_provenance.authoritative.raw',
    'inputs/p8g3_arithmetic_precision_results.authoritative.raw',
    'inputs/p8g3_summary.authoritative.raw',
]:
    assert (ROOT/p).is_file(), p

assert hashlib.sha256((ROOT/'inputs/p8g3_shader_provenance.authoritative.raw').read_bytes()).hexdigest().upper() == 'C10D0DB9444E584CDC76D939F5D134EC1229BD600CF3EED181C9D08505E955E8'
assert hashlib.sha256((ROOT/'inputs/p8g3_arithmetic_precision_results.authoritative.raw').read_bytes()).hexdigest().upper() == 'B0ADAAF9790018467C721599C2147AF7A6C0F69F0967B5C25F77631095571835'
assert hashlib.sha256((ROOT/'inputs/p8g3_summary.authoritative.raw').read_bytes()).hexdigest().upper() == 'EF494284E7BFBED541380267EDB197CF53F9EBB7E86040A83546735F84209C4D'

parent=json.loads((ROOT/'inputs/p8g3_arithmetic_precision_results.authoritative.raw').read_text(encoding='utf-8'))
assert parent['status']=='COMPLETE'
assert parent['diagnostic_valid'] is True
assert parent['attribution']['classification']=='H-FP32-ACCUMULATION'
assert parent['attribution']['closure_earliest_pass']=='R1'
assert parent['attribution']['alpha_earliest_pass']=='R1'
assert parent['execution']['prefix_dispatches']==58
assert parent['execution']['prefix_submits']==1
assert parent['execution']['precision_controls_cpu_only'] is True
assert parent['execution']['layer4_plus_executed'] is False

s=(ROOT/'src/p8g4_fresh_compositional_decomposition.cpp').read_text(encoding='utf-8')

for x in [
    'arcllm.p8g3.arithmetic_precision_attribution.v1',
    'pr.find("\\\"classification\\\":\\\"H-FP32-ACCUMULATION\\\"")',
    'arcllm.p8g3.shader_provenance.v1',
    'arcllm.p8g3.summary.v1',
    'blk.3.ffn_down.weight',
    'focus_span_exact',
    'lt[3].dw->ggml_type!=Q4',
    'const std::array<uint32_t,4> seed_ids={17u,29u,43u,61u};',
    '0.13*std::sin((di+11.0+37.0*ds)*0.009)',
    '0.04*std::cos((di+5.0+19.0*ds)*0.017)',
    '0.02*std::sin((di+3.0+23.0*ds)*0.0043)',
    'p8g4_r1_down_pair',
    'const float prod_cpu=wf*x_cpu[xi];',
    'const float prod_gpu=wf*x_gpu[xi];',
    'sum_cpu+=double(prod_cpu);',
    'sum_gpu+=double(prod_gpu);',
    'out.y_cpu[oi]=float(sum_cpu);',
    'out.y_gpu[oi]=float(sum_gpu);',
    'E_state','E_local','E_total','E_reconstructed',
    'compare_vec(E_total,E_reconstructed,1e-5,1e-7)',
    'compare_vec(Y_CG,Y_GG,0.02,0.005)',
    'o.local_to_state_max<=0.05&&o.local_to_state_rms<=0.05',
    'H-COMPOSITIONAL-SEPARATION-REPLICATED',
    'H-HETEROGENEOUS-SEPARATION',
    'H-LOCAL-ERROR-NONNEGLIGIBLE',
    'DECOMPOSITION-INVALID',
    'arcllm.p8g4.fresh_compositional_decomposition.v1',
]:
    assert x in s, x

# CPU reference stops L3 at SwiGLU.
assert 'auto build_ref=[&](uint32_t l,const std::vector<float>&input,bool include_down)' in s
for call in [
    'ref[0]=build_ref(0u,x,true);',
    'ref[1]=build_ref(1u,ref[0].out,true);',
    'ref[2]=build_ref(2u,ref[1].out,true);',
    'ref[3]=build_ref(3u,ref[2].out,false);',
]:
    assert call in s, call

# GPU prefix remains frozen and local chain is exactly one down dispatch.
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

l0=s.index('std::vector<DispatchOp> local_ops;')
l1=s.index('if(local_ops.size()!=1u)',l0)
local=s[l0:l1]
assert local.count('local_ops.push_back(')==1
assert '"p7g_ffn_q4k_tiled16.spv"' in local
assert 'local_stats.dispatch_count==1u' in s
assert 'local_stats.submit_count==1u' in s
assert s.count('vk.execute_prepared(')==2

# The exact fresh input is copied into the prepared prefix input buffer.
assert 'std::memcpy(b_x.mapped,x.data(),x.size()*sizeof(float));' in s
assert '0.17f*std::sin' not in s

# D3 is descriptive and excluded from decision variables.
assert '\\"decision_role\\":\\"descriptive_only\\"' in s
assert 'o.seed_pass=o.d0&&o.d1&&o.d2;' in s
class0=s.index('std::string classification;')
class1=s.index('const bool diagnostic_valid',class0)
classification=s[class0:class1]
assert 'historical_total_gate_pass' not in classification

# Structural invalidity or any D0 failure must force DECOMPOSITION-INVALID.
assert 'if(!structural_valid||!all_d0)classification="DECOMPOSITION-INVALID";' in s

# Historical governance cannot move.
for x in [
    '\\"replacement_gate_defined\\":false',
    '\\"p8g_verdict_changed\\":false',
    '\\"p8g1_verdict_changed\\":false',
    '\\"p8g2_verdict_changed\\":false',
    '\\"p8g3_verdict_changed\\":false',
    '\\"p8h_permitted\\":false',
    '\\"full_inference_permitted\\":false',
]:
    assert x in s, x
assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s

# Prepared chains are created/destroyed once and reused across seeds.
assert s.count('vk.prepare_chain(prefix_ops)')==1
assert s.count('vk.prepare_chain(local_ops)')==1
assert s.count('vk.destroy_prepared(local_chain)')==1
assert s.count('vk.destroy_prepared(prefix_chain)')==1

cleanup_def=s.index('auto destroy_layer=[&](P8GLayerBuffers&b)')
cleanup_call=s.index('for(int l=int(EXEC_LAYERS)-1;l>=0;--l)destroy_layer',cleanup_def)
assert cleanup_def>=0 and cleanup_call>cleanup_def

c=(ROOT/'tools/compile_p8g4_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p8g4.shader_provenance.v1' in c
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert c.count('.comp"')==11

b=(ROOT/'tools/build_p8g4.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8g4_fresh_compositional_decomposition.cpp','gguf.cpp','tensor_store.cpp'], cpp_refs
assert 'arcllm_p8g4.exe' in b

r=(ROOT/'run_p8g4.ps1').read_text(encoding='ascii')
for h in [
    'C10D0DB9444E584CDC76D939F5D134EC1229BD600CF3EED181C9D08505E955E8',
    'B0ADAAF9790018467C721599C2147AF7A6C0F69F0967B5C25F77631095571835',
    'EF494284E7BFBED541380267EDB197CF53F9EBB7E86040A83546735F84209C4D',
]:
    assert h in r
assert 'p8g4_fresh_compositional_results.json' in r
assert 'arcllm.p8g4.summary.v1' in r
assert 'fresh_input_ids=@(17,29,43,61)' in r
assert 'p8g_verdict_frozen="FAIL"' in r
assert 'p8g3_classification_frozen="H-FP32-ACCUMULATION"' in r
assert 'historical_total_gate_decision_role="descriptive_only"' in r
assert 'replacement_gate_defined=$false' in r
assert 'p8h_permitted=$false' in r
assert 'full_inference_permitted=$false' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-G4'
assert m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8g_verdict_frozen']=='FAIL'
assert m['p8g1_classification_frozen']=='H-AMPLIFICATION'
assert m['p8g2_classification_frozen']=='H-NONLINEAR/UNEXPLAINED'
assert m['p8g3_classification_frozen']=='H-FP32-ACCUMULATION'
assert m['p8h_blocked'] is True
assert m['full_inference_forbidden'] is True
assert m['p8g4']['fresh_input_ids']==[17,29,43,61]
assert m['p8g4']['historical_total_gate_decision_role']=='descriptive_only'
assert m['p8g4']['replacement_gate_definition_permitted'] is False
assert m['implementation']['prefix_dispatches_per_seed']==58
assert m['implementation']['prefix_submits_per_seed']==1
assert m['implementation']['local_down_dispatches_per_seed']==1
assert m['implementation']['local_down_submits_per_seed']==1
assert m['implementation']['r1_oracle_cpu_only'] is True
assert m['implementation']['target_experiment_run_in_lock_commit'] is False

print('ArcLLM P8-G4 static contract PASS')
