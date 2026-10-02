from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
probe = (root / "baseline/r1r_tokenizer_probe.cpp").read_text(encoding="utf-8")
runner = (root / "tools/run_token_xray_r1r_one_shot.py").read_text(encoding="utf-8")
contract = json.loads((root / "config/token_xray_r1r_execution_contract_v0.1.json").read_text(encoding="utf-8"))

assert 'u8"Hà Nội là thủ đô của"' not in probe
assert "decode_hex" in probe
assert "--prompt" in probe
assert "prompt_source" in probe and "hex_decoded_bytes" in probe
assert "llama_tokenize" in probe

assert contract["canonical_execution_route"]["transport"] == "RemoteMCP managed task_job_submit"
assert contract["canonical_execution_route"]["executable"] == "python"
assert contract["canonical_execution_route"]["shell_wrapper"] is False
assert contract["canonical_execution_route"]["alternate_route_after_lock"] is False
assert contract["execution_lock"]["canonical_namespace"] == "results/token_xray_r1r_one_shot_lock_v0.1"
assert contract["execution_lock"]["rerun_permitted"] is False

p2 = contract["prompts"]["P2"]
assert bytes.fromhex(p2["utf8_hex"]).decode("utf-8") == "Hà Nội là thủ đô của"
assert p2["token_ids"] == [39,6362,128685,37915,128799,130287,59735]

assert 'choices=["route-preflight", "science"]' in runner
assert "STOP_BEFORE_MODEL_EXECUTION: exact-GGUF tokenization mismatch" in runner
assert "model_execution_started" in runner
assert "pairs_consumed" in runner
assert "fresh namespace already exists; rerun forbidden" in runner
assert 'subprocess.run(argv' in runner
assert 'shell=True' not in runner
assert 'powershell' not in runner.lower()
assert 'cmd.exe' not in runner.lower()
assert "job_object_status" in runner
assert "memory_limit_flags" in runner

# Explicit false boundary fields are required and are not semantic/mechanistic claims.
assert '"semantic_interpretation": False' in runner
assert '"causal_mechanism": False' in runner
assert '"performance_authority": False' in runner

print("TOKEN_XRAY_R1R_STATIC_PREFLIGHT=PASS")
