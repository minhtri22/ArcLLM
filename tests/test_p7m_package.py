from pathlib import Path
import json,re,hashlib
ROOT=Path(__file__).resolve().parents[1]
REQ=['run_p7m.ps1','manifest.json','docs/P7M_CONTRACT.md','docs/P7M_IMPLEMENTATION.md','src/p7m_post_gateup_fusion_profile.cpp','tools/build_p7m.ps1','tools/compile_p7m_shaders.ps1','shaders/p7l_ffn_q4k_gateup_fused.comp','inputs/p7l_ab_results.authoritative.json','inputs/p7l_summary.authoritative.json','inputs/p7l_shader_provenance.authoritative.json']
for r in REQ: assert (ROOT/r).is_file(),r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P7-M'
assert m['measurement']['prefill_dispatches']==441 and m['measurement']['decode_dispatches']==469
assert m['measurement']['performance_threshold_applied'] is False
assert m['gate']['performance_threshold_applied'] is False and not m['gate']['p7_closed_by_this_package']
l=json.loads((ROOT/'inputs/p7l_ab_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert l['status']=='PASS' and l['regression']['pass'] and l['p7l_gate']['pass']
assert l['ab_pp512']['speedup_pass'] and abs(l['ab_pp512']['wall_speedup']-1.242926134)<1e-9
assert hashlib.sha256((ROOT/'inputs/p7l_ab_results.authoritative.json').read_bytes()).hexdigest().upper()=='369B0D98F3B42F6F84D13F4F097C1694EBF92CD72A295DAF989477E0FEBE319B'
assert hashlib.sha256((ROOT/'inputs/p7l_summary.authoritative.json').read_bytes()).hexdigest().upper()=='76702E630548721E4CB91793CD4186EB82666D2BA86EA52AA14F8C08A9066816'
assert hashlib.sha256((ROOT/'inputs/p7l_shader_provenance.authoritative.json').read_bytes()).hexdigest().upper()=='37EECABA41D0CB0FAF211A3096491E685F0519FA1F59AD9B24F71590BCC0E4D9'
s=(ROOT/'src/p7m_post_gateup_fusion_profile.cpp').read_text(encoding='utf-8')
assert 'struct PCFusedGU{uint32_t n,rows,batch,row_bytes,gate_base_bytes,up_base_bytes;};' in s
pref=s[s.index('auto build_prefill=[&]()'):s.index('auto build_decode=[&](uint32_t pos)')]
dec=s[s.index('auto build_decode=[&](uint32_t pos)'):s.index('auto ppops=')]
assert pref.count('ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv')==1
assert 'ffn_gate","p7g_ffn_q4k_tiled16.spv' not in pref and 'ffn_up","p7g_ffn_q4k_tiled16.spv' not in pref
assert 'ffn_down",lr[l].dw->ggml_type==Q4_K?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv"' in pref
assert 'p7l_ffn_q4k_gateup_fused.spv' not in dec
assert 'expected_prefill=1u+layers*15u+1u+lm_dispatches' in s
assert 'expected_decode=1u+layers*16u+1u+lm_dispatches' in s
assert '<<expected_decode<<"'+chr(92)+'n";' in s
assert 'arcllm.p7m.post_tile16_reprofile.v1' not in s
assert 'arcllm.p7m.post_gateup_fusion_reprofile.v1' in s
p=s.index('performance_threshold_applied')
assert s[p:p+80].replace('\\','').startswith('performance_threshold_applied":false')
assert 'fused_gate_pass' in s and 'fused_up_pass' in s
assert 'ffn_gate_up' in s
build=(ROOT/'tools/build_p7m.ps1').read_text(encoding='ascii')
assert "p7m_post_gateup_fusion_profile.cpp" in build and "p7m_ffn_gateup_fused_ab.cpp" not in build
ct=(ROOT/'tools/compile_p7m_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p7m.shader_provenance.v1' in ct
assert 'p7l_ffn_q4k_gateup_fused.comp' in ct
assert ct.count('.comp"')==16
run=(ROOT/'run_p7m.ps1').read_text(encoding='ascii')
assert 'post-gateup-fusion re-profile' in run
assert '@($ProvObj.compiled).Count -ne 16' in run
assert '$P7L.optimized_pp512_tok_s' in run and '$P7L.pp512_wall_speedup' in run
assert '$ResultObj.p7m_gate.pass' in run
print('ArcLLM P7-M static contract PASS')
