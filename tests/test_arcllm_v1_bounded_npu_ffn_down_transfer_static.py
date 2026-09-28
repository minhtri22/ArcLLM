from pathlib import Path
import json,re

root=Path(__file__).resolve().parents[1]
p=json.loads((root/"config/arcllm_v1_bounded_npu_ffn_down_transfer_prereg_v0.1.json").read_text())
s=(root/"tools/run_arcllm_v1_bounded_npu_ffn_down_transfer.py").read_text()
assert p["status"]=="FROZEN_DESIGN_EXECUTION_NOT_YET_RUN"
assert p["structural_outcome_blind_layer_selection"]["Q4_K_selected"]==[3,14,22]
assert p["structural_outcome_blind_layer_selection"]["Q6_K_selected"]==[0,16,27]
for tok in [
    'SELECTED = [','"layer":3','"layer":14','"layer":22','"layer":0','"layer":16','"layer":27',
    'MAX_ABS_MAX = 0.02','RMSE_MAX = 0.005','WARMUPS = 3','REPEATS = 9',
    'gguf.dequantize','compile_model(model,"NPU"','io.BytesIO()','compiled.export_model(stream)','import_model(stream,"NPU")',
    '28.0*p95','0.90*min(GPU_BUDGET.values())'
]:
    assert tok in s,tok
for forbidden in [
    'arcllm_v1_runtime.exe','run_arcllm','llama.cpp','Gate/Up','lm_head','attn_qkv'
]:
    assert forbidden not in s,forbidden
print("ARCLLM_V1_BOUNDED_NPU_FFN_DOWN_TRANSFER_STATIC_QA=PASS")
