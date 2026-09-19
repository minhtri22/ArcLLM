from pathlib import Path
import hashlib,json,re
ROOT=Path(__file__).resolve().parents[1]
REQ=['docs/P8B_CONTRACT.md','docs/P8B_IMPLEMENTATION.md','src/p8b_segmented_residency.cpp','tools/build_p8b.ps1','run_p8b.ps1','config/p8_target.json','manifest.json','inputs/p8a2_segment_plan.authoritative.json','inputs/p8a2_summary.authoritative.json']
for r in REQ: assert (ROOT/r).is_file(),r
assert hashlib.sha256((ROOT/'inputs/p8a2_segment_plan.authoritative.json').read_bytes()).hexdigest().upper()=='7390D579937EDD8748A08D7D72E7DFEC53D7470FA9C86208C2412A33E231D481'
assert hashlib.sha256((ROOT/'inputs/p8a2_summary.authoritative.json').read_bytes()).hexdigest().upper()=='2534F8AC68134562F8D0AD1F5CB6464440C368E48659FC0DE1EB051C02B74876'
p=json.loads((ROOT/'inputs/p8a2_segment_plan.authoritative.json').read_text(encoding='utf-8-sig'))
assert p['status']=='PASS' and p['gate']['p8a2_pass']
assert p['arena_plan']['count']==19 and p['arena_plan']['piece_count']==341
assert p['arena_plan']['piece_bytes_total']==4677120000
assert p['addressability']['pass'] is True
assert p['memory']['total_planned_bytes']==5347770372 and p['memory']['capacity_pass'] is True
seg={x['name']:x for x in p['segmented_tensors']}
assert seg['token_embd.weight']['type_name']=='Q4_K' and seg['token_embd.weight']['row_bytes']==2016 and seg['token_embd.weight']['segment_count']==2
assert [x['row_count'] for x in seg['token_embd.weight']['segments']]==[133152,18912]
assert seg['output.weight']['type_name']=='Q6_K' and seg['output.weight']['row_bytes']==2940 and seg['output.weight']['segment_count']==2
assert [x['row_count'] for x in seg['output.weight']['segments']]==[91304,60760]
s=(ROOT/'src/p8b_segmented_residency.cpp').read_text(encoding='utf-8')
for x in ['EXPECTED_FILE_BYTES=4683074048ull','ARENA_CAP=268435456ull','EXPECTED_WEIGHT_BYTES=4677120000ull','EXPECTED_KV_BYTES=469762048ull','EXPECTED_WORK_BYTES=200888324ull','EXPECTED_TOTAL_BYTES=5347770372ull','USABLE=16374562816ull','std::memcmp','compared==EXPECTED_WEIGHT_BYTES','exhaustive_row_translation','memory_type_flags','vkAllocateMemory failed','work.size()==20','kv.size()==2','weights.size()==19','actual_vk_allocation']:
    assert x in s,x
assert 'reqd<PFN_vkCreateShaderModule>' not in s
assert 'read_spv(' not in s and '.spv' not in s
assert 'prepare_chain(' not in s and 'execute_prepared(' not in s
assert 'reqd<PFN_vkCmdDispatch>' not in s
assert '133152,2016,0,268434432,0,0' in s
assert '91304,2940,4230051840,4498485600,17,0' in s
assert s.count('weights.push_back(vk.make_buffer')==1
b=(ROOT/'tools/build_p8b.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8b_segmented_residency.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs
r=(ROOT/'run_p8b.ps1').read_text(encoding='ascii')
assert 'resolve_p8_target.ps1' in r and 'Get-FileHash' in r
assert 'test_p8b_package.py' in r and 'arcllm_p8b.exe' in r
assert 'full_inference_permitted=$false' in r
assert 'segmented_access_correctness_permitted=($Obj.status -eq "PASS")' in r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-B' and m['status']=='READY_TO_RUN'
assert m['authoritative_p8a2']['plan_sha256']=='7390D579937EDD8748A08D7D72E7DFEC53D7470FA9C86208C2412A33E231D481'
assert m['authoritative_p8a2']['summary_sha256']=='2534F8AC68134562F8D0AD1F5CB6464440C368E48659FC0DE1EB051C02B74876'
assert m['frozen']['total_requested_bytes']==5347770372
assert m['gate']['performance_gate'] is False and m['gate']['no_shader'] is True and m['gate']['no_dispatch'] is True and m['gate']['full_inference_forbidden'] is True
print('ArcLLM P8-B static contract PASS')
