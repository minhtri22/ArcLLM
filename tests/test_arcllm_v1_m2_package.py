from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=(ROOT/"tools/parse_arcllm_v1_m2_llama_perf.py").read_text(encoding="utf-8")
for x in ["GGML_VK_PERF_LOGGER=1","ffn_gate_up_q4_k","ffn_down_q4_k","ffn_down_q6_k","lm_head_q6_k","q_o_q4_k_combined","k_v_q4_k_combined","v_q6_k","[0,15,30]"]: assert x in p,x
r=(ROOT/"run_arcllm_v1_m2.ps1").read_text(encoding="utf-8")
for x in ["GGML_VK_PERF_LOGGER","GGML_VK_PERF_LOGGER_FREQUENCY","i003_llama_adapter.exe","1a31529a4fa40742","d9b495aa8e8764e4","QUIET_HOST_REQUIRED=false"]: assert x in r,x
assert "GGML_VK_PERF_LOGGER_CONCURRENT" in r
print("ArcLLM v1 M2 static package PASS")

# Regression: Windows PowerShell must not invoke llama directly with native stderr redirection under ErrorActionPreference=Stop.
assert "Start-Process" in r
assert "Invoke-NativeCaptured" in r
assert "M2_STDERR_PROBE" in r and "M2_STDOUT_PROBE" in r
assert "-RedirectStandardError" in r and "-RedirectStandardOutput" in r
assert "& $Exe --model" not in r
assert "Vulkan Timings:" in r
print("ArcLLM v1 M2 Windows native-stderr regression PASS")

# Synthetic parser regression: two logger names may map to one semantic family and must aggregate.
import json, subprocess, sys, tempfile
from pathlib import Path as _P
def _m2_block():
    return "\n".join([
        "Vulkan Timings:",
        "MUL_MAT_VEC q4_K m=18944 n=1 k=3584: 28 x 1.0 us = 28.0 us",
        "FUSED_SILU MUL_MAT_VEC q4_K m=18944 n=1 k=3584: 28 x 1.0 us = 28.0 us",
        "MUL_MAT_VEC q4_K m=3584 n=1 k=18944: 14 x 1.0 us = 14.0 us",
        "MUL_MAT_VEC q6_K m=3584 n=1 k=18944: 14 x 1.0 us = 14.0 us",
        "MUL_MAT_VEC q6_K m=152064 n=1 k=3584: 1 x 1.0 us = 1.0 us",
        "MUL_MAT_VEC q4_K m=3584 n=1 k=3584: 56 x 1.0 us = 56.0 us",
        "MUL_MAT_VEC q4_K m=512 n=1 k=3584: 42 x 1.0 us = 42.0 us",
        "MUL_MAT_VEC q6_K m=512 n=1 k=3584: 14 x 1.0 us = 14.0 us",
        "Total time: 197.0 us."
    ])
with tempfile.TemporaryDirectory() as td:
    td=_P(td)
    payload="\n".join([_m2_block() for _ in range(64)])+"\n"
    ws=td/"ws.txt"; wc=td/"wc.txt"; out=td/"out.json"
    ws.write_text(payload,encoding="utf-8"); wc.write_text(payload,encoding="utf-8")
    cp=subprocess.run([sys.executable,str(ROOT/"tools/parse_arcllm_v1_m2_llama_perf.py"),"--ws-log",str(ws),"--wc-log",str(wc),"--out",str(out)],capture_output=True,text=True)
    assert cp.returncode==0,(cp.stdout,cp.stderr)
    obj=json.loads(out.read_text(encoding="utf-8"))
    assert obj["status"]=="PASS",obj
    assert obj["global_family_metrics"]["ffn_gate_up_q4_k"]["expected_call_count"]==56
print("ArcLLM v1 M2 synthetic parser aggregation PASS")
