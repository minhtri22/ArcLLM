from pathlib import Path
import json
R=Path(__file__).resolve().parents[1]
A=(R/"include/arcllm/v1/package_api.h").read_text()
B=(R/"include/arcllm/v1/backend_api.h").read_text()
C=(R/"src/arcllm_v1_phase2_package.cpp").read_text()
K=json.loads((R/"config/arcllm_v1_phase2_package_contract_v0.1.json").read_text())
def req(x,m):
    if not x: raise AssertionError(m)
req(K["status"]=="FROZEN_PHASE2_PACKAGE_API_BOUNDARY","contract status")
for s in ["CapabilityId","EvidenceProfileId","PrimitiveId","AcquisitionId","LifecycleAction","PlanRequest","CapabilityDescriptor"]:
    req(s in A,f"public API missing {s}")
for bad in ["Vk","SPIR-V",".spv","shader","sidecar_path","workgroup"]:
    req(bad not in A and bad not in B,f"hardware detail leaked: {bad}")
req("PROFILE_0" in A and "PROFILE_1" in A,"opaque profiles")
req("automatic" in B and "retry" in B,"no retry contract")
req("549527552" in C,"residency mapping")
req("Workload::WS" in C and "Workload::WC" in C,"internal evidence profile mapping")
req("fallback_unsupported" in C,"fail closed unsupported")
print("PHASE2_PACKAGE_API_STATIC_QA=PASS")
