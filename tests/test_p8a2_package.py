from pathlib import Path
import hashlib,json,re
ROOT=Path(__file__).resolve().parents[1]
REQ=['docs/P8A2_CONTRACT.md','docs/P8A2_IMPLEMENTATION.md','src/p8a2_segmented_memory_plan.cpp','tools/build_p8a2.ps1','run_p8a2.ps1','tools/resolve_p8_target.ps1','config/p8_target.json','manifest.json','inputs/p8a_memory_plan.authoritative.json','inputs/p8a_summary.authoritative.json']
for r in REQ: assert (ROOT/r).is_file(),r
assert hashlib.sha256((ROOT/'inputs/p8a_memory_plan.authoritative.json').read_bytes()).hexdigest().upper()=='6495545700E4104B05ED7D913ACC728C64329CB92AC894342A690946EDECE31E'
assert hashlib.sha256((ROOT/'inputs/p8a_summary.authoritative.json').read_bytes()).hexdigest().upper()=='C191DC70AEBAEEF733847AFE8B15ECECC95BE112F1041178DD307FE68D981EA9'
p=json.loads((ROOT/'inputs/p8a_memory_plan.authoritative.json').read_text(encoding='utf-8-sig'))
assert p['status']=='FAIL' and not p['gate']['p8a_pass']
assert p['gate']['size_pass'] and p['gate']['architecture_pass'] and p['gate']['context_pass']
assert p['gate']['supported_types_pass'] and p['gate']['no_overlap_pass'] and p['gate']['capacity_pass']
assert not p['gate']['arena_pack_pass'] and not p['gate']['no_oversize_tensor_pass']
assert set(p['oversize_tensors'])=={'token_embd.weight','output.weight'}
assert p['memory']['total_planned_bytes']==5347770372
assert p['memory']['usable_budget_bytes']==16374562816
assert p['memory']['headroom_bytes']==11026792444
s=(ROOT/'src/p8a2_segmented_memory_plan.cpp').read_text(encoding='utf-8')
for x in ['EXPECTED_FILE_BYTES=4683074048ull','ARENA_CAP=268435456ull','MAX_CTX=4096ull','PREFILL=512ull','arcllm.p8a2.segmented_memory_plan.v1','expected_oversize={"token_embd.weight","output.weight"}','ARENA_CAP/rb','row_start','row_count','arena_byte_base','embedding_global_row_to_segment_local_row','lm_head_global_row_to_segment_local_row','contained_once_pass','piece_bytes_total!=tensor_total']:
    assert x in s,x
assert 'VkRuntime' not in s and 'vkCreate' not in s and '#include <vulkan' not in s
assert 'TensorStore' not in s
assert 'segment_count(emb)>=2' in s and 'segment_count(outw)>=2' in s
assert 'emb->dims[1]==vocab' in s and 'outw->dims[1]==vocab' in s
assert 'bytes>ARENA_CAP' in s or 's.bytes<=ARENA_CAP' in s
assert 'total_planned<=USABLE' in s
b=(ROOT/'tools/build_p8a2.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8a2_segmented_memory_plan.cpp','gguf.cpp'],cpp_refs
r=(ROOT/'run_p8a2.ps1').read_text(encoding='ascii')
assert 'tools\\resolve_p8_target.ps1' in r and 'Get-FileHash' in r
assert 'test_p8a2_package.py' in r and 'arcllm_p8a2.exe' in r
assert 'full_inference_permitted=$false' in r
assert 'residency_allocation_permitted=($Obj.status -eq "PASS")' in r
assert 'P8-B segmented residency allocation/copy bring-up' in r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-A2' and m['status']=='READY_TO_RUN'
assert m['authoritative_p8a']['status']=='FAIL' and m['authoritative_p8a']['capacity_pass'] is True
assert set(m['authoritative_p8a']['oversize_tensors'])=={'token_embd.weight','output.weight'}
assert m['frozen']['arena_cap_bytes']==268435456 and m['frozen']['max_context']==4096 and m['frozen']['prefill_chunk']==512
assert m['gate']['performance_gate'] is False and m['gate']['full_inference_forbidden'] is True
assert m['gate']['row_aligned_segments'] is True and m['gate']['embedding_addressability_required'] is True and m['gate']['lm_head_addressability_required'] is True
print('ArcLLM P8-A2 static contract PASS')
