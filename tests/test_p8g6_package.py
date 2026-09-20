from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]

for p in [
    'docs/P8G6_CONTRACT.md',
    'docs/P8G6_IMPLEMENTATION.md',
    'src/p8g6_fresh_production_semantic_confirmation.cpp',
    'tools/compile_p8g6_shaders.ps1',
    'tools/build_p8g6.ps1',
    'run_p8g6.ps1',
    'inputs/p8g5_shader_provenance.authoritative.raw',
    'inputs/p8g5_production_semantic_attribution_results.authoritative.raw',
    'inputs/p8g5_summary.authoritative.raw',
]:
    assert (ROOT/p).is_file(), p

assert hashlib.sha256((ROOT/'inputs/p8g5_shader_provenance.authoritative.raw').read_bytes()).hexdigest().upper() == '6483D82540EC31F3CE058B51FC48C9BABFED9983F7F59A090D9AE14917176F9A'
assert hashlib.sha256((ROOT/'inputs/p8g5_production_semantic_attribution_results.authoritative.raw').read_bytes()).hexdigest().upper() == '64565C94AD2D9EF85D1DFF8CE972FD263C2C1B4A28A28B5F6830F6EB9AD6E096'
assert hashlib.sha256((ROOT/'inputs/p8g5_summary.authoritative.raw').read_bytes()).hexdigest().upper() == '1732663E0A9CF90848D54487E2C2D031EB1A5C2A90808A34ABB1C06D57995751'

parent=json.loads((ROOT/'inputs/p8g5_production_semantic_attribution_results.authoritative.raw').read_text(encoding='utf-8'))
assert parent['status']=='COMPLETE'
assert parent['diagnostic_valid'] is True
assert parent['adjudication']['classification']=='H-R1-ORACLE-SEMANTIC-MISMATCH'
for k in ['all_p0','all_p1','all_p2','all_p3_closure','all_p3_dominance','structural_valid']:
    assert parent['adjudication'][k] is True, k
assert parent['governance']['retrospective_attribution_only'] is True

s=(ROOT/'src/p8g6_fresh_production_semantic_confirmation.cpp').read_text(encoding='utf-8')

for x in [
    'arcllm.p8g5.production_semantic_local_attribution.v1',
    'pr.find("\\\"classification\\\":\\\"H-R1-ORACLE-SEMANTIC-MISMATCH\\\"")',
    'arcllm.p8g5.shader_provenance.v1',
    'arcllm.p8g5.summary.v1',
    'const std::array<uint32_t,4> seed_ids={73u,89u,107u,131u};',
    '0.13*std::sin((di+11.0+37.0*ds)*0.009)',
    '0.04*std::cos((di+5.0+19.0*ds)*0.017)',
    '0.02*std::sin((di+3.0+23.0*ds)*0.0043)',
    'float r0c=0.0f,r0g=0.0f;',
    'double r1c=0.0,r1g=0.0;',
    'r0c+=pc;r0g+=pg;',
    'r1c+=double(pc);r1g+=double(pg);',
    'const Metrics c1m=compare_vec(oracle.r0_gpu,Y_GPU,1e-4,1e-6);',
    'const Metrics c2m=compare_vec(E_total_R0,E_reconstructed_R0,1e-5,1e-7);',
    'local_to_state_max<=0.05&&local_to_state_rms<=0.05',
    'H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED',
    'H-FRESH-RATIO-SENSITIVITY',
    'H-FRESH-PRODUCTION-LOCAL-RESIDUAL',
    'CONFIRMATION-INVALID',
    'arcllm.p8g6.fresh_production_semantic_confirmation.v1',
]:
    assert x in s, x

# Old cohort must not be the executable seed array.
assert 'const std::array<uint32_t,4> seed_ids={17u,29u,43u,61u};' not in s
assert '\\"previous_ids\\":[17,29,43,61]' in s
assert '\\"ids\\":[73,89,107,131]' in s

# Exact CPU/GPU scope.
for call in [
    'ref[0]=build_ref(0u,x,true);',
    'ref[1]=build_ref(1u,ref[0].out,true);',
    'ref[2]=build_ref(2u,ref[1].out,true);',
    'ref[3]=build_ref(3u,ref[2].out,false);',
    'append_prefix_layer(0u,&b_x,gpu[0],true);',
    'append_prefix_layer(1u,layer1_input,gpu[1],true);',
    'append_prefix_layer(2u,layer2_input,gpu[2],true);',
    'append_prefix_layer(3u,layer3_input,gpu[3],false);',
]:
    assert call in s, call
assert 'append_prefix_layer(4u' not in s
assert 'prefix_stats.dispatch_count==58u' in s
assert 'prefix_stats.submit_count==1u' in s
assert 'local_stats.dispatch_count==1u' in s
assert 'local_stats.submit_count==1u' in s
assert s.count('vk.execute_prepared(')==2

