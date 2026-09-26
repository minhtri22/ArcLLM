import json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
K=json.loads((R/"config/arcllm_v1_phase2_package_contract_v0.2.json").read_text())
A=(R/"include/arcllm/v1/package_api.h").read_text()
B=(R/"include/arcllm/v1/backend_api.h").read_text()
C=(R/"src/arcllm_v1_phase2_package.cpp").read_text()
T=(R/"tests/arcllm_v1_phase2_package_zero_science.cpp").read_text()
def req(x,m):
    if not x: raise AssertionError(m)
req(K["status"]=="FROZEN_PHASE2_PACKAGE_API_BOUNDARY_V0_2","contract")
for bad in ["Vk","SPIR-V",".spv","shader","sidecar_path","workgroup"]:
    req(bad not in A and bad not in B,f"hardware leak {bad}")
req("PROFILE_0" in A and "PROFILE_1" in A,"opaque profiles")
for x in ["cleanup_status","unreleased_representation","First failure"]:
    req(x in B,f"backend cleanup contract missing {x}")
for x in ["record_cleanup","preserve_existing_failure","Keep the handle because backend did not confirm release"]:
    req(x in C,f"implementation cleanup contract missing {x}")
for x in ["primary failure survives cleanup fail","evict failure preserves handle","no hidden retry"]:
    req(x in T,f"QA missing {x}")
print("PHASE2_PACKAGE_API_STATIC_QA=PASS")
