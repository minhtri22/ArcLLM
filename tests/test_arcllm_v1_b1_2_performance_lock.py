from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/"src/arcllm_v1_b1_2_performance.cpp").read_text(encoding="utf-8")
ZERO=(ROOT/"src/arcllm_v1_b1_2_zero_science.cpp").read_text(encoding="utf-8")

def req(c,m):
    if not c:
        raise AssertionError(m)

# Performance wrapper must embed zero-science helpers without compiling its standalone main.
req(SRC.startswith('#define ARCLLM_B1_2_EMBED_LIBRARY 1\n#include "arcllm_v1_b1_2_zero_science.cpp"\n#undef ARCLLM_B1_2_EMBED_LIBRARY\n'),
    "performance wrapper embed prologue drift")
req('#define main arcllm_b1_2_zero_science_main_disabled' not in SRC,
    "legacy macro main-renaming path must not return")
req('#ifndef ARCLLM_B1_2_EMBED_LIBRARY' in ZERO and '#endif // ARCLLM_B1_2_EMBED_LIBRARY' in ZERO,
    "zero-science standalone main embed guard missing")

required=[
    'kB12Attempts=8u',
    'kP0ReferenceMs=231.6382',
    'kExpectedSidecarRawSha[]="3f168749256e8acbbbe61da06196ca0e51a249b3923938e5651a4da6f50ab43f"',
    'auto p1_chain=vk.prepare_chain(p1_ops);',
    'for(uint32_t block=0;block<kB12Attempts;++block)',
    'P1 -> P3-WARM -> P3-COLD',
    'p1.samples.push_back(ps.record_submit_wait_ms)',
    'buffered_preload(sidecar);',
    'buffered_load(sidecar',
    'unbuffered_load(sidecar',
    'validate_attempt(vk,src_cpu,src_gpu,src_base,exec,shader_dir)',
    'PASS_COMPLETE_24_ATTEMPT_COLLECTION',
    '"warmup_attempts\\":0',
    '"candidate_specific_retries\\":false',
    'all_attempts_validated_outside_primary_timer',
]
for x in required:
    req(x in SRC,f"missing frozen performance contract: {x}")

req('p1.samples.push_back(ps.submit_wait_ms)' not in SRC,
    "P1 primary metric must not use submit_wait_ms")
req(SRC.count('p1.samples.push_back(')==1,"P1 sample emission path must be singular")
req(SRC.count('warm.samples.push_back(')==1,"P3 warm sample path must be singular")
req(SRC.count('cold.samples.push_back(')==1,"P3 cold sample path must be singular")

loop=SRC.index('for(uint32_t block=0;block<kB12Attempts;++block)')
outopen=SRC.index('std::ofstream o(out',loop)
req(outopen>loop,"result output must occur after measurement loop")
for token in ['write_samples(o,p1.samples)','write_samples(o,warm.samples)','write_samples(o,cold.samples)']:
    req(SRC.index(token)>outopen,f"timing samples must only be emitted after completed loop: {token}")

# P3 wall timer anchors must immediately bound only the qualified load functions.
req(re.search(r'auto wt0=std::chrono::steady_clock::now\(\);\s*buffered_load\(sidecar,reinterpret_cast<uint8_t\*>\(exec\.mapped\)\);\s*auto wt1=std::chrono::steady_clock::now\(\);',SRC,re.S) is not None,
    "P3 warm timer boundary drift")
req(re.search(r'auto ct0=std::chrono::steady_clock::now\(\);\s*unbuffered_load\(sidecar,reinterpret_cast<uint8_t\*>\(exec\.mapped\),sector\);\s*auto ct1=std::chrono::steady_clock::now\(\);',SRC,re.S) is not None,
    "P3 cold timer boundary drift")

# No retries, adaptive tuning, compression or overlap paths.
for bad in ['retry','autotune','tune_geometry','compression=true','overlap=true']:
    req(bad.lower() not in SRC.lower(),f"forbidden adaptive/rescue surface: {bad}")

# Exact frozen geometry must be inherited, not redefined differently.
req('build_p1_ops(src_gpu,src_base,exec,shader_dir)' in SRC,"P1 must reuse qualified builder")
req('kFrozenWeightArenas' in SRC and '4677120000ull' in SRC,"frozen 19-arena topology required")
req('kExecFamilyBytes' in SRC,"frozen EXEC148 bytes required")

# Failure path must not write the result JSON with partial samples.
catch=SRC.index('}catch(const std::exception&e)')
req('std::ofstream o(out' not in SRC[catch:],"failure path must not emit partial result JSON")

print("B1_2_PERFORMANCE_LOCK_STATIC_QA=PASS")
print("NO MODEL LOAD. NO GPU DISPATCH. NO PERFORMANCE EXECUTION.")
