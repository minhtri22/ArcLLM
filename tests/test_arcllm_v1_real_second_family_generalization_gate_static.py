import json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
K=json.loads((R/"config/arcllm_v1_phase2_real_second_family_generalization_gate_v0.1.json").read_text())
T=(R/"tests/arcllm_v1_real_second_family_generalization_gate.cpp").read_text()
M=(R/"include/arcllm/v1/primitive_registry_model.h").read_text()
P=(R/"src/arcllm_v1_generic_policy_engine.cpp").read_text()
def req(x,m):
    if not x: raise AssertionError(m)
req(K["status"]=="FROZEN_REAL_SECOND_FAMILY_GENERALIZATION_GATE","gate status")
req(K["candidate_selection"]["selected"]=="P8_SEGMENTED_RESIDENCY_AND_GRAPH_BINDING_FAMILY","candidate drift")
req(K["p8_evidence"]["p8a_contiguous_plan"]["status"]=="FAIL","P8A evidence")
req(K["p8_evidence"]["p8b_residency"]["status"]=="PASS","P8B evidence")
req(K["p8_evidence"]["p8g_larger_prefix"]["status"]=="FAIL","P8G boundary")
req(K["p8_evidence"]["full_inference_permitted"] is False,"full inference boundary")
for token in ["COUNTEREXAMPLE_1","COUNTEREXAMPLE_2","FAIL_ABSTRACTION_INCOMPLETE","5347770372ull"]:
    req(token in T,f"gate harness missing {token}")
for token in ["evict_on_execution_unavailable","evict_on_model_unload"]:
    req(token in M,f"frozen registry model unexpected drift {token}")
req("if (!r.future_reuse_known)" in P and "if (r.future_reuse_units == 0)" in P,
    "frozen reuse prerequisite changed; gate no longer tests intended engine")
print("PHASE2_REAL_SECOND_FAMILY_GATE_STATIC_QA=PASS")
