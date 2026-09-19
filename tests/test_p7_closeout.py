from pathlib import Path
import json, hashlib

ROOT=Path(__file__).resolve().parents[1]

REQ=[
    'docs/P7_CLOSEOUT.md',
    'docs/P8_ENTRY_CONTRACT.md',
    'inputs/p7o_ab_results.authoritative.json',
    'inputs/p7o_summary.authoritative.json',
    'inputs/p7o_shader_provenance.authoritative.json',
    'manifest.json',
    'README.md',
    'lineage.md',
]
for r in REQ:
    assert (ROOT/r).is_file(), r

assert hashlib.sha256((ROOT/'inputs/p7o_ab_results.authoritative.json').read_bytes()).hexdigest().upper() == '814D72E96F082575BF160CF8DE02102BA69DDAA79494A07E9EEDE67F39DE6624'
assert hashlib.sha256((ROOT/'inputs/p7o_summary.authoritative.json').read_bytes()).hexdigest().upper() == '9C5D3CC11EAE52CC7BF7C55BD2F479F22B0456C113E3BE07E476356700C894B6'
assert hashlib.sha256((ROOT/'inputs/p7o_shader_provenance.authoritative.json').read_bytes()).hexdigest().upper() == '59AD51C2DA5D68D01C387592635D675D674490CCC8BCE4B060DB93984B898EC2'
prov=json.loads((ROOT/'inputs/p7o_shader_provenance.authoritative.json').read_text(encoding='utf-8-sig'))
assert prov['schema']=='arcllm.p7o.shader_provenance.v1'
assert len(prov['compiled'])==18
assert all('spv_bytes' in x and isinstance(x['spv_bytes'], int) and x['spv_bytes']>0 for x in prov['compiled'])
assert next(x for x in prov['compiled'] if x['source']=='p7c_ffn_q6k_tiled.comp')['spv_bytes']==12412

o=json.loads((ROOT/'inputs/p7o_ab_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert o['status']=='FAIL'
assert o['regression']['pass'] and o['p7o_gate']['correctness_pass']
assert not o['ab_pp512']['speedup_pass']
assert abs(o['ab_pp512']['wall_speedup']-0.8836691312) < 1e-12
assert o['ab_pp512']['top1_match_all_trials']
assert o['ab_pp512']['logits_match_all_trials']

m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P7-CLOSEOUT'
assert m['status']=='CLOSED'
assert m['frozen_winner']['checkpoint']=='P7-L'
assert abs(m['frozen_winner']['pp512_same_run_speedup']-1.242926134) < 1e-12
assert m['final_negative']['checkpoint']=='P7-O'
assert abs(m['final_negative']['wall_speedup']-0.8836691312) < 1e-12
assert m['closeout']['global_p7_throughput_gate_pre_registered'] is False
assert m['closeout']['option_gates_remain_authoritative'] is True
assert m['closeout']['production_path_ready'] is True
assert m['closeout']['p7_closed'] is True
assert m['closeout']['next_phase']=='P8'

r=(ROOT/'README.md').read_text(encoding='utf-8')
assert 'P0-P7 are CLOSED. P8 (7B memory-planned runtime) is NEXT.' in r
assert 'P7  Q4_K_M production path                          CLOSED' in r
assert 'P8  7B memory-planned runtime                       NEXT' in r

c=(ROOT/'docs/P7_CLOSEOUT.md').read_text(encoding='utf-8')
assert 'P7 is CLOSED' in c
assert 'P7-L' in c
assert 'P7-O FFN-down K32 -> K64: 0.8836691312x — FAIL.' in c
assert 'no separate global P7 throughput gate' in c
assert '1.1030x theoretical end-to-end speedup' in c

p8=(ROOT/'docs/P8_ENTRY_CONTRACT.md').read_text(encoding='utf-8')
assert 'Exact 7B model file selected.' in p8
assert 'SHA256 frozen.' in p8
assert 'Memory-plan-only bring-up' in p8

print('ArcLLM P7 closeout contract PASS')
