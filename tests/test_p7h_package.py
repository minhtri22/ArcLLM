from pathlib import Path
import json,hashlib,re
ROOT=Path(__file__).resolve().parents[1]
req=['run_p7h.ps1','manifest.json','lineage.md','README.md','config/p0_target.json','docs/P7H_CONTRACT.md','docs/P7H_IMPLEMENTATION.md','src/p7h_post_tile16_profile.cpp','tools/build_p7h.ps1','tools/compile_p7h_shaders.ps1','inputs/p7g_ab_results.authoritative.json','inputs/p7g_summary.authoritative.json','inputs/p7g_shader_provenance.authoritative.json','shaders/p7c_ffn_q4k_tiled.comp','shaders/p7c_ffn_q6k_tiled.comp','shaders/p7g_ffn_q4k_tiled16.comp','shaders/p7g_ffn_q6k_tiled16.comp','PACKAGE_SHA256SUMS.txt']
for r in req: assert (ROOT/r).is_file(),r
# Build/package closure: every local C++ source referenced by build_p7h.ps1 must exist.
build=(ROOT/'tools/build_p7h.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$Src '([^']+\.cpp)'",build)
assert cpp_refs, 'no C++ sources discovered in build_p7h.ps1'
for cpp in cpp_refs:
    assert (ROOT/'src'/cpp).is_file(), f'build script references missing source: src/{cpp}'
assert 'p7h_post_tile16_profile.cpp' in cpp_refs
assert 'p7h_post_attnproj_profile.cpp' not in build
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'));assert m['phase']=='P7-H' and m['gate']['measurement_only'] and not m['gate']['p7_closed_by_this_package']
g=json.loads((ROOT/'inputs/p7g_ab_results.authoritative.json').read_text(encoding='utf-8-sig'));assert g['status']=='PASS' and g['p7g_gate']['pass'] and g['ab_pp512']['wall_speedup']>=1.22
gs=json.loads((ROOT/'inputs/p7g_summary.authoritative.json').read_text(encoding='utf-8-sig'));assert gs['optimized_pp512_tok_s']>96.2 and gs['p7g_gate_pass']
prov=json.loads((ROOT/'inputs/p7g_shader_provenance.authoritative.json').read_text(encoding='utf-8-sig'));assert prov['compiler_release']=='16.5.0' and len(prov['compiled'])==15
s=(ROOT/'src/p7h_post_tile16_profile.cpp').read_text(encoding='utf-8');a=s[s.index('  auto build_prefill=[&]()'):s.index('  auto build_decode=[&]')];d=s[s.index('  auto build_decode=[&]'):s.index('  auto ppops=build_prefill()')]
assert 'q_proj","p7c_ffn_q4k_tiled.spv' in a and 'k_proj","p7c_ffn_q4k_tiled.spv' in a and 'o_proj","p7c_ffn_q4k_tiled.spv' in a
assert 'v_proj",lr[l].vw->ggml_type==Q4_K?"p7c_ffn_q4k_tiled.spv":"p7c_ffn_q6k_tiled.spv"' in a
assert 'ffn_gate","p7g_ffn_q4k_tiled16.spv' in a and 'ffn_up","p7g_ffn_q4k_tiled16.spv' in a
assert 'ffn_down",lr[l].dw->ggml_type==Q4_K?"p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv"' in a
assert a.count('(pp+15u)/16u')==3
assert 'p7g_ffn_' not in d and 'p7c_ffn_' not in d
assert 'execute_profiled' in s and 'arcllm.p7h.post_tile16_reprofile.v1' in s and 'p7h_gate' in s
for sh in ['shaders/p7g_ffn_q4k_tiled16.comp','shaders/p7g_ffn_q6k_tiled16.comp']:
 t=(ROOT/sh).read_text(encoding='utf-8');assert 'local_size_x = 8, local_size_y = 8' in t
for r in ['run_p7h.ps1','tools/build_p7h.ps1','tools/compile_p7h_shaders.ps1']:
 b=(ROOT/r).read_bytes();b.decode('ascii');assert b'\r\n' in b and b'\n' not in b.replace(b'\r\n',b'')
ct=(ROOT/'tools/compile_p7h_shaders.ps1').read_text(encoding='ascii');assert 'arcllm.p7h.shader_provenance.v1' in ct and 'p7g_ffn_q4k_tiled16.comp' in ct and 'p7g_ffn_q6k_tiled16.comp' in ct
vs=re.findall(r'\$([A-Za-z_][A-Za-z0-9_]*)',ct);cm={}
for v in vs: cm.setdefault(v.lower(),set()).add(v)
assert not {k:v for k,v in cm.items() if len(v)>1}
checks={}
for line in (ROOT/'PACKAGE_SHA256SUMS.txt').read_text(encoding='ascii').splitlines():
 if line.strip(): h,r=line.split('  ',1);checks[r]=h
def tracked_source_file(p):
 rel=p.relative_to(ROOT)
 parts=rel.parts
 if not p.is_file() or p.name=='PACKAGE_SHA256SUMS.txt': return False
 if '.git' in parts or (parts and parts[0]=='results'): return False
 if p.suffix.lower() in {'.exe','.obj','.pdb','.ilk','.spv','.pyc'}: return False
 if '__pycache__' in parts: return False
 if len(parts)>=2 and parts[0]=='tools' and parts[1].startswith('glslang-'): return False
 return True
actual={p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*') if tracked_source_file(p)};assert set(checks)==actual,(set(checks)^actual)
for r,h in checks.items():assert hashlib.sha256((ROOT/r).read_bytes()).hexdigest().upper()==h,r
print('ArcLLM P7-H package/static contract PASS')
