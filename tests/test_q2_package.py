from pathlib import Path
import hashlib, json, re

ROOT=Path(__file__).resolve().parents[1]
def txt(p): return (ROOT/p).read_text(encoding="utf-8")

required=[
 "docs/Q2_MATCHED_BENCHMARK_CONTRACT.md","docs/Q2_IMPLEMENTATION.md",
 "config/q2_workloads.json","src/q2_benchmark.cpp",
 "baseline/q2_llama_adapter.cpp","baseline/CMakeLists.txt",
 "tools/q2_resource_sampler.py","tools/q2_gpu_sampler.ps1","tools/summarize_q2.py",
 "tools/compile_q2_shaders.ps1","tools/build_q2.ps1","tools/qualify_q2_baseline.ps1",
 "run_q2.ps1","manifest.json","inputs/q1_return_to_chatgpt.authoritative.zip",
]
for p in required: assert (ROOT/p).is_file(),p

assert hashlib.sha256((ROOT/"inputs/q1_return_to_chatgpt.authoritative.zip").read_bytes()).hexdigest().upper()=="DFB3E86C4F51D06290A2AE1ED1479C96F35AE0D329CFA5E2B7D50289DC641B43"

contract=txt("docs/Q2_MATCHED_BENCHMARK_CONTRACT.md")
assert "Status: DESIGN_FROZEN" in contract
assert "391fac16460f15233a7740550d858ac96df3419d" in contract and "v0.4.1" in contract
assert "Q2 must not define a winner." in contract
assert "1 complete warmup inference" in contract and "5 measured inference attempts" in contract
assert "W-S" in contract and "W-C" in contract

cfg=json.loads(txt("config/q2_workloads.json"))
ws=cfg["W_S"]["token_ids"];wc=cfg["W_C"]["token_ids"]
assert ws==[1,133151,133152,152062]
assert len(wc)==256 and wc[:4]==ws
assert all(wc[i]==1+((104729+7919*i)%152063) for i in range(4,256))
assert all(0<=x<152064 for x in ws+wc)
assert cfg["W_S"]["output_tokens"]==32 and cfg["W_C"]["output_tokens"]==32

src=txt("src/q2_benchmark.cpp")
main=src[src.index("int main("):]
for x in [
 "MAXSEQ=256","input_ids={1u,133151u,133152u,152062u}",
 "1u+((104729u+7919u*i)%152063u)","auto build_prefill=[&](uint32_t seq)",
 "EXPECT_PREFILL=441","EXPECT_DECODE=469","di<31u","a.generated.size()==32u",
 'Q2_"<<kind<<"_BEGIN|ArcLLM|',"cpu_full_logits_read_bytes_per_inference",
 '"advantage_claimed\\":false'
]: assert x in src,x
assert not re.search(r"\bSEQ\b",main)
assert main.count("for(uint32_t l=0;l<LAYERS;++l)")>=3
assert "matmul_q4_cpu(" not in main and "matmul_q6_cpu(" not in main
assert "cpu_model_math_fallback\\\":false" in main
assert "cpu_teacher_forcing\\\":false" in main
assert "for(int i=0;i<5;++i)" in main
assert "run_attempt(0,true)" in main
assert "31.0/(a.decode_ms/1000.0)" in main
assert "p8q1_lmhead_q6k_segmented_chunk.spv" in main
assert "p8c_embedding_q4k_segmented_probe.spv" in main

base=txt("baseline/q2_llama_adapter.cpp")
for x in [
 "391fac16460f15233a7740550d858ac96df3419d","v0.4.1",
 "llama_batch_get_one(in.data()","mp.n_gpu_layers=-1",
 "cp.n_ctx=4096","cp.n_batch=256","cp.n_ubatch=256",
 "cp.n_threads=8","cp.n_threads_batch=8","cp.type_k=GGML_TYPE_F32","cp.type_v=GGML_TYPE_F32",
 "llama_sampler_init_greedy","llama_memory_clear","di<31","i<5",
 '"raw_token_input\\":true','"tokenizer_used\\":false','"eos_early_stop\\":false',
 '"speculative_decoding\\":false','"advantage_claimed\\":false'
]: assert x in base,x
assert "llama_tokenize(" not in base
assert "llama_vocab_is_eog" not in base
assert "common_sampler" not in base

