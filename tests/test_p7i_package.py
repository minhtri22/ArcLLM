from pathlib import Path
import json,re,hashlib
ROOT=Path(__file__).resolve().parents[1]
REQ=[
 'run_p7i.ps1','manifest.json','lineage.md','config/p0_target.json',
 'docs/P7I_CONTRACT.md','docs/P7I_IMPLEMENTATION.md',
 'src/p7i_ffn_tile32_ab.cpp','tools/build_p7i.ps1','tools/compile_p7i_shaders.ps1',
 'inputs/p7h_profile_results.authoritative.json','inputs/p7h_summary.authoritative.json','inputs/p7h_shader_provenance.authoritative.json',
 'shaders/p7i_ffn_q4k_tiled32.comp','shaders/p7i_ffn_q6k_tiled32.comp',
 'shaders/p7g_ffn_q4k_tiled16.comp','shaders/p7g_ffn_q6k_tiled16.comp',
 'P7I_SHA256SUMS.txt'
]
for r in REQ: assert (ROOT/r).is_file(),r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P7-I' and m['ab']['baseline_ffn_token_tile']==16 and m['ab']['optimized_ffn_token_tile']==32
assert m['ab']['regression_batch']==25 and m['gate']['min_pp512_speedup']==1.10 and not m['gate']['p7_closed_by_this_package']
h=json.loads((ROOT/'inputs/p7h_profile_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert h['status']=='PASS' and h['p7h_gate']['pass'] and h['regression']['pass']
ffn=sum(x['chain_share'] for x in h['prefill_profile']['categories'] if x['name'] in {'ffn_gate_up','ffn_down'})
assert ffn>0.76 and h['prefill_profile']['barrier_or_unattributed_ticks']/h['prefill_profile']['chain_ticks']<0.001
# build path closure
build=(ROOT/'tools/build_p7i.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",build)
assert cpp_refs==['p7i_ffn_tile32_ab.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs
for cpp in cpp_refs: assert (ROOT/'src'/cpp).is_file(),cpp
# graph attribution
s=(ROOT/'src/p7i_ffn_tile32_ab.cpp').read_text(encoding='utf-8')
base=s[s.index('auto build_prefill_baseline=[&]()'):s.index('auto build_prefill_opt=[&]()')]
opt=s[s.index('auto build_prefill_opt=[&]()'):s.index('auto build_decode=[&](uint32_t pos)')]
dec=s[s.index('auto build_decode=[&](uint32_t pos)'):s.index('auto ppbase=')]
assert base.count('p7g_ffn_q4k_tiled16.spv')==3 and base.count('p7g_ffn_q6k_tiled16.spv')==1
assert base.count('(pp+15u)/16u')==3 and 'p7i_ffn_' not in base
assert opt.count('p7i_ffn_q4k_tiled32.spv')==3 and opt.count('p7i_ffn_q6k_tiled32.spv')==1
assert opt.count('(pp+31u)/32u')==3
assert 'p7i_ffn_' not in dec and 'p7g_ffn_' not in dec and 'p7c_ffn_' not in dec
assert 'const uint32_t tb=25;' in s and 'arcllm.p7i.ffn_token_tile32_ab.v1' in s
assert 'const double min_speedup=1.10' in s and 'p7i_gate' in s
# tile32 shader coverage / resource shape
for sh in ['shaders/p7i_ffn_q4k_tiled32.comp','shaders/p7i_ffn_q6k_tiled32.comp']:
 t=(ROOT/sh).read_text(encoding='utf-8')
 assert 'local_size_x = 8, local_size_y = 8' in t and 'shared float sX[1024]' in t
 assert 'tok_d=tok0+lt+24u' in t and 'if(valid_d)Y.y[' in t and 'tok0=gl_WorkGroupID.y*32u' in t
# compiler closure and PowerShell case-collision guard
ct=(ROOT/'tools/compile_p7i_shaders.ps1').read_text(encoding='ascii')
for n in ['p7g_ffn_q4k_tiled16.comp','p7g_ffn_q6k_tiled16.comp','p7i_ffn_q4k_tiled32.comp','p7i_ffn_q6k_tiled32.comp']: assert n in ct
assert 'arcllm.p7i.shader_provenance.v1' in ct
vs=re.findall(r'\$([A-Za-z_][A-Za-z0-9_]*)',ct); cm={}
for v in vs: cm.setdefault(v.lower(),set()).add(v)
assert not {k:v for k,v in cm.items() if len(v)>1}
for r in ['run_p7i.ps1','tools/build_p7i.ps1','tools/compile_p7i_shaders.ps1']:
 b=(ROOT/r).read_bytes(); b.decode('ascii'); assert b'\r\n' in b and b'\n' not in b.replace(b'\r\n',b'')
# critical-file SHA closure
checks={}
for line in (ROOT/'P7I_SHA256SUMS.txt').read_text(encoding='ascii').splitlines():
 if line.strip(): hsh,r=line.split('  ',1); checks[r]=hsh
for r,hsh in checks.items(): assert hashlib.sha256((ROOT/r).read_bytes()).hexdigest().upper()==hsh,r
assert set(checks)==set(REQ)-{'P7I_SHA256SUMS.txt'}
print('ArcLLM P7-I static contract PASS')