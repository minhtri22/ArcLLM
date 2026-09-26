from pathlib import Path
import json
R=Path(__file__).resolve().parents[1]
MODEL=(R/"include/arcllm/v1/primitive_registry_model.h").read_text()
API=(R/"include/arcllm/v1/primitive_registry_api.h").read_text()
CORE=(R/"src/arcllm_v1_primitive_registry.cpp").read_text()
REF=(R/"src/registrations/arcllm_v1_q4k_down_reference_registration.cpp").read_text()
TEST=(R/"tests/arcllm_v1_generic_primitive_registry_zero_science.cpp").read_text()
CFG=json.loads((R/"config/arcllm_v1_phase2_generic_primitive_registry_contract_v0.1.json").read_text())

def req(x,m):
    if not x: raise AssertionError(m)

req(CFG["status"]=="FROZEN_GENERIC_PRIMITIVE_CAPABILITY_MODEL_AND_REGISTRY_CONTRACT","contract")
for token in ["OpaqueId","RegistrationBundle","ValidationState","EvidenceSetDescriptor","AcquisitionThresholdDescriptor","LifecycleDescriptor"]:
    req(token in MODEL,f"model missing {token}")
for token in ["add_bundle","find_capability","find_primitive","find_threshold","find_resident_preference","find_lifecycle"]:
    req(token in API,f"api missing {token}")

# Generic machinery must remain family-agnostic.
for forbidden in ["Q4K","EXEC148","SPLIT_K32","GPU_IN_PLACE","P3_COLD","W-S","W-C","549527552","0x0000A001","0x0000B001"]:
    req(forbidden not in MODEL and forbidden not in API and forbidden not in CORE,
        f"family-specific literal leaked into generic core: {forbidden}")

# Family-specific facts belong only in registration data.
for token in ["Q4K_DOWN_REFERENCE_REGISTRATION","A_SPLIT_K32","B_EXEC148","P1_GPU_IN_PLACE","P3_WARM_PAGE_CACHE","DISABLED_BY_EVIDENCE","549527552ull"]:
    req(token in REF,f"reference registration missing {token}")

req("SYNTHETIC_SECOND_FAMILY" in TEST,"second-family scalability proof missing")
req("duplicate family rejected" in TEST and "invalid reference rejected" in TEST,"registry integrity QA missing")
req(CFG["legacy_v1_package"]["new_family_enum_extension_forbidden"] is True,"legacy enum extension must be forbidden")
req(CFG["scaling_contract"]["new_family_requires_generic_registry_core_change"] is False,"registry core must not change per family")
print("PHASE2_GENERIC_PRIMITIVE_REGISTRY_STATIC_QA=PASS")
