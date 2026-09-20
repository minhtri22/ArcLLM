from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[1]

for p in [
    'docs/P8G5_CONTRACT.md',
    'docs/P8G5_IMPLEMENTATION.md',
    'src/p8g5_production_semantic_attribution.cpp',
    'tools/compile_p8g5_shaders.ps1',
    'tools/build_p8g5.ps1',
    'run_p8g5.ps1',
    'inputs/p8g4_shader_provenance.authoritative.raw',
    'inputs/p8g4_fresh_compositional_results.authoritative.raw',
    'inputs/p8g4_summary.authoritative.raw',
]:
    assert (ROOT/p).is_file(), p

assert hashlib.sha256((ROOT/'inputs/p8g4_shader_provenance.authoritative.raw').read_bytes()).hexdigest().upper() == '1C146FCDD60D14A782512687865AC403D6DEE5C72FC125F8A74C5D4A920D5C7C'
assert hashlib.sha256((ROOT/'inputs/p8g4_fresh_compositional_results.authoritative.raw').read_bytes()).hexdigest().upper() == '9255B70BA50DF316B7D8BA04CB6696DA9FB2CB97D82F7A559DCF166F7E76E757'
assert hashlib.sha256((ROOT/'inputs/p8g4_summary.authoritative.raw').read_bytes()).hexdigest().upper() == '6347545710333A3CF9856399DF040E57A6C140E3463C1E50E5A8A432AF82FFD0'

parent=json.loads((ROOT/'inputs/p8g4_fresh_compositional_results.authoritative.raw').read_text(encoding='utf-8'))
assert parent['status']=='COMPLETE'
assert parent['diagnostic_valid'] is True
assert parent['cohort']['classification']=='H-LOCAL-ERROR-NONNEGLIGIBLE'
assert parent['cohort']['seed_pass_count']==0
assert parent['cohort']['all_d0_pass'] is True
assert parent['cohort']['structural_valid'] is True
assert [x['id'] for x in parent['seeds']]==[17,29,43,61]
assert all(x['D0']['pass'] for x in parent['seeds'])
assert all(x['D1_local_historical_gate']['pass'] for x in parent['seeds'])
assert all(not x['D2_state_dominance']['pass'] for x in parent['seeds'])

s=(ROOT/'src/p8g5_production_semantic_attribution.cpp').read_text(encoding='utf-8')

for x in [
    'arcllm.p8g4.fresh_compositional_decomposition.v1',
    'pr.find("\\\"classification\\\":\\\"H-LOCAL-ERROR-NONNEGLIGIBLE\\\"")',
    'arcllm.p8g4.shader_provenance.v1',
    'arcllm.p8g4.summary.v1',
    'const std::array<uint32_t,4> seed_ids={17u,29u,43u,61u};',
    '0.13*std::sin((di+11.0+37.0*ds)*0.009)',
    '0.04*std::cos((di+5.0+19.0*ds)*0.017)',
    '0.02*std::sin((di+3.0+23.0*ds)*0.0043)',
    'float r0c=0.0f,r0g=0.0f;',
    'double r1c=0.0,r1g=0.0;',
    'const float pc=wf*x_cpu[xi];',
    'const float pg=wf*x_gpu[xi];',
    'r0c+=pc;r0g+=pg;',
    'r1c+=double(pc);r1g+=double(pg);',
    'out.r0_cpu[oi]=r0c;out.r0_gpu[oi]=r0g;',
    'out.r1_cpu[oi]=float(r1c);out.r1_gpu[oi]=float(r1g);',
    'E_prod',
    'E_oracle',
    'E_local_R1',
    'E_state_R0',
    'E_local_R0',
    'E_total_R0',
    'H-R1-ORACLE-SEMANTIC-MISMATCH',
    'H-RATIO-SENSITIVITY',
    'H-PRODUCTION-LOCAL-RESIDUAL',
    'ATTRIBUTION-INVALID',
    'arcllm.p8g5.production_semantic_local_attribution.v1',
]:
    assert x in s, x

# Exact P8-G4 cohort and scope are reused.
assert '0.17f*std::sin' not in s
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

# P0 freezes exact P8-G4 metrics and ratio tolerances.
for v in [
    '0.03265380859375','0.00372314453125','0.022247314453125','0.01629638671875',
    '0.00152587890625','0.00140380859375','0.000732421875','0.001007080078125',
    '0.0467289719626168','0.377049180327869','0.0329218106995885','0.0617977528089888',
    '0.0539959165053624','0.175281903700474','0.0640244987353183','0.0523069533336445',
]:
    assert v in s, v
