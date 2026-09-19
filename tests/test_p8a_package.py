from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[1]
REQ=['config/p8_target.json','docs/P8A_CONTRACT.md','docs/P8A_IMPLEMENTATION.md','src/p8a_memory_plan.cpp','tools/build_p8a.ps1','tools/resolve_p8_target.ps1','tools/fetch_p8_target.ps1','run_p8a.ps1','manifest.json']
for r in REQ: assert (ROOT/r).is_file(),r
t=json.loads((ROOT/'config/p8_target.json').read_text(encoding='utf-8'))
assert t['schema']=='arcllm.p8.target.v1'
assert t['source']=='ollama-local'
assert t['ollama_repository']=='registry.ollama.ai/library/qwen2.5-coder'
assert t['model_layer_media_type']=='application/vnd.ollama.image.model'
assert t['model_layer_digest']=='sha256:60e05f2100071479f596b964f89f510f057ce397ea22f2833a0cfe029bfc2463'
assert t['sha256']=='60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463'
assert t['size_bytes']==4683074048
assert t['superseded_pre_execution_candidate']['sha256']=='509287F78CB4D4CF6B3843734733B914B2C158E43E22A7F4BF5E963800894D3C'
assert t['superseded_pre_execution_candidate']['size_bytes']==4683073536
assert t['max_context_initial']==4096 and t['prefill_chunk_initial']==512
assert t['arena_cap_bytes']==268435456
s=(ROOT/'src/p8a_memory_plan.cpp').read_text(encoding='utf-8')
assert 'EXPECTED_FILE_BYTES=4683074048ull' in s
assert '4683073536ull' not in s
assert 'VkRuntime' not in s and 'vkCreate' not in s and '#include <vulkan' not in s
assert 'TensorStore' not in s
assert 'arcllm.p8a.memory_plan.v1' in s
b=(ROOT/'tools/build_p8a.ps1').read_text(encoding='ascii')
cpp_refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert cpp_refs==['p8a_memory_plan.cpp','gguf.cpp'],cpp_refs
res=(ROOT/'tools/resolve_p8_target.ps1').read_text(encoding='ascii')
assert 'config\\p8_target.json' in res
assert 'OLLAMA_MODELS' in res and '.ollama\\models' in res
assert 'Get-ChildItem -Path $ManifestDir -File -Recurse' in res
assert '$Layer.mediaType' in res and '$Layer.digest' in res and '$Layer.size' in res
assert 'model_layer_digest' in res and 'blob_relative' in res
assert 'Get-FileHash' not in res  # hash is independently enforced by runner
r=(ROOT/'run_p8a.ps1').read_text(encoding='ascii')
assert 'config\\p8_target.json' in r
assert 'tools\\resolve_p8_target.ps1' in r
assert 'Get-FileHash' in r
assert 'target_source="ollama-local-auto"' in r
assert 'full_inference_permitted' in r
assert 'curl.exe' not in r
f=(ROOT/'tools/fetch_p8_target.ps1').read_text(encoding='ascii')
assert 'resolve_p8_target.ps1' in f
assert 'no network download is performed' in f
assert 'curl.exe' not in f
m=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
assert m['phase']=='P8-A' and m['revision']=='R1' and m['status']=='READY_TO_RUN'
assert m['target']['source']=='ollama-local' and m['target']['auto_discovery'] is True
assert m['target']['sha256']==t['sha256'] and m['target']['size_bytes']==t['size_bytes']
assert m['superseded_pre_execution_target']['executed'] is False
assert m['gate']['performance_gate'] is False
assert m['gate']['full_inference_forbidden_until_pass'] is True
assert m['gate']['exact_blob_identity_required'] is True
print('ArcLLM P8-A-R1 static contract PASS')
