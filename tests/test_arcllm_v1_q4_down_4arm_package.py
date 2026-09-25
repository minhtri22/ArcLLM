import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
LOCK = ROOT / "config" / "arcllm_v1_q4_down_4arm_correctness_lock_v0.1.json"
L = json.loads(LOCK.read_text(encoding="utf-8"))

def req(cond, msg):
    if not cond:
        raise AssertionError(msg)

req(L["schema"] == "arcllm.v1.q4_down.4arm.correctness_lock.v0.1", "schema")
req(L["target"]["layers"] == [3,4,6,7,8,11,12,14,15,17,18,19,21,22], "target layers")
req(L["target"]["rows"] == 3584 and L["target"]["k"] == 18944 and L["target"]["blocks_per_row"] == 74, "geometry")

E = L["exec148"]
req(E["source_block_bytes"] == 144 and E["exec_block_bytes"] == 148, "block geometry")
req(E["source_row_bytes"] == 10656 and E["exec_row_bytes"] == 10952, "row geometry")
req(E["source_tensor_bytes"] == 38191104 and E["exec_tensor_bytes"] == 39251968, "tensor geometry")
req(E["per_tensor_overhead_bytes"] == 1060864 and E["family_overhead_bytes"] == 14852096, "overhead")
req(E["incremental_resident_bytes"] == 549527552, "incremental residency")
req(E["materialization_threads"] == 8 and not E["lazy_materialization"], "materialization policy")

R = L["residency"]
req(R["max_correctness_resident_bytes"] == R["parent_resident_bytes"] + E["incremental_resident_bytes"], "residency arithmetic")
req(R["max_correctness_resident_bytes"] < R["frozen_usable_envelope_bytes"], "residency envelope")

req(not L["performance_authorized"], "performance lock")
req(not L["hardware_counters_authorized"], "counter lock")
req(not L["timing_authorized"], "timing lock")
req(L["materialization_time_ms"] is None and L["validation_time_ms"] is None, "timing null")
req(not L["prefill_modified"] and not L["q6_down_modified"] and not L["token_xray_in_scope"], "scope lock")

req(L["arms"]["0"]["workgroups"] == 56 and L["arms"]["B"]["workgroups"] == 56, "serial workgroups")
req(L["arms"]["A"]["subgroup_size"] == 32 and L["arms"]["AB"]["subgroup_size"] == 32, "subgroup32")
req(L["arms"]["A"]["workgroups"] == 896 and L["arms"]["AB"]["workgroups"] == 896, "split workgroups")

req(L["target_model"]["sha256"] == "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463", "model hash")
req(L["correctness"]["workloads"]["W-S"]["generated_hash_fnv1a64"] == "f31d4bb9fe5eb9c3", "W-S generated hash")
req(L["correctness"]["workloads"]["W-C"]["generated_hash_fnv1a64"] == "471519ddc45b232e", "W-C generated hash")

for rel, sha in L["critical_git_blobs"].items():
    got = subprocess.check_output(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD:" + rel],
        text=True,
    ).strip()
    req(got == sha, f"blob drift {rel}: {got} != {sha}")

h = (ROOT / "src" / "arcllm_v1_q4_down_arch_4arm.cpp").read_text(encoding="utf-8")
req("#define chrono arcllm_no_timing" in h and "#undef chrono" in h, "inherited chrono not neutralized")
req("std::chrono::steady_clock::now()" not in h and "std::chrono::duration" not in h, "direct real timing call in correctness harness")
req("vkCmdWriteTimestamp" not in h and "VkQueryPool" not in h, "GPU timing primitive in correctness harness")
req('latency_fields_emitted\\":false' in h, "latency output lock")
req('materialization_time_ms\\":null' in h and 'validation_time_ms\\":null' in h, "timing null output lock")
req("q4_down_exec148_serial.spv" in h and "q4_down_exec148_splitk32.spv" in h, "B/AB shader wiring")
req("sa1_q4k_subgroup_splitk.spv" in h and "p7_q4k_gemm_2d.spv" in h, "0/A shader wiring")
req("Token-XRay" not in h and "TOKEN_XRAY" not in h, "Token-XRay leakage")

mat = (ROOT / "src" / "q4_down_exec148_materializer.h").read_text(encoding="utf-8")
req("const uint8_t qj=s[8u+j]" in mat, "packed scale/min offset regression")
req("validator_source_tuple" in mat and "validator_exec_tuple" in mat and "Sha256" in mat, "independent validators")

serial = (ROOT / "shaders" / "q4_down_exec148_serial.comp").read_text(encoding="utf-8")
split = (ROOT / "shaders" / "q4_down_exec148_splitk32.comp").read_text(encoding="utf-8")
req("ib*148u" in serial and "base+20u" in serial, "B EXEC148 decode")
req("ib*148u" in split and "base+20u" in split and "subgroupAdd" in split, "AB EXEC148/subgroup")

print("Q4_DOWN_4ARM_STATIC_QA=PASS")
