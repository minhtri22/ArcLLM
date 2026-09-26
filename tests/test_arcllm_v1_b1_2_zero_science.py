from pathlib import Path
import json
import re

ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/"src/arcllm_v1_b1_2_zero_science.cpp").read_text(encoding="utf-8")
SH=(ROOT/"shaders/b1_2_exec148_gpu_materialize.comp").read_text(encoding="utf-8")
AUTH=json.loads((ROOT/"config/arcllm_v1_b1_2_p1_p3_implementation_authorization_v0.1.json").read_text(encoding="utf-8"))
PRE=json.loads((ROOT/"config/arcllm_v1_b1_2_p1_p3_placement_experiment_prelock_v0.1.json").read_text(encoding="utf-8"))

def req(c,m):
    if not c:
        raise AssertionError(m)

req(AUTH["status"]=="BOUNDED_P1_P3_IMPLEMENTATION_AND_ZERO_SCIENCE_QUALIFICATION_AUTHORIZED","authorization status")
req(AUTH["authorization"]["bounded_implementation"] is True,"implementation auth")
req(AUTH["authorization"]["zero_science_qualification"] is True,"zero-science auth")
req(AUTH["authorization"]["P1_performance_execution"] is False,"P1 performance must stay closed")
req(AUTH["authorization"]["P3_performance_execution"] is False,"P3 performance must stay closed")
req(AUTH["authorization"]["any_acquisition_timing"] is False,"acquisition timing must stay closed")
req(PRE["immutable_scientific_contract"]["exact_exec148_family_hash"]=="60565f9f0b12de4884e884d8311263df7238679745c83393a695935cd3eccbb2","canonical hash")
req(PRE["immutable_scientific_contract"]["exec_image_bytes"]==549527552,"image bytes")

# No B1.2 timing instrumentation or inspection is permitted in the qualifier source.
for forbidden in [
    "std::chrono",
    "QueryPerformanceCounter",
    "GetTickCount",
    "execute_profiled(",
    "execute_targeted_profiled(",
    "record_submit_wait_ms",
    "submit_wait_ms",
    "timestamp_period_ns",
    "op_ticks",
]:
    req(forbidden not in SRC,f"forbidden timing surface: {forbidden}")

for required in [
    'kCanonicalFamilyHash[]="60565f9f0b12de4884e884d8311263df7238679745c83393a695935cd3eccbb2"',
    "kExecFamilyBytes",
    "kSourceLayerBytes",
    "kBlocksPerLayer=kRows*kBlocksPerRow",
    "kP1LocalSize=256u",
    "kP1Groups=kBlocksPerLayer/kP1LocalSize",
    "ops.reserve(14u)",
    "kFrozenWeightArenas",
    "arena_bytes!=4677120000ull",
    "src_base[slot]=uint32_t(base)",
    '"b1_2_exec148_gpu_materialize.comp.spv"',
    "FILE_FLAG_NO_BUFFERING",
    "FILE_FLAG_SEQUENTIAL_SCAN",
    "kP3ChunkBytes=4ull*1024ull*1024ull",
    "VirtualAlloc",
    "buffered_preload(sidecar)",
    "buffered_load(sidecar",
    "unbuffered_load(sidecar",
    '\\"acquisition_timing_emitted\\":false',
    '\\"gpu_timing_emitted\\":false',
    '\\"storage_timing_emitted\\":false',
]:
    req(required in SRC,f"missing frozen zero-science contract: {required}")

req("compression" in SRC and "false" in SRC,"P3 no compression contract")
req("materialize_tensor(src[slot],layer.data())" in SRC,"P3 must use frozen CPU materializer offline")
req("component_oracle" in SRC and "max_abs<=0.02" in SRC and "rmse(g)<=0.005" in SRC,"component oracle")
req("src_base_bytes" in SRC and "dst_base_bytes" in SRC,"P1 source/destination base contract")

# Prevent MSVC most-vexing-parse declarations for size_t(...) vectors.
req(re.search(r"std::vector<[^>]+>\\s+\\w+\\s*\\(\\s*size_t\\s*\\(", SRC) is None,
    "ambiguous vector(size_t(...)) declaration")

# Shader is exactly block-parallel EXEC148 transformation.
for required in [
    "layout(local_size_x = 256",
    "readonly buffer SrcQ4K",
    "writeonly buffer DstExec148",
    "uint src_base_bytes;",
    "uint dst_base_bytes;",
    "uint block_count;",
    "block_id * 144u",
    "block_id * 148u",
    "scale_value",
    "min_value",
    "qpair",
]:
    req(required in SH,f"missing P1 shader contract: {required}")
req("atomic" not in SH.lower(),"P1 transform must not use atomics")
req("float" not in SH,"P1 exact-byte transform must not use floating point")

# Static geometry checks.
req(3584*74==3713024//14,"block geometry")
req(3584*74==265216,"blocks per layer")
req(265216%256==0 and 265216//256==1036,"P1 workgroup geometry")
req(14*39251968==549527552,"EXEC148 family geometry")

print("B1_2_ZERO_SCIENCE_STATIC_QA=PASS")
print("NO_MODEL_LOAD. NO_GPU_DISPATCH. NO_ACQUISITION_TIMING.")
