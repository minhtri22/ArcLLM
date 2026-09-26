from pathlib import Path
import json
R=Path(__file__).resolve().parents[1]
H=(R/"include/arcllm/v1/generic_policy_engine.h").read_text()
C=(R/"src/arcllm_v1_generic_policy_engine.cpp").read_text()
T=(R/"tests/arcllm_v1_generic_policy_engine_zero_science.cpp").read_text()
K=json.loads((R/"config/arcllm_v1_phase2_generic_policy_engine_contract_v0.2.json").read_text())
M=(R/"include/arcllm/v1/primitive_registry_model.h").read_text()

def req(x,m):
    if not x: raise AssertionError(m)

req(K["status"]=="FROZEN_PHASE2_GENERIC_POLICY_ENGINE_CONTRACT_V0_2","contract status")
for token in ["PrimitiveRuntimeState","AcquisitionRuntimeState","PolicyRequest","PolicyDecision","evaluate"]:
    req(token in H,f"header missing {token}")

for forbidden in ["Q4K","EXEC148","SPLIT_K32","GPU_IN_PLACE","P3_COLD","W-S","W-C","549527552","0x0000A001","0x0000B001"]:
    req(forbidden not in H and forbidden not in C,f"family literal leaked into generic policy: {forbidden}")

for token in ["ValidationState::VALIDATED","bundle_at","find_threshold","find_resident_preference","find_lifecycle","priority"]:
    req(token in C,f"engine not registry driven: {token}")

for token in ["evict_on_execution_unavailable","evict_on_model_unload"]:
    req(token in M and token in C,f"lifecycle completeness missing {token}")

req("REFERENCE_EQUIVALENCE_CASES" in T and "114688" in T,"reference exhaustive equivalence missing")
req("generic tertiary diagnostic threshold normalization" in T,"legacy diagnostic normalization coverage missing")
req("SYNTHETIC_POLICY_FAMILY" in T,"synthetic policy proof missing")
req("CAPABILITY_GATED" in T and "DISABLED_BY_EVIDENCE" in T,"evidence-state filtering proof missing")
req(K["generic_rules"]["family_specific_literals_in_engine"] is False,"family literal contract")
print("PHASE2_GENERIC_POLICY_ENGINE_STATIC_QA=PASS")
