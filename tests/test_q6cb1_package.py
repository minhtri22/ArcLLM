#!/usr/bin/env python3
from __future__ import annotations
import json,re,subprocess
from pathlib import Path

R=Path(__file__).resolve().parents[1]

def req(x,msg):
    if not x:
        raise AssertionError(msg)

def blob(path):
    return subprocess.check_output(["git","-C",str(R),"rev-parse","HEAD:"+path],text=True).strip()

auth=json.loads((R/"config/q6cb1_implementation_authorization_v0.1.json").read_text(encoding="utf-8"))
lock=json.loads((R/"config/q6cb1_implementation_lock_v0.1.json").read_text(encoding="utf-8"))
spec=json.loads((R/"config/q6cb1_specification_v0.1.json").read_text(encoding="utf-8"))
term=json.loads((R/"config/q6cb_termination_goal_alignment_v0.1.json").read_text(encoding="utf-8"))
man=json.loads((R/"manifest.json").read_text(encoding="utf-8"))
ref=(R/"src/q6cb_q6_reference.hpp").read_text(encoding="utf-8")
gen=(R/"src/q6cb_fixture_generator.hpp").read_text(encoding="utf-8")
cpp=(R/"src/q6cb_causal_harness.cpp").read_text(encoding="utf-8")
packed=(R/"shaders/q6cb_t32_gpu_packed.comp").read_text(encoding="utf-8")
expanded=(R/"shaders/q6cb_t32_gpu_expanded.comp").read_text(encoding="utf-8")
compile_ps=(R/"tools/compile_q6cb1.ps1").read_text(encoding="utf-8")
build_ps=(R/"tools/build_q6cb1.ps1").read_text(encoding="utf-8")

req(auth["decision"]=="Q6CB1_CAUSAL_HARNESS_IMPLEMENTATION_AUTHORIZED","implementation authorization")
req(auth["scope"]=="IMPLEMENTATION_AND_BUILDONLY_EVIDENCE_ONLY","authorization scope")
req(lock["status"]=="Q6CB1_IMPLEMENTATION_SCOPE_FROZEN","implementation lock status")
req(term["q6cb5_permitted"] is False and term["hard_budget"]["successor_interventions_max"]==1,"termination governance")
req(spec["authorization"]["scientific_cpu_execution"] is False and spec["authorization"]["scientific_gpu_execution"] is False,"spec execution closed")

# SA1 must remain immutable.
req(blob("shaders/sa1_q6k_subgroup_splitk.comp")=="0fdc0c8f195872396a653b38ee2283156fbaeaa0","SA1 Q6 candidate drift")
req(blob("artifacts/SA1/SA1_CLOSEOUT_v0.1.json")=="83b30549410ce89e8d5047685c8bcbddc87983ee","SA1 closeout drift")
req(blob("artifacts/SA1/SA1K2_Q6_ADJUDICATION_v0.1.json")=="a2419af5a98ec931424bda46de840d3495e0bc38","SA1 adjudication drift")

# Five causal arms are implemented explicitly.
for token in [
    "arm_r64","arm_s32","arm_t32_cpu",
    "T32_GPU_PACKED","T32_GPU_EXPANDED",
    "TOPOLOGY_T32CPU_vs_S32",
    "DEVICE_GPU_EXPANDED_vs_T32CPU",
    "PACKED_PATH_GPU_PACKED_vs_GPU_EXPANDED"
]:
    req(token in (ref+cpp),"missing causal arm/contrast: "+token)

# R64/S32/T32 CPU semantics.
req("double sum = 0.0" in ref,"R64 binary64 accumulation")
req("float sum = 0.0f" in ref,"S32 binary32 serial accumulation")
for token in ["k = l; k < n; k += kSplitLanes","stride = 16u","stride >>= 1u"]:
    req(token in ref,"T32 CPU fixed-tree invariant: "+token)

# Independent Q6 canonical decoder and exact 210-byte layout.
for token in ["kQ6BlockBytes = 210","block + 128u","block + 192u","block + 208u","int(low | (high << 4u)) - 32"]:
    req(token in ref,"canonical Q6 decoder invariant: "+token)

# Fresh-data exclusions are coded into the generator.
for token in [
    "s.n == 3584u && s.rows == 512u",
    "s.n == 18944u && s.rows == 3584u",
    "0x5341315046495831ull","0x5341315046495834ull",
    "LOW_CANCELLATION_BOUNDED_RANGE","HIGH_CANCELLATION",
    "SCALE_HETEROGENEITY","MIXED_SIGN_HIGH_DYNAMIC_RANGE",
    "NEUTRAL_RANDOM_CONTROL"
]:
    req(token in gen,"fixture generator invariant: "+token)
