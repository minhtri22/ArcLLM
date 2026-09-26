import json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
K=json.loads((R/"config/arcllm_v1_phase2_direct_execution_holdout_generalization_gate_v0.1.json").read_text())
T=(R/"tests/arcllm_v1_direct_execution_holdout_generalization_gate.cpp").read_text()
P=(R/"src/arcllm_v1_generic_policy_engine_v2.cpp").read_text()
def req(x,m):
    if not x: raise AssertionError(m)
req(K["status"]=="FROZEN_DIRECT_EXECUTION_PRIMITIVE_HOLDOUT_GENERALIZATION_GATE","gate status")
req(K["holdout_family"]["result"]=="PASS_I002_REAL_MODEL_CARRY_THROUGH","holdout evidence")
req(K["holdout_family"]["representation_change"] is False,"representation boundary")
req(K["holdout_family"]["acquisition_action"] is False,"acquisition boundary")
req(K["holdout_family"]["additional_residency"] is False,"residency boundary")
req("COUNTEREXAMPLE_DIRECT_READY" in T and "RESIDENCY_OVERLOAD" in T,"counterexample coverage")
req("if(pr&&pr->resident)" in P,"frozen v2 residency gate changed")
req("for(std::size_t b=0;b<registry.bundle_count();++b)" in P,"frozen acquisition scan changed")
print("PHASE2_DIRECT_EXECUTION_HOLDOUT_GATE_STATIC_QA=PASS")
