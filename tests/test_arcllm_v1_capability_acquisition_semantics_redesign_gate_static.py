import json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
K=json.loads((R/"config/arcllm_v1_phase2_capability_acquisition_semantics_redesign_gate_v0.1.json").read_text())
M=(R/"include/arcllm/v1/primitive_registry_v2.h").read_text()
P=(R/"src/arcllm_v1_generic_policy_engine_v2.cpp").read_text()
T=(R/"tests/arcllm_v1_capability_acquisition_semantics_redesign_gate.cpp").read_text()
P8=(R/"src/registrations/arcllm_v1_p8_segmented_reference_registration_v2.cpp").read_text()
Q4=(R/"src/registrations/arcllm_v1_q4k_down_reference_registration_v2.cpp").read_text()

def req(x,m):
    if not x: raise AssertionError(m)

req(K["status"]=="FROZEN_CAPABILITY_ACQUISITION_SEMANTICS_REDESIGN_GATE","gate status")
for token in ["AcquisitionTrigger","REUSE_AMORTIZED","MANDATORY_FOR_FEASIBILITY","fallback_primitive{}","preferred_primitive{}"]:
    req(token in M,f"model missing {token}")
for token in ["PolicyStatus::NOT_READY","PolicyStatus::OUTSIDE_VALIDATED_CAPABILITY","MANDATORY_FOR_FEASIBILITY","request_within_capability_evidence_scope"]:
    req(token in P,f"policy missing {token}")
req("nullptr,0,kEvidence" in P8,"P8 must register zero reuse metrics")
req("MANDATORY_FOR_FEASIBILITY" in P8 and "nullptr,0,kLife" in P8,"P8 must have mandatory acquisition and zero thresholds")
req("REUSE_AMORTIZED" in Q4 and "549527552ull" in Q4,"first family evidence migration")
req("FIRST_FAMILY_EQUIVALENCE_CASES" in T and "114688" in T,"exhaustive first-family proof")
req("P8_REAL_FAMILY_ORACLE=PASS" in T,"P8 oracle proof")
req("mandatory threshold rejection" in T,"semantic guard")
for forbidden in ["Q4K","EXEC148","P8_","549527552","5347770372"]:
    req(forbidden not in P,f"family-specific literal leaked into v2 policy core: {forbidden}")
print("PHASE2_CAPABILITY_ACQUISITION_REDESIGN_STATIC_QA=PASS")