assert 'p8g5_metric_match(r1_state_max,parent_state_max[si],1e-6)' in s
assert 'p8g5_metric_match(r1_state_rms,parent_state_rms[si],1e-7)' in s
assert 'p8g5_ratio_match(r1_ratio_max,parent_ratio_max[si])' in s
assert 'p8g5_ratio_match(r1_ratio_rms,parent_ratio_rms[si])' in s
assert 'const bool p0_d2_r1_fail=!(r1_ratio_max<=0.05&&r1_ratio_rms<=0.05);' in s

# Frozen P1/P2/P3 gates.
assert 'compare_vec(E_local_R1,E_reconstructed_local,1e-5,1e-7)' in s
assert 'compare_vec(oracle.r0_gpu,Y_GPU,1e-4,1e-6)' in s
assert 'compare_vec(E_total_R0,E_reconstructed_R0,1e-5,1e-7)' in s
assert 'r0_ratio_max<=0.05&&r0_ratio_rms<=0.05' in s

# Classification priority is exact.
c0=s.index('std::string classification;')
c1=s.index('const bool diagnostic_valid',c0)
cs=s[c0:c1]
assert 'if(!structural_valid||!all_p0||!all_p1||!all_p3_closure)classification="ATTRIBUTION-INVALID";' in cs
assert 'else if(!all_p2)classification="H-PRODUCTION-LOCAL-RESIDUAL";' in cs
assert 'else if(all_p3_dominance)classification="H-R1-ORACLE-SEMANTIC-MISMATCH";' in cs
assert 'else classification="H-RATIO-SENSITIVITY";' in cs

# Production graph is unchanged and oracles are CPU-only.
assert s.count('vk.prepare_chain(prefix_ops)')==1
assert s.count('vk.prepare_chain(local_ops)')==1
assert s.count('vk.destroy_prepared(local_chain)')==1
assert s.count('vk.destroy_prepared(prefix_chain)')==1

# Governance is immutable.
for x in [
    '\\"replacement_gate_defined\\":false',
    '\\"retrospective_attribution_only\\":true',
    '\\"p8g_verdict_changed\\":false',
    '\\"p8g1_verdict_changed\\":false',
    '\\"p8g2_verdict_changed\\":false',
    '\\"p8g3_verdict_changed\\":false',
    '\\"p8g4_verdict_changed\\":false',
    '\\"p8h_permitted\\":false',
    '\\"full_inference_permitted\\":false',
]:
    assert x in s, x
assert 'p7_embedding' not in s
assert 'p7_lmhead' not in s
assert 'build_decode' not in s

c=(ROOT/'tools/compile_p8g5_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p8g5.shader_provenance.v1' in c
assert 'glslang-16.5.0' in c
assert '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert c.count('.comp"')==11

b=(ROOT/'tools/build_p8g5.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8g5_production_semantic_attribution.cpp','gguf.cpp','tensor_store.cpp'], cpp_refs
assert 'arcllm_p8g5.exe' in b

r=(ROOT/'run_p8g5.ps1').read_text(encoding='ascii')
for h in [
    '1C146FCDD60D14A782512687865AC403D6DEE5C72FC125F8A74C5D4A920D5C7C',
    '9255B70BA50DF316B7D8BA04CB6696DA9FB2CB97D82F7A559DCF166F7E76E757',
    '6347545710333A3CF9856399DF040E57A6C140E3463C1E50E5A8A432AF82FFD0',
]:
    assert h in r
assert 'p8g5_production_semantic_attribution_results.json' in r
assert 'arcllm.p8g5.summary.v1' in r
assert 'retrospective_attribution_only=$true' in r
assert 'p8g4_classification_frozen="H-LOCAL-ERROR-NONNEGLIGIBLE"' in r
assert 'replacement_gate_defined=$false' in r
assert 'p8h_permitted=$false' in r
assert 'full_inference_permitted=$false' in r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-G5'
assert m['status']=='READY_TO_RUN'
assert m['target_run_permitted'] is True
assert m['p8g_verdict_frozen']=='FAIL'
assert m['p8g1_classification_frozen']=='H-AMPLIFICATION'
assert m['p8g2_classification_frozen']=='H-NONLINEAR/UNEXPLAINED'
assert m['p8g3_classification_frozen']=='H-FP32-ACCUMULATION'
assert m['p8g4_classification_frozen']=='H-LOCAL-ERROR-NONNEGLIGIBLE'
assert m['p8h_blocked'] is True
assert m['full_inference_forbidden'] is True
assert m['p8g5']['cohort_ids']==[17,29,43,61]
assert m['p8g5']['state_dominance_ratio_max']==0.05
assert m['p8g5']['replacement_gate_definition_permitted'] is False
assert m['implementation']['r0_r1_oracles_cpu_only'] is True
assert m['implementation']['prefix_dispatches_per_seed']==58
assert m['implementation']['local_down_dispatches_per_seed']==1
assert m['implementation']['target_experiment_run_in_lock_commit'] is False

print('ArcLLM P8-G5 static contract PASS')
