from pathlib import Path
import hashlib,json,re
ROOT=Path(__file__).resolve().parents[1]
REQ=['docs/P8C_CONTRACT.md','docs/P8C_IMPLEMENTATION.md','src/p8c_segmented_access_correctness.cpp','shaders/p8c_embedding_q4k_segmented_probe.comp','shaders/p8c_lmhead_q6k_segmented_probe.comp','tools/compile_p8c_shaders.ps1','tools/build_p8c.ps1','run_p8c.ps1','manifest.json','inputs/p8b_residency_results.authoritative.json','inputs/p8b_summary.authoritative.json']
for r in REQ: assert (ROOT/r).is_file(),r
assert hashlib.sha256((ROOT/'inputs/p8b_residency_results.authoritative.json').read_bytes()).hexdigest().upper()=='127E25B9CA8F8B4C74CDEBAFB6559CAFF078EFFE7F1513BD1A7429FD0E69B3B2'
assert hashlib.sha256((ROOT/'inputs/p8b_summary.authoritative.json').read_bytes()).hexdigest().upper()=='078FE23DF5B31B3D69162BADC6285D608F0EB1178430E38A56DD0FF827EC2B5C'
assert len((ROOT/'inputs/p8b_residency_results.authoritative.json').read_bytes())==1346
assert len((ROOT/'inputs/p8b_summary.authoritative.json').read_bytes())==430
p=json.loads((ROOT/'inputs/p8b_residency_results.authoritative.json').read_text(encoding='utf-8-sig'))
assert p['status']=='PASS' and p['gate']['p8b_pass']
assert p['parent_plan']['arena_count']==19 and p['parent_plan']['exact_match'] and p['parent_plan']['row_translation_exhaustive_pass']
assert p['weights']['full_byte_compare_bytes']==4677120000 and p['weights']['full_byte_compare_pass']
assert p['kv']['requested_bytes']==469762048 and p['kv']['allocation_pass']
assert p['working']['requested_bytes']==200888324 and p['working']['allocation_pass']
assert p['residency']['requested_bytes']==5347770372 and p['residency']['vulkan_allocation_bytes']==5347770496
s=(ROOT/'src/p8c_segmented_access_correctness.cpp').read_text(encoding='utf-8')
for x in ['hidden=3584,vocab=152064','emb_boundary=133152,out_boundary=91304','emb_rb=2016,out_rb=2940','emb0_bytes=268434432ull,emb1_bytes=38126592ull','out0_start=4230051840ull,out0_bytes=268433760ull,out1_start=4498485600ull,out1_bytes=178634400ull','emb_ids={0u,1u,133150u,133151u,133152u,133153u,152062u,152063u}','lm_rows={0u,1u,91302u,91303u,91304u,91305u,152062u,152063u}','compare_vec(emb_ref,emb_got,0.02,0.005)','compare_vec(lm_ref,lm_got,0.02,0.005)','stats.dispatch_count==2u','arcllm.p8c.segmented_access_correctness.v1']:
    assert x in s,x
assert s.count('p8c_embedding_q4k_segmented_probe.spv')==1
assert s.count('p8c_lmhead_q6k_segmented_probe.spv')==1
for x in ['mapping_equivalent=[]','emb_mapping_equivalence_pass','lm_mapping_equivalence_pass','P8-C pre-dispatch mapping equivalence failed','mapping_equivalence_pass&&em.pass','mapping_equivalence','mapping_equivalence_pass']:
    assert x in s,x
assert 'build_prefill' not in s and 'build_decode' not in s
e=(ROOT/'shaders/p8c_embedding_q4k_segmented_probe.comp').read_text(encoding='utf-8')
assert e.count('binding=')==4 and 'row<pc.boundary' in e and 'row-pc.boundary' in e and '*144u' in e
assert 'pc.n*pc.count' in e and 'local_size_x = 256' in e
l=(ROOT/'shaders/p8c_lmhead_q6k_segmented_probe.comp').read_text(encoding='utf-8')
assert l.count('binding=')==5 and 'row<pc.boundary' in l and 'row-pc.boundary' in l and '*210u' in l
assert 'local_size_x = 64' in l and 'Y.y[i]=sum' in l
c=(ROOT/'tools/compile_p8c_shaders.ps1').read_text(encoding='ascii')
assert 'glslang-16.5.0' in c and '06B71298B750268C127F2EE7AE0EF7525E2068120C6C8A3A08B2F58CA6F325CE' in c
assert c.count('.comp"')==2 and 'arcllm.p8c.shader_provenance.v1' in c
b=(ROOT/'tools/build_p8c.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8c_segmented_access_correctness.cpp','gguf.cpp','tensor_store.cpp'],cpp_refs
r=(ROOT/'run_p8c.ps1').read_text(encoding='ascii')
assert 'compile_p8c_shaders.ps1' in r and 'test_p8c_package.py' in r and 'arcllm_p8c.exe' in r
assert 'full_inference_permitted=$false' in r and 'graph_integration_permitted=($Obj.status -eq "PASS")' in r
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-C' and m['status']=='READY_TO_RUN'
assert m['authoritative_p8b']['results_sha256']=='127E25B9CA8F8B4C74CDEBAFB6559CAFF078EFFE7F1513BD1A7429FD0E69B3B2'
assert m['authoritative_p8b']['summary_sha256']=='078FE23DF5B31B3D69162BADC6285D608F0EB1178430E38A56DD0FF827EC2B5C'
assert m['gate']['performance_gate'] is False and m['gate']['max_abs']==0.02 and m['gate']['rmse']==0.005 and m['gate']['exact_dispatches']==2
assert m['gate']['mapping_equivalence_required'] is True
print('ArcLLM P8-C static contract PASS')
