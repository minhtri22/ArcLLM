from pathlib import Path
import json,re,hashlib
ROOT=Path(__file__).resolve().parents[1]
REQ=[
 'run_p7j.ps1','manifest.json','lineage.md','config/p0_target.json',
 'docs/P7J_CONTRACT.md','docs/P7J_IMPLEMENTATION.md',
 'src/p7j_ffn_vec4_ab.cpp','tools/build_p7j.ps1','tools/compile_p7j_shaders.ps1',
 'inputs/p7i_ab_results.authoritative.json','inputs/p7i_summary.authoritative.json','inputs/p7i_shader_provenance.authoritative.json',
 'shaders/p7j_ffn_q4k_vec4.comp','shaders/p7j_ffn_q6k_vec4.comp',
 'shaders/p7g_ffn_q4k_tiled16.comp','shaders/p7g_ffn_q6k_tiled16.comp',
 'P7J_SHA256SUMS.txt'
]
for r in REQ: assert (ROOT/r).is_file(),r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P7-J' and m['ab']['baseline_ffn_token_tile']==16 and m['ab']['optimized_ffn_token_tile']==16
assert m['ab']['optimized_change']=='block_aware_vec4_dequant_only' and m['ab']['regression_batch']==9
assert m['gate']['min_pp512_speedup']==1.10 and not m['gate']['p7_closed_by_this_package']
i=json.loads((ROOT/'inputs/p7i_ab_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert i['status']=='FAIL' and i['regression']['pass'] and i['p7i_gate']['correctness_pass']
assert not i['ab_pp512']['speedup_pass'] and abs(i['ab_pp512']['wall_speedup']-1.003404444)<1e-9
# build closure
build=(ROOT/'tools/build_p7j.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",build)
assert cpp_refs==['p7j_ffn_vec4_ab.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs
for cpp in cpp_refs: assert (ROOT/'src'/cpp).is_file(),cpp
# graph attribution
s=(ROOT/'src/p7j_ffn_vec4_ab.cpp').read_text(encoding='utf-8')
base=s[s.index('auto build_prefill_baseline=[&]()'):s.index('auto build_prefill_opt=[&]()')]
opt=s[s.index('auto build_prefill_opt=[&]()'):s.index('auto build_decode=[&](uint32_t pos)')]
dec=s[s.index('auto build_decode=[&](uint32_t pos)'):s.index('auto ppbase=')]
assert base.count('p7g_ffn_q4k_tiled16.spv')==3 and base.count('p7g_ffn_q6k_tiled16.spv')==1
assert base.count('(pp+15u)/16u')==3 and 'p7j_ffn_' not in base
assert opt.count('p7j_ffn_q4k_vec4.spv')==3 and opt.count('p7j_ffn_q6k_vec4.spv')==1
assert opt.count('(pp+15u)/16u')==3 and '(pp+31u)/32u' not in opt
assert 'p7j_ffn_' not in dec and 'p7g_ffn_' not in dec and 'p7c_ffn_' not in dec
assert 'const uint32_t tb=9;' in s and 'arcllm.p7j.ffn_vec4_dequant_ab.v1' in s
assert 'const double min_speedup=1.10' in s and 'p7j_gate' in s and 'p7i_gate' not in s
assert 'prefill_ffn_vec4_dequant_only' in s and 'optimized_tile_tokens\\":16' in s
# optimized shader mechanics and frozen tile geometry
for sh in ['shaders/p7j_ffn_q4k_vec4.comp','shaders/p7j_ffn_q6k_vec4.comp']:
 t=(ROOT/sh).read_text(encoding='utf-8')
 assert 'local_size_x = 8, local_size_y = 8' in t and 'shared float sW[256]' in t and 'shared float sX[512]' in t
 assert 'tok0=gl_WorkGroupID.y*16u' in t and 'kk0=lt*4u' in t and 'load_u32_unaligned' in t
 assert t.count('barrier();')==2
q4=(ROOT/'shaders/p7j_ffn_q4k_vec4.comp').read_text(encoding='utf-8')
assert 'float d=fp16_at(base), dmin=fp16_at(base+2u)' in q4 and 'scale_min(base,kin0>>5u,sc,mn)' in q4
assert 'uint packed=load_u32_unaligned' in q4
q6=(ROOT/'shaders/p7j_ffn_q6k_vec4.comp').read_text(encoding='utf-8')
assert 'uint qlw=load_u32_unaligned(ql_off), qhw=load_u32_unaligned(qh_off)' in q6
assert 'quarter=(kin0&127u)>>5u' in q6 and 'int sc=load_i8(sc_base+sc_idx)' in q6
# compiler/provenance closure
ct=(ROOT/'tools/compile_p7j_shaders.ps1').read_text(encoding='ascii')
for n in ['p7g_ffn_q4k_tiled16.comp','p7g_ffn_q6k_tiled16.comp','p7j_ffn_q4k_vec4.comp','p7j_ffn_q6k_vec4.comp']: assert n in ct
assert 'p7i_ffn_q4k_tiled32.comp' not in ct and 'arcllm.p7j.shader_provenance.v1' in ct
vs=re.findall(r'\$([A-Za-z_][A-Za-z0-9_]*)',ct);cm={}
for v in vs: cm.setdefault(v.lower(),set()).add(v)
assert not {k:v for k,v in cm.items() if len(v)>1}
for r in ['run_p7j.ps1','tools/build_p7j.ps1','tools/compile_p7j_shaders.ps1']:
 b=(ROOT/r).read_bytes();b.decode('ascii');assert b'\r\n' in b and b'\n' not in b.replace(b'\r\n',b'')
# critical SHA closure
checks={}
for line in (ROOT/'P7J_SHA256SUMS.txt').read_text(encoding='ascii').splitlines():
 if line.strip(): hsh,r=line.split('  ',1);checks[r]=hsh
for r,hsh in checks.items(): assert hashlib.sha256((ROOT/r).read_bytes()).hexdigest().upper()==hsh,r
assert set(checks)==set(REQ)-{'P7J_SHA256SUMS.txt'}
print('ArcLLM P7-J static contract PASS')
