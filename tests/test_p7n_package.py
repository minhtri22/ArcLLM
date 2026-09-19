from pathlib import Path
import json,re,hashlib
ROOT=Path(__file__).resolve().parents[1]
REQ=[
 'run_p7n.ps1','manifest.json','docs/P7N_CONTRACT.md','docs/P7N_IMPLEMENTATION.md',
 'src/p7n_ffn_gateup_swiglu_ab.cpp','src/p7l_ffn_gateup_fused_ab.cpp',
 'tools/build_p7n.ps1','tools/compile_p7n_shaders.ps1',
 'shaders/p7l_ffn_q4k_gateup_fused.comp','shaders/p7n_ffn_q4k_gateup_swiglu_fused.comp',
 'inputs/p7m_profile_results.authoritative.json','inputs/p7m_summary.authoritative.json',
 'inputs/p7m_shader_provenance.authoritative.json'
]
for r in REQ: assert (ROOT/r).is_file(),r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P7-N'
assert m['ab']['baseline_prefill_dispatches']==441
assert m['ab']['optimized_prefill_dispatches']==413
assert m['ab']['decode_dispatches']==469
assert m['ab']['ffn_row_tile']==8 and m['ab']['ffn_token_tile']==16 and m['ab']['tile_k']==32 and m['ab']['workgroup']=='8x8'
assert m['gate']['min_pp512_speedup']==1.10 and not m['gate']['p7_closed_by_this_package']

p=json.loads((ROOT/'inputs/p7m_profile_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert p['status']=='PASS' and p['p7m_gate']['pass'] and not p['p7m_gate']['performance_threshold_applied']
assert p['runtime']['prefill_dispatches']==441 and p['runtime']['decode_dispatches']==469
cats={x['name']:x for x in p['prefill_profile']['categories']}
assert abs(cats['ffn_gate_up']['chain_share']-0.4440335651)<1e-12
assert abs(cats['ffn_down']['chain_share']-0.3042145996)<1e-12
assert p['prefill_profile']['barrier_or_unattributed_ticks']==19355
assert hashlib.sha256((ROOT/'inputs/p7m_profile_results.authoritative.json').read_bytes()).hexdigest().upper()=='B06095481B842A5A6BDF571EE0B368471F9AE5AF34C781BB89E65A7015645358'
assert hashlib.sha256((ROOT/'inputs/p7m_summary.authoritative.json').read_bytes()).hexdigest().upper()=='020C8A07EA73282ED89EEBCD1BC64F873569B376F5783FFFC1A9A52DAFCE2282'
assert hashlib.sha256((ROOT/'inputs/p7m_shader_provenance.authoritative.json').read_bytes()).hexdigest().upper()=='C0F980CBE32E7A42CAD2B177A02F535BB772F6B5B62A84D438BAC7CB463543A5'

s=(ROOT/'src/p7n_ffn_gateup_swiglu_ab.cpp').read_text(encoding='utf-8')
base=s[s.index('auto build_prefill_baseline=[&]()'):s.index('auto build_prefill_opt=[&]()')]
opt=s[s.index('auto build_prefill_opt=[&]()'):s.index('auto build_decode=[&](uint32_t pos)')]
dec=s[s.index('auto build_decode=[&](uint32_t pos)'):s.index('auto ppbase=')]

assert base.count('ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv')==1
assert base.count('swiglu","p7_swiglu.spv')==1
assert 'p7n_ffn_q4k_gateup_swiglu_fused.spv' not in base
assert opt.count('ffn_gate_up_swiglu_fused","p7n_ffn_q4k_gateup_swiglu_fused.spv')==1
assert 'swiglu","p7_swiglu.spv' not in opt
assert 'p7l_ffn_q4k_gateup_fused.spv' not in opt
for seg in (base,opt):
    assert 'ffn_down",lr[l].dw->ggml_type==Q4_K?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv"' in seg
assert 'p7n_ffn_' not in dec and 'p7l_ffn_q4k_gateup_fused.spv' not in dec

assert 'expected_base=1u+layers*15u+1u+lm_dispatches' in s
assert 'expected_opt=1u+layers*14u+1u+lm_dispatches' in s
assert 'expected_decode=1u+layers*16u+1u+lm_dispatches' in s
assert 'arcllm.p7n.ffn_gateup_swiglu_fused_ab.v1' in s
assert 'const double min_speedup=1.10' in s
assert 'ref_s=swiglu_cpu(ref_gate,ref_up)' in s
assert 'fused_swiglu_pass' in s and 'mfs.max_abs' in s and 'mfs.rmse' in s
assert 'historical_p7g_optimized_tok_s' not in s
assert 'historical_p7l_optimized_tok_s' in s and 'historical_p7g_decode_tok_s' in s
assert 'p7n_gate' in s

sh=(ROOT/'shaders/p7n_ffn_q4k_gateup_swiglu_fused.comp').read_text(encoding='utf-8')
for x in ['local_size_x = 8, local_size_y = 8','binding = 0','binding = 1','binding = 2','binding = 3',
          'shared float sWG[256]','shared float sWU[256]','shared float sX[512]',
          'row0=gl_WorkGroupID.x*8u','tok0=gl_WorkGroupID.y*16u','S.y[','exp(-ga)','exp(-gb)']:
    assert x in sh,x
assert 'binding = 4' not in sh
assert sh.count('barrier();')==2

build=(ROOT/'tools/build_p7n.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",build)
assert cpp_refs==['p7n_ffn_gateup_swiglu_ab.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs

ct=(ROOT/'tools/compile_p7n_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p7n.shader_provenance.v1' in ct
assert 'p7l_ffn_q4k_gateup_fused.comp' in ct
assert 'p7n_ffn_q4k_gateup_swiglu_fused.comp' in ct
assert ct.count('.comp"')==17
assert 'ToUpperInvariant(,' not in ct

run=(ROOT/'run_p7n.ps1').read_text(encoding='ascii')
assert 'FFN gate+up+SwiGLU fusion A/B' in run
assert '@($ProvObj.compiled).Count -ne 17' in run
assert 'baseline_p7l_pp512_tok_s' in run and 'baseline_p7g_pp512_tok_s' not in run
assert 'fused_swiglu_pass=$ResultObj.regression.fused_swiglu_pass' in run
assert '$ResultObj.p7n_gate.pass' in run

print('ArcLLM P7-N static contract PASS')
