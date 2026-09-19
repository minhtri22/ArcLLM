from pathlib import Path
import json,re,hashlib
ROOT=Path(__file__).resolve().parents[1]
REQ=['run_p7k.ps1','manifest.json','docs/P7K_CONTRACT.md','docs/P7K_IMPLEMENTATION.md','src/p7k_ffn_row16_ab.cpp','tools/build_p7k.ps1','tools/compile_p7k_shaders.ps1','shaders/p7k_ffn_q4k_row16.comp','shaders/p7k_ffn_q6k_row16.comp','inputs/p7j_ab_results.authoritative.json','inputs/p7j_summary.authoritative.json','inputs/p7j_shader_provenance.authoritative.json','P7K_CANONICAL_SHA256SUMS.txt']
for r in REQ: assert (ROOT/r).is_file(),r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P7-K' and m['ab']['baseline_ffn_row_tile']==8 and m['ab']['optimized_ffn_row_tile']==16
assert m['ab']['ffn_token_tile']==16 and m['ab']['tile_k']==32 and m['ab']['workgroup']=='8x8'
assert m['gate']['min_pp512_speedup']==1.10 and not m['gate']['p7_closed_by_this_package']
j=json.loads((ROOT/'inputs/p7j_ab_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert j['status']=='FAIL' and j['regression']['pass'] and j['p7j_gate']['correctness_pass']
assert not j['ab_pp512']['speedup_pass'] and abs(j['ab_pp512']['wall_speedup']-0.9821774944)<1e-9
assert hashlib.sha256((ROOT/'inputs/p7j_ab_results.authoritative.json').read_bytes()).hexdigest().upper()=='02373F1E4BF6BEA72CB75D5D4BE564EE973DC1767DDF015F348BC94D0568F458'
assert hashlib.sha256((ROOT/'inputs/p7j_summary.authoritative.json').read_bytes()).hexdigest().upper()=='7859906F2EB170624007A774589CB7FFD3CE6A83FCDF13130D457D3F38E952A2'
assert hashlib.sha256((ROOT/'inputs/p7j_shader_provenance.authoritative.json').read_bytes()).hexdigest().upper()=='2E6A5B034C040748A2EAB7714C60C9F5F208036ACE5F4CB51C74AE1774910943'
build=(ROOT/'tools/build_p7k.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",build)
assert cpp_refs==['p7k_ffn_row16_ab.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs
s=(ROOT/'src/p7k_ffn_row16_ab.cpp').read_text(encoding='utf-8')
base=s[s.index('auto build_prefill_baseline=[&]()'):s.index('auto build_prefill_opt=[&]()')]
opt=s[s.index('auto build_prefill_opt=[&]()'):s.index('auto build_decode=[&](uint32_t pos)')]
dec=s[s.index('auto build_decode=[&](uint32_t pos)'):s.index('auto ppbase=')]
assert base.count('p7g_ffn_q4k_tiled16.spv')==3 and base.count('p7g_ffn_q6k_tiled16.spv')==1
assert opt.count('p7k_ffn_q4k_row16.spv')==3 and opt.count('p7k_ffn_q6k_row16.spv')==1
assert opt.count('(pp+15u)/16u')==3 and opt.count('(ffn+15u)/16u')==2 and opt.count('(hidden+15u)/16u')==1
assert 'p7k_ffn_' not in dec and 'p7g_ffn_' not in dec and 'p7c_ffn_' not in dec
assert 'const uint32_t tb=9;' in s and 'arcllm.p7k.ffn_row16_ab.v1' in s
assert 'const double min_speedup=1.10' in s and 'p7k_gate' in s
for sh in ['shaders/p7k_ffn_q4k_row16.comp','shaders/p7k_ffn_q6k_row16.comp']:
 t=(ROOT/sh).read_text(encoding='utf-8')
 assert 'local_size_x = 8, local_size_y = 8' in t
 assert 'shared float sW[512]' in t and 'shared float sX[512]' in t
 assert 'row0=gl_WorkGroupID.x*16u' in t and 'row_b=row0+lr+8u' in t
 assert 'tok0=gl_WorkGroupID.y*16u' in t and 'acc_bb' in t
 assert t.count('barrier();')==2
ct=(ROOT/'tools/compile_p7k_shaders.ps1').read_text(encoding='ascii')
for n in ['p7g_ffn_q4k_tiled16.comp','p7g_ffn_q6k_tiled16.comp','p7k_ffn_q4k_row16.comp','p7k_ffn_q6k_row16.comp']: assert n in ct
assert 'arcllm.p7k.shader_provenance.v1' in ct
def canon_bytes(p):
 b=p.read_bytes(); s=b.decode('ascii' if p.suffix=='.ps1' else 'utf-8')
 s=s.replace('\r\n','\n').replace('\r','\n')
 if not s.endswith('\n'): s+='\n'
 return s.encode('utf-8')
checks={}
for line in (ROOT/'P7K_CANONICAL_SHA256SUMS.txt').read_text(encoding='ascii').splitlines():
 if line.strip(): h,p=line.split('  ',1); checks[p]=h
for p,h in checks.items(): assert hashlib.sha256(canon_bytes(ROOT/p)).hexdigest().upper()==h,p
assert set(checks)==set(["run_p7k.ps1","manifest.json","docs/P7K_CONTRACT.md","docs/P7K_IMPLEMENTATION.md","src/p7k_ffn_row16_ab.cpp","tools/build_p7k.ps1","tools/compile_p7k_shaders.ps1","shaders/p7k_ffn_q4k_row16.comp","shaders/p7k_ffn_q6k_row16.comp","tests/test_p7k_package.py"])
print('ArcLLM P7-K static contract PASS')
