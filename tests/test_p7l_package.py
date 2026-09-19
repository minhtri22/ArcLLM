from pathlib import Path
import json,re,hashlib
ROOT=Path(__file__).resolve().parents[1]
REQ=['run_p7l.ps1','manifest.json','docs/P7L_CONTRACT.md','docs/P7L_IMPLEMENTATION.md','src/p7l_ffn_gateup_fused_ab.cpp','tools/build_p7l.ps1','tools/compile_p7l_shaders.ps1','shaders/p7l_ffn_q4k_gateup_fused.comp','inputs/p7k_ab_results.authoritative.json','inputs/p7k_summary.authoritative.json','inputs/p7k_shader_provenance.authoritative.json']
for r in REQ: assert (ROOT/r).is_file(),r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P7-L'
assert m['ab']['baseline_gate_up_dispatches']==56 and m['ab']['optimized_fused_dispatches']==28
assert m['ab']['ffn_row_tile']==8 and m['ab']['ffn_token_tile']==16 and m['ab']['tile_k']==32 and m['ab']['workgroup']=='8x8'
assert m['gate']['min_pp512_speedup']==1.10 and not m['gate']['p7_closed_by_this_package']
k=json.loads((ROOT/'inputs/p7k_ab_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert k['status']=='FAIL' and k['regression']['pass'] and k['p7k_gate']['correctness_pass']
assert not k['ab_pp512']['speedup_pass'] and abs(k['ab_pp512']['wall_speedup']-1.068446103)<1e-9
assert hashlib.sha256((ROOT/'inputs/p7k_ab_results.authoritative.json').read_bytes()).hexdigest().upper()=='69C31507F1F5675966C5EA12D478571B21686AC5DCD52F7A15583CA67ECCD922'
assert hashlib.sha256((ROOT/'inputs/p7k_summary.authoritative.json').read_bytes()).hexdigest().upper()=='3917F53E594EF2E14C306AD26BCFE768C0B364104008F0E75AE40B28D9739E4D'
assert hashlib.sha256((ROOT/'inputs/p7k_shader_provenance.authoritative.json').read_bytes()).hexdigest().upper()=='C26C49DFCC4E9B465086D0D88A6C4622AB5965A06D4B9BFE43355489A4BA492E'
s=(ROOT/'src/p7l_ffn_gateup_fused_ab.cpp').read_text(encoding='utf-8')
assert 'struct PCFusedGU{uint32_t n,rows,batch,row_bytes,gate_base_bytes,up_base_bytes;};' in s
base=s[s.index('auto build_prefill_baseline=[&]()'):s.index('auto build_prefill_opt=[&]()')]
opt=s[s.index('auto build_prefill_opt=[&]()'):s.index('auto build_decode=[&](uint32_t pos)')]
dec=s[s.index('auto build_decode=[&](uint32_t pos)'):s.index('auto ppbase=')]
assert base.count('ffn_gate","p7g_ffn_q4k_tiled16.spv')==1 and base.count('ffn_up","p7g_ffn_q4k_tiled16.spv')==1
assert opt.count('ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv')==1
assert 'ffn_gate","' not in opt and 'ffn_up","' not in opt
assert opt.count('ffn_down",lr[l].dw->ggml_type==Q4_K?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv"')==1
assert 'p7l_ffn_' not in dec
assert 'expected_opt=expected_base-layers' in s and 'ppbase.size()!=expected_base||ppopt.size()!=expected_opt||dtemplate.size()!=expected_base' in s
assert 'const double min_speedup=1.10' in s and 'arcllm.p7l.ffn_gateup_fused_ab.v1' in s
assert 'fused_gate_pass' in s and 'fused_up_pass' in s and 'p7l_gate' in s
sh=(ROOT/'shaders/p7l_ffn_q4k_gateup_fused.comp').read_text(encoding='utf-8')
for x in ['local_size_x = 8, local_size_y = 8','binding = 0','binding = 1','binding = 2','binding = 3','binding = 4','shared float sWG[256]','shared float sWU[256]','shared float sX[512]','tok0=gl_WorkGroupID.y*16u']:
 assert x in sh,x
assert sh.count('barrier();')==2
ct=(ROOT/'tools/compile_p7l_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p7l.shader_provenance.v1' in ct and 'p7l_ffn_q4k_gateup_fused.comp' in ct and 'p7l_ffn_q6k_gateup_fused.comp' not in ct
assert '$Names=@(' in ct and ct.count('.comp"')==16
build=(ROOT/'tools/build_p7l.ps1').read_text(encoding='ascii')
assert "p7l_ffn_gateup_fused_ab.cpp" in build and "p7k_ffn_row16_ab.cpp" not in build
run=(ROOT/'run_p7l.ps1').read_text(encoding='ascii')
assert 'FFN gate+up fusion A/B' in run and '@($ProvObj.compiled).Count -ne 16' in run
assert 'fused_gate_pass=$ResultObj.regression.fused_gate_pass' in run and 'fused_up_pass=$ResultObj.regression.fused_up_pass' in run
print('ArcLLM P7-L static contract PASS')