req("while" not in gen,"generator must not contain adaptive search loop")
for token in ["expected_q","expected_scale","expected_d_bits","verify_semantic_invariant","q_mismatches","scale_mismatches","d_mismatches"]:
    req(token in gen,"runtime semantic invariant missing: "+token)
req("\"semantic_invariant\"" in cpp and "semantic_invariant.pass" in cpp,"harness semantic invariant evidence missing")

# GPU arms preserve the same split-32 geometry.
for shader,name in [(packed,"packed"),(expanded,"expanded")]:
    for token in ["local_size_x = 128","gl_SubgroupID","gl_SubgroupInvocationID","k += 32u","subgroupAdd","gl_WorkGroupID.x * 4u"]:
        req(token in shader,f"{name} GPU topology invariant: {token}")
    req("shared " not in shader.lower(),f"{name} shared memory forbidden")
req("ib * 210u" in packed and "base + 208u" in packed,"packed direct Q6 decode")
req("load_u8" not in expanded and "float w[]" in expanded,"expanded arm must bypass packed decode")

# No performance instrumentation or model path in the causal harness.
for forbidden in ["vkCreateQueryPool","vkCmdWriteTimestamp","VK_QUERY_TYPE_TIMESTAMP","timestampPeriod","--model","llama_"]:
    req(forbidden not in cpp,"forbidden causal harness feature: "+forbidden)

# Fail-closed authorization check must precede fixture generation and Vulkan init.
auth_pos=cpp.find("require_future_execution_authorization(args.authorization)")
gen_pos=cpp.find("q6cb::generate_fixture(spec)")
vk_pos=cpp.find("vk.init()")
req(0 <= auth_pos < gen_pos < vk_pos,"authorization must precede fixture generation and GPU init")
req("Q6CB1_SCIENTIFIC_EXECUTION_AUTHORIZED" in cpp,"future authorization marker")
req(not (R/"run_q6cb1.ps1").exists(),"runner forbidden during Q6CB-1 implementation")

# BuildOnly tooling may compile/link but must not launch the built harness.
req("scientific_execution=$false" in compile_ps and "gpu_dispatch=$false" in compile_ps,"shader BuildOnly flags")
req("executable_launched=$false" in build_ps and "scientific_execution=$false" in build_ps,"native BuildOnly flags")
req("& $Exe" not in build_ps,"native BuildOnly must not launch harness")
req("q6cb_causal_harness.exe" in build_ps,"expected BuildOnly target")

# Manifest remains execution-closed even while implementation writing is open.
q=man["q6cb1"]
req(q["causal_harness_implementation_permitted"] is True,"implementation gate should be open")
for k in [
    "fresh_fixture_execution_permitted",
    "scientific_cpu_execution_permitted",
    "scientific_gpu_execution_permitted",
    "performance_timing_permitted",
    "target_model_load_permitted"
]:
    req(q[k] is False,"execution boundary violated: "+k)
req(q["execution_authorization_file"] is None,"execution authorization must not exist")

# Tiny non-scientific codec invariant: prove the documented low/high bit mapping round-trips.
block=bytearray(210)
def set_code(k,code):
    half=k>>7; kk=k&127; l=kk&31; quarter=kk>>5
    ql=half*64; qh=128+half*32
    lo=code&15; hi=(code>>4)&3
    if quarter==0:
        block[ql+l]=(block[ql+l]&0xF0)|lo; block[qh+l]=(block[qh+l]&0xFC)|hi
    elif quarter==1:
        block[ql+l+32]=(block[ql+l+32]&0xF0)|lo; block[qh+l]=(block[qh+l]&0xF3)|(hi<<2)
    elif quarter==2:
        block[ql+l]=(block[ql+l]&0x0F)|(lo<<4); block[qh+l]=(block[qh+l]&0xCF)|(hi<<4)
    else:
        block[ql+l+32]=(block[ql+l+32]&0x0F)|(lo<<4); block[qh+l]=(block[qh+l]&0x3F)|(hi<<6)
def get_code(k):
    half=k>>7; kk=k&127; l=kk&31; quarter=kk>>5
    ql=half*64; qh=128+half*32
    if quarter==0: return (block[ql+l]&15)|(((block[qh+l]>>0)&3)<<4)
    if quarter==1: return (block[ql+l+32]&15)|(((block[qh+l]>>2)&3)<<4)
    if quarter==2: return (block[ql+l]>>4)|(((block[qh+l]>>4)&3)<<4)
    return (block[ql+l+32]>>4)|(((block[qh+l]>>6)&3)<<4)
for k in range(256):
    code=(k*37+11)&63
    set_code(k,code)
for k in range(256):
    req(get_code(k)==((k*37+11)&63),"Q6 packed codec roundtrip")

print("Q6CB1_STATIC_UNIT_QA_PASS")