# R1 is descriptive-only: it may appear in C0 finiteness, but not in C1/C2/C3 or classification.
c1a=s.index('const Metrics c1m=compare_vec(oracle.r0_gpu,Y_GPU,1e-4,1e-6);')
c1b=s.index('// Descriptive-only diagnostics:',c1a)
decision_metrics=s[c1a:c1b]
assert 'oracle.r1_' not in decision_metrics
assert 'E_state_R1' not in decision_metrics
assert 'E_local_R1' not in decision_metrics
class0=s.index('std::string classification;')
class1=s.index('const bool diagnostic_valid',class0)
classification=s[class0:class1]
assert 'R1' not in classification
assert 'r1_' not in classification
assert 'historical_total_gate_pass' not in classification

# C0 finite R1 is allowed and required by the frozen contract.
assert 'const bool finite_r1=p8g_finite(oracle.r1_cpu)&&p8g_finite(oracle.r1_gpu);' in s
assert 'const bool c0=finite_primary&&finite_r1&&exec_ok;' in s

# Exact classification order.
assert 'if(!structural_valid||!all_c0||!all_c2)classification="CONFIRMATION-INVALID";' in classification
assert 'else if(!all_c1)classification="H-FRESH-PRODUCTION-LOCAL-RESIDUAL";' in classification
assert 'else if(all_c3)classification="H-PRODUCTION-SEMANTIC-SEPARATION-CONFIRMED";' in classification
assert 'else classification="H-FRESH-RATIO-SENSITIVITY";' in classification

# Descriptive-only fields are emitted but cannot drive adjudication.
for x in [
    '\\"role\\":\\"descriptive_only\\"',
    '\\"r1_decision_role\\":\\"descriptive_only\\"',
    'historical_total_gate_pass',
    'R0_to_R1_CPU',
    'R0_to_R1_GPU',
    'state_to_signal',
    'local_to_signal',
]:
    assert x in s, x

# Prepared graph reuse.
assert s.count('vk.prepare_chain(prefix_ops)')==1
assert s.count('vk.prepare_chain(local_ops)')==1
assert s.count('vk.destroy_prepared(local_chain)')==1
assert s.count('vk.destroy_prepared(prefix_chain)')==1

# Governance.
for x in [
    '\\"replacement_gate_defined\\":false',
    '\\"p8g_verdict_changed\\":false',
    '\\"p8g1_verdict_changed\\":false',
    '\\"p8g2_verdict_changed\\":false',
    '\\"p8g3_verdict_changed\\":false',
    '\\"p8g4_verdict_changed\\":false',
    '\\"p8g5_verdict_changed\\":false',
    '\\"p8h_permitted\\":false',
    '\\"full_inference_permitted\\":false',
]:
    assert x in s, x
assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s

c=(ROOT/'tools/compile_p8g6_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p8g6.shader_provenance.v1' in c
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert c.count('.comp"')==11

b=(ROOT/'tools/build_p8g6.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8g6_fresh_production_semantic_confirmation.cpp','gguf.cpp','tensor_store.cpp'], cpp_refs
assert 'arcllm_p8g6.exe' in b

r=(ROOT/'run_p8g6.ps1').read_text(encoding='ascii')
for h in [
    '6483D82540EC31F3CE058B51FC48C9BABFED9983F7F59A090D9AE14917176F9A',
    '64565C94AD2D9EF85D1DFF8CE972FD263C2C1B4A28A28B5F6830F6EB9AD6E096',
    '1732663E0A9CF90848D54487E2C2D031EB1A5C2A90808A34ABB1C06D57995751',
]:
    assert h in r
assert 'p8g6_fresh_production_semantic_results.json' in r
assert 'arcllm.p8g6.summary.v1' in r
assert 'fresh_cohort_ids=@(73,89,107,131)' in r
assert 'previous_cohort_ids=@(17,29,43,61)' in r
assert 'primary_oracle="R0"' in r
assert 'r1_decision_role="descriptive_only"' in r
assert 'p8g5_classification_frozen="H-R1-ORACLE-SEMANTIC-MISMATCH"' in r
assert 'replacement_gate_defined=$false' in r
assert 'p8h_permitted=$false' in r
assert 'full_inference_permitted=$false' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-G6'
assert m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8g6']['fresh_cohort_ids']==[73,89,107,131]
assert m['p8g6']['previous_cohort_ids']==[17,29,43,61]
assert m['p8g6']['r1_decision_role']=='descriptive_only'
assert m['p8g6']['state_dominance_ratio_max']==0.05
assert m['p8g6']['replacement_gate_definition_permitted'] is False
assert m['p8g_verdict_frozen']=='FAIL'
assert m['p8g5_classification_frozen']=='H-R1-ORACLE-SEMANTIC-MISMATCH'
assert m['p8h_blocked'] is True
assert m['full_inference_forbidden'] is True
assert m['implementation']['r0_primary'] is True
assert m['implementation']['r1_decision_role']=='descriptive_only'
assert m['implementation']['prefix_dispatches_per_seed']==58
assert m['implementation']['local_down_dispatches_per_seed']==1
assert m['implementation']['target_experiment_run_in_lock_commit'] is False

print('ArcLLM P8-G6 static contract PASS')
