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