cmake=txt("baseline/CMakeLists.txt")
assert "BUILD_SHARED_LIBS OFF" in cmake
assert "GGML_VULKAN ON" in cmake
assert "GGML_BACKEND_DL OFF" in cmake
assert "target_link_libraries(q2_llama_adapter PRIVATE llama)" in cmake

qual=txt("tools/qualify_q2_baseline.ps1")
assert 'PinnedCommit="391fac16460f15233a7740550d858ac96df3419d"' in qual
assert 'PinnedRelease="v0.4.1"' in qual
assert "TagCommit -ne $PinnedCommit" in qual
assert "source tree is dirty" in qual
assert "VULKAN_SDK" in qual and "glslc.exe" in qual
assert "target_model_executed=$false" in qual
assert 'qualification="BUILD_API_QUALIFIED"' in qual

sampler=txt("tools/q2_resource_sampler.py")
assert "GetProcessMemoryInfo" in sampler and "GetProcessTimes" in sampler
assert 'default=100' in sampler
assert 'for idx in range(5)' in sampler
assert '"attempted":True' in sampler and '"available":bool(gpu_samples)' in sampler
assert "raw_process_samples" in sampler and "raw_gpu_samples" in sampler
gpu=txt("tools/q2_gpu_sampler.ps1")
assert "GPU Engine" in gpu and "GPU Process Memory" in gpu
assert "SampleMilliseconds=100" in gpu

summ=txt("tools/summarize_q2.py")
for x in ['"median"','"min"','"max"','"mad"','descriptive_only_no_winner','"winner_declared":False','"expected_measured_attempts":20']:
    assert x in summ,x
assert "p95" not in summ.lower()
assert "Q2_EVIDENCE_READY_FOR_ADJUDICATION" in summ
assert '"q3_started":False' in summ

compile_ps=txt("tools/compile_q2_shaders.ps1")
assert "arcllm.q2.arcllm_shader_provenance.v1" in compile_ps
assert "-ne 16" in compile_ps
build_ps=txt("tools/build_q2.ps1")
assert "q2_benchmark.cpp" in build_ps and "arcllm_q2.exe" in build_ps

runner=txt("run_q2.ps1")
for h in [
 "DFB3E86C4F51D06290A2AE1ED1479C96F35AE0D329CFA5E2B7D50289DC641B43",
 "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463",
 "391fac16460f15233a7740550d858ac96df3419d","32.0.101.8860"
]: assert h in runner,h
order=[runner.index("$CellExit.arcllm_ws"),runner.index("$CellExit.baseline_ws"),runner.index("$CellExit.baseline_wc"),runner.index("$CellExit.arcllm_wc")]
assert order==sorted(order)
assert '"--warmups","1","--measured","5"' in runner
assert "q2_resource_sampler.py" in runner and "summarize_q2.py" in runner
assert "q2_return_to_chatgpt.zip" in runner
assert "q3_started=$false" in runner

m=json.loads(txt("manifest.json"))
assert m["phase"]=="Q2-MATCHED-BENCHMARK"
assert m["q1"]["verdict"]=="Q1_FEASIBILITY_ESTABLISHED"
assert m["q2"]["baseline"]["commit"]=="391fac16460f15233a7740550d858ac96df3419d"
assert m["q2"]["warmups_per_cell"]==1 and m["q2"]["measured_attempts_per_cell"]==5
assert m["q2"]["total_measured_attempts"]==20
assert m["q2"]["advantage_claim_permitted"] is False
assert m["q3_permitted"] is False
assert m["target_run_permitted"] is False
assert m["q2"]["target_run_permitted"] is False

print("ArcLLM Q2 static package: PASS")
