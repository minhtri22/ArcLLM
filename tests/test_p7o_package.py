from pathlib import Path
import json,re,hashlib
ROOT=Path(__file__).resolve().parents[1]
REQ=[
 'run_p7o.ps1','manifest.json','docs/P7O_CONTRACT.md','docs/P7O_IMPLEMENTATION.md',
 'src/p7o_ffn_down_k64_ab.cpp','tools/build_p7o.ps1','tools/compile_p7o_shaders.ps1',
 'shaders/p7o_ffn_down_q4k_k64.comp','shaders/p7o_ffn_down_q6k_k64.comp',
 'shaders/p7l_ffn_q4k_gateup_fused.comp',
 'inputs/p7n_ab_results.authoritative.json','inputs/p7n_summary.authoritative.json',
 'inputs/p7n_shader_provenance.authoritative.json'
]
for r in REQ: assert (ROOT/r).is_file(),r

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P7-O'
assert m['ab']['baseline_prefill_dispatches']==441 and m['ab']['optimized_prefill_dispatches']==441
assert m['ab']['decode_dispatches']==469
assert m['ab']['baseline_down_k_tile']==32 and m['ab']['optimized_down_k_tile']==64
assert m['ab']['row_tile']==8 and m['ab']['token_tile']==16 and m['ab']['workgroup']=='8x8'
assert m['gate']['min_pp512_speedup']==1.10 and not m['gate']['p7_closed_by_this_package']

n=json.loads((ROOT/'inputs/p7n_ab_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert n['status']=='FAIL' and n['regression']['pass'] and n['p7n_gate']['correctness_pass']
assert not n['ab_pp512']['speedup_pass'] and abs(n['ab_pp512']['wall_speedup']-1.051612109)<1e-9
assert hashlib.sha256((ROOT/'inputs/p7n_ab_results.authoritative.json').read_bytes()).hexdigest().upper()=='B5D7E821D5E51F98800F537A6FAEFD11982F2536E6E7BD781F3AA3C343475429'
assert hashlib.sha256((ROOT/'inputs/p7n_summary.authoritative.json').read_bytes()).hexdigest().upper()=='536ED16FBE31CE88B0A4B4ACDCD12AA2DF28F49CBB913441C93A912083E5CA16'
assert hashlib.sha256((ROOT/'inputs/p7n_shader_provenance.authoritative.json').read_bytes()).hexdigest().upper()=='47B2FAE93CBC582577BBF6AB30F5333B915E41BB0C17408A0C89F1184BC6A5C1'

s=(ROOT/'src/p7o_ffn_down_k64_ab.cpp').read_text(encoding='utf-8')
base=s[s.index('auto build_prefill_baseline=[&]()'):s.index('auto build_prefill_opt=[&]()')]
opt=s[s.index('auto build_prefill_opt=[&]()'):s.index('auto build_decode=[&](uint32_t pos)')]
dec=s[s.index('auto build_decode=[&](uint32_t pos)'):s.index('auto ppbase=')]

# Exact P7-L baseline and exact same graph except down shaders in optimized arm.
for seg in (base,opt):
    assert seg.count('ffn_gate_up_fused","p7l_ffn_q4k_gateup_fused.spv')==1
    assert seg.count('swiglu","p7_swiglu.spv')==1
assert 'p7o_ffn_down_' not in base
assert 'p7g_ffn_q4k_tiled16.spv' in base and 'p7g_ffn_q6k_tiled16.spv' in base
assert 'p7o_ffn_down_q4k_k64.spv' in opt and 'p7o_ffn_down_q6k_k64.spv' in opt
assert 'p7g_ffn_q4k_tiled16.spv":"p7g_ffn_q6k_tiled16.spv' not in opt
assert 'p7o_ffn_' not in dec and 'p7l_ffn_q4k_gateup_fused.spv' not in dec

assert 'expected_prefill=1u+layers*15u+1u+lm_dispatches' in s
assert 'expected_decode=1u+layers*16u+1u+lm_dispatches' in s
assert 'arcllm.p7o.ffn_down_k64_ab.v1' in s
assert 'const double min_speedup=1.10' in s
assert 'down_q4_k64_pass' in s and 'down_q6_k64_pass' in s
assert 'ArcLLM P7-O FFN-down K64 A/B' in s
assert 'md4.max_abs' in s and 'md6.max_abs' in s
assert 'q4_layer=layers,q6_layer=layers' in s
assert 'matmul_q4_cpu' in s and 'matmul_q6_cpu' in s
assert 'p7o_gate' in s
assert 'p7n_ffn_' not in s

for sh in ['shaders/p7o_ffn_down_q4k_k64.comp','shaders/p7o_ffn_down_q6k_k64.comp']:
    t=(ROOT/sh).read_text(encoding='utf-8')
    for x in ['local_size_x = 8, local_size_y = 8',
              'shared float sW[512]','shared float sX[1024]',
              'row0=gl_WorkGroupID.x*8u','tok0=gl_WorkGroupID.y*16u',
              'k0+=64u','wb=lr*64u','kk<64u']:
        assert x in t,(sh,x)
    assert t.count('barrier();')==2
    assert t.count('kk<64u')==2
    assert 'kk<32u' not in t
    assert 'k0+=32u' not in t

build=(ROOT/'tools/build_p7o.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",build)
assert cpp_refs==['p7o_ffn_down_k64_ab.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs

ct=(ROOT/'tools/compile_p7o_shaders.ps1').read_text(encoding='ascii')
assert 'arcllm.p7o.shader_provenance.v1' in ct
assert 'p7l_ffn_q4k_gateup_fused.comp' in ct
assert 'p7o_ffn_down_q4k_k64.comp' in ct and 'p7o_ffn_down_q6k_k64.comp' in ct
assert 'p7n_ffn_q4k_gateup_swiglu_fused.comp' not in ct
assert ct.count('.comp"')==18
assert 'ToUpperInvariant(,' not in ct

run=(ROOT/'run_p7o.ps1').read_text(encoding='ascii')
assert 'FFN-down K64 A/B' in run
assert '@($ProvObj.compiled).Count -ne 18' in run
assert 'down_q4_k64_pass=$ResultObj.regression.down_q4_k64_pass' in run
assert 'down_q6_k64_pass=$ResultObj.regression.down_q6_k64_pass' in run
assert 'baseline_p7l_pp512_tok_s' in run
assert 'if($ResultObj.status -eq "ERROR")' in run
assert 'result JSON missing scope outside ERROR status' in run
assert '$ResultObj.p7o_gate.pass' in run

print('ArcLLM P7-O static contract PASS')
