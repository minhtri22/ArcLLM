#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"src/sa0_capability_probe.cpp"
BUILD=ROOT/"tools/build_sa0_cap.ps1"
RUN=ROOT/"run_sa0_capability_preflight.ps1"
CFG=ROOT/"config/sa0_capability_preflight_v0.1.json"
DOC=ROOT/"docs/SA0_CAPABILITY_PREFLIGHT_IMPLEMENTATION.md"
MAN=ROOT/"manifest.json"

def req(cond,msg):
    if not cond: raise AssertionError(msg)

cfg=json.loads(CFG.read_text(encoding="utf-8"))
man=json.loads(MAN.read_text(encoding="utf-8"))
s=SRC.read_text(encoding="utf-8")
b=BUILD.read_text(encoding="utf-8")
r=RUN.read_text(encoding="utf-8")
d=DOC.read_text(encoding="utf-8")

req(cfg["schema"]=="arcllm.sa0.capability_preflight.v0.1","schema")
req(cfg["status"]=="SA0_CAP_PROBE_IMPLEMENTATION_STATIC_LOCKED","status")
req(cfg["parent_sa0_commit"]=="1ade625ae60acaa9ebeb43890d53a198c200d576","parent binding")
for k in ["model_load_permitted","shader_execution_permitted","successor_kernel_implementation_permitted","target_benchmark_permitted"]:
    req(cfg[k] is False,f"{k} must be false")

for forbidden in ["vkCreateDevice","vkCreateShaderModule","vkCreateComputePipelines","vkCmdDispatch","vkQueueSubmit","gguf","GGUF"]:
    req(forbidden not in s,f"probe forbidden token: {forbidden}")
for required in [
    "vkCreateInstance","vkEnumeratePhysicalDevices","vkGetPhysicalDeviceProperties2",
    "vkGetPhysicalDeviceFeatures2","vkGetPhysicalDeviceQueueFamilyProperties",
    "vkGetPhysicalDeviceMemoryProperties","VkPhysicalDeviceSubgroupProperties",
    "VkPhysicalDevice8BitStorageFeatures","VkPhysicalDevice16BitStorageFeatures",
    "VkPhysicalDeviceShaderFloat16Int8Features","VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME",
    "vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR"
]:
    req(required in s,f"probe missing query: {required}")

req("compile_q2_shaders" not in b and "glsl" not in b.lower(),"BuildOnly must not compile shaders")
req("src\\sa0_capability_probe.cpp" in b,"build source")
for forbidden in ["run_q2.ps1","run_q3.ps1","--model","resolve_p8_target","compile_q2_shaders"]:
    req(forbidden not in r,f"preflight forbidden call: {forbidden}")
for required in [
    "test_sa0_capability_package.py","build_sa0_cap.ps1","PASS_QUERY",
    "scientific_workload","NOT_RUN","model_loaded","shader_modules_created",
    "compute_pipelines_created","dispatches_submitted","Arc*140V*","32.0.101.8860",
    "sa0_capability_preflight_lock.json","sa0_capability_return_to_chatgpt.zip"
]:
    req(required in r,f"preflight missing guard: {required}")

req(man["status"]=="SA0_CAP_PROBE_IMPLEMENTATION_STATIC_LOCKED","manifest status")
req(man["implementation_permitted"] is False,"successor implementation remains closed")
req(man["target_run_permitted"] is False,"target remains closed")
req(man["q3_permitted"] is False,"Q3 remains closed")
req(man["sa0"]["successor_implementation_authorized"] is False,"SA0 successor implementation false")
req(man["sa0"]["target_model_execution_authorized"] is False,"SA0 target execution false")
req(man["sa0"]["capability_probe"]["scientific_workload"]=="NOT_RUN","zero science")
req("NO model load" in d and "NO shader module" in d and "NO dispatch" in d,"doc zero-science boundary")

print("SA0_CAP_STATIC_QA_PASS")
