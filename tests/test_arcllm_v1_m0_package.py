from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[1]
REQ=[
 "src/arcllm_v1_m0_tensor_census.cpp",
 "src/gguf.cpp","src/gguf.h",
 "tools/build_arcllm_v1_m0.ps1",
 "run_arcllm_v1_m0.ps1",
 "config/arcllm_v1_m0_tensor_census_lock_v0.1.json",
 "config/p8_target.json",
 "artifacts/ARCLLM_V1/ARCLLM_V1_ONE_TOKEN_HARDWARE_MODEL_v0.1.json"
]
for p in REQ: assert (ROOT/p).is_file(),p
s=(ROOT/"src/arcllm_v1_m0_tensor_census.cpp").read_text(encoding="utf-8")
for x in ["arcllm.v1.m0.tensor_census.v0.1","METADATA_ONLY_NO_INFERENCE_NO_PERFORMANCE","EXPECT_TARGET_Q4=28","EXPECT_TARGET_Q6=28",".attn_v.weight",".ffn_down.weight","patch_manifest","no_inference","no_performance_measurement"]:
    assert x in s,x
for bad in ["#include <vulkan","VkRuntime","vkCreate","vkQueue","chrono","QueryPerformanceCounter","benchmark"]:
    assert bad not in s,bad
b=(ROOT/"tools/build_arcllm_v1_m0.ps1").read_text(encoding="utf-8")
refs=re.findall(r"Join-Path \$SrcDir '([^']+\.cpp)'",b)
assert refs==["arcllm_v1_m0_tensor_census.cpp","gguf.cpp"],refs
r=(ROOT/"run_arcllm_v1_m0.ps1").read_text(encoding="utf-8")
for x in ["Get-FileHash","git -C $Root hash-object","test_arcllm_v1_m0_package.py","INFERENCE_EXECUTED=false","PERFORMANCE_MEASURED=false","arcllm_v1_m0_return_to_chatgpt.zip"]:
    assert x in r,x
lock=json.loads((ROOT/"config/arcllm_v1_m0_tensor_census_lock_v0.1.json").read_text(encoding="utf-8"))
assert lock["execution_class"]=="METADATA_ONLY_NO_INFERENCE_NO_PERFORMANCE"
assert lock["performance_measurement_authorized"] is False
assert lock["inference_authorized"] is False
assert lock["expected_target_summary"]=={"target_tensor_count":56,"Q4_K":28,"Q6_K":28}
model=json.loads((ROOT/"artifacts/ARCLLM_V1/ARCLLM_V1_ONE_TOKEN_HARDWARE_MODEL_v0.1.json").read_text(encoding="utf-8"))
assert len(model["expanded_decode_dispatch_ledger"])==469
print("ArcLLM v1 M0 static package PASS")
