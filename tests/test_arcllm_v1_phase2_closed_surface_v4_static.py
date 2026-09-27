from pathlib import Path

root=Path(__file__).resolve().parents[1]
h=(root/"include/arcllm/v1/generic_policy_engine_v4.h").read_text(encoding="utf-8")
s=(root/"src/arcllm_v1_generic_policy_engine_v4.cpp").read_text(encoding="utf-8")
bh=(root/"include/arcllm/v1/generic_backend_binding_v4.h").read_text(encoding="utf-8")
bs=(root/"src/arcllm_v1_generic_backend_binding_v4.cpp").read_text(encoding="utf-8")

assert "execution_ready" in h
assert "execution_available" in h
assert "resident" in h
assert "AcquisitionRuntimeState" in h
assert "LifecycleAction" in h
assert "runtime_routable" in s
assert "FALLBACK_NOT_READY" in h
assert "PREFERRED_NOT_READY" in h
assert "class BackendAdapter" in bh
assert "apply_decision" in bh
assert "fallback_a" not in bs
assert "retry" not in bs.lower()
assert "A_SPLIT_K32" not in s
assert "B_EXEC148" not in s
assert "P8" not in s
assert "I002" not in s
assert "ANL64" not in s
assert "549527552" not in s
assert "5347770372" not in s
print("PHASE2_CLOSED_SURFACE_V4_STATIC_QA=PASS")
