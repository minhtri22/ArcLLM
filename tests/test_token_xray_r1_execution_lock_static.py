import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
contract = json.loads((ROOT / "config/token_xray_r1_execution_contract_v0.1.json").read_text(encoding="utf-8"))
runner = (ROOT / "tools/run_token_xray_r1_one_shot.ps1").read_text(encoding="utf-8")
harness = (ROOT / "src/token_xray_r1_capture_harness.cpp").read_text(encoding="utf-8")

EXPECTED_MODEL_SHA = "60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
EXPECTED_PARENT = "c6c9b2e0a198b8af43b120e2b269e2ddd0429ef1"
EXPECTED_PROMPTS = {
    "P0": [785, 6722, 315, 9625, 374],
    "P1": [17, 488, 220, 17, 284],
    "P2": [39, 6362, 128685, 37915, 128799, 130287, 59735],
    "P3": [32, 425, 356, 422, 468, 434, 479, 472],
}
EXPECTED_PAIRS = [
    (1, "P0", ["baseline", "instrumented"]),
    (2, "P1", ["instrumented", "baseline"]),
    (3, "P2", ["baseline", "instrumented"]),
    (4, "P3", ["instrumented", "baseline"]),
    (5, "P0", ["instrumented", "baseline"]),
    (6, "P1", ["baseline", "instrumented"]),
    (7, "P2", ["instrumented", "baseline"]),
    (8, "P3", ["baseline", "instrumented"]),
]


def test_exact_model_and_parent_are_frozen():
    assert contract["arcllm_parent"] == EXPECTED_PARENT
    assert contract["model"]["sha256"] == EXPECTED_MODEL_SHA
    assert contract["model"]["size_bytes"] == 4683074048
    assert contract["model"]["layers"] == 28
    assert contract["model"]["hidden"] == 3584


def test_p0_p3_token_ids_are_frozen():
    assert set(contract["prompts"]) == set(EXPECTED_PROMPTS)
    for pid, ids in EXPECTED_PROMPTS.items():
        assert contract["prompts"][pid]["token_ids"] == ids


def test_exact_eight_pair_schedule_is_frozen_and_balanced():
    got = [
        (int(x["pair"]), x["prompt"], list(x["order"]))
        for x in contract["matched_pairs"]
    ]
    assert got == EXPECTED_PAIRS
    assert len(got) == 8
    for pid in EXPECTED_PROMPTS:
        rows = [x for x in got if x[1] == pid]
        assert len(rows) == 2
        first_members = [x[2][0] for x in rows]
        assert sorted(first_members) == ["baseline", "instrumented"]


def test_one_shot_state_is_created_before_any_harness_invocation():
    guard = 'if(Test-Path $StatePath){throw "STOP: R1 one-shot state already exists; rerun forbidden"}'
    save = "Save-State"
    invoke = "& $Harness --model $ModelPath"
    assert guard in runner
    assert invoke in runner
    guard_i = runner.index(guard)
    first_save_i = runner.index(save, guard_i)
    invoke_i = runner.index(invoke)
    assert guard_i < first_save_i < invoke_i


def test_identity_and_exact_gguf_tokenization_gate_precede_model_execution():
    sha_gate = 'if($ObservedSha-ne([string]$Contract.model.sha256).ToUpperInvariant()){throw "STOP: model SHA256 mismatch"}'
    gguf_probe = '& (Join-Path $Root "token_xray_r1_tokenizer_probe.exe") --model $ModelPath --out $ExactTokPath'
    token_compare = 'if([uint32]$Expected[$i]-ne[uint32]$Observed[$i]){throw "STOP: exact-GGUF token ID mismatch for $Pid index $i"}'
    invoke = "& $Harness --model $ModelPath"
    assert runner.index(sha_gate) < runner.index(gguf_probe) < runner.index(token_compare) < runner.index(invoke)


def test_repo_heads_are_bound_to_frozen_refs_not_operator_arguments():
    lock = contract["execution_lock"]
    assert lock["arcllm_freeze_ref"] == "freeze/token-xray-r1-real-runtime-exec-v0.1"
    assert lock["token_xray_freeze_ref"] == "freeze/representation-trajectory-r1-validator-v0.1"
    assert lock["token_xray_head"] == "2e1680fa9187b166a8fb6c52a5c8677a86766c39"
    assert "ExpectedArcLLMHead" not in runner
    assert "[Parameter(Mandatory=$true)][string]$ExpectedTokenXRayHead" not in runner
    assert 'if($ArcHead-ne$ArcFreezeHead){throw "STOP: ArcLLM HEAD is not the frozen R1 execution ref"}' in runner
    assert 'if($TxHead-ne$ExpectedTokenXRayHead){throw "STOP: Token-XRay HEAD mismatch"}' in runner
    assert 'if($TxFreezeHead-ne$ExpectedTokenXRayHead){throw "STOP: Token-XRay freeze ref mismatch"}' in runner


def test_runtime_capture_is_not_performance_authority():
    assert contract["performance_authority"] is False
    assert contract["capture"]["observer_dispatches_excluded_from_canonical_model_dispatch_stats"] is True
    assert "request_elapsed_ns" in harness
    assert "performance" not in harness.lower()


def test_namespace_and_rerun_are_frozen():
    lock = contract["execution_lock"]
    assert lock["canonical_namespace"] == "results/token_xray_r1_one_shot_lock_v0.1"
    assert lock["out_dir_override_permitted"] is False
    assert lock["rerun_permitted"] is False
    assert 'if($ResolvedOut-ne$ResolvedCanonical){throw "STOP: R1 output namespace is frozen; override forbidden"}' in runner
    assert 'if(Test-Path $StatePath){throw "STOP: R1 one-shot state already exists; rerun forbidden"}' in runner


def test_executable_and_shader_hashes_are_materialized_before_first_generate_call():
    for token in [
        "$ContractSha=(Get-FileHash $ContractPath -Algorithm SHA256).Hash.ToUpperInvariant()",
        "$RunnerSha=(Get-FileHash $RunnerPath -Algorithm SHA256).Hash.ToUpperInvariant()",
        "$HarnessSha=(Get-FileHash $Harness -Algorithm SHA256).Hash.ToUpperInvariant()",
        "$TokenizerSha=(Get-FileHash $TokenizerExe -Algorithm SHA256).Hash.ToUpperInvariant()",
        "$CaptureShaderSha=(Get-FileHash $CaptureShader -Algorithm SHA256).Hash.ToUpperInvariant()",
        "compiled_shaders=$ShaderRows",
        "function Assert-ExecutionLock()",
        "Assert-ExecutionLock",
    ]:
        assert token in runner
    state_i = runner.index('status="PREFLIGHT_COMPLETE_NOT_STARTED"')
    lock_call_i = runner.index("Assert-ExecutionLock", runner.index('$RawCandidate='))
    invoke_i = runner.index("& $Harness --model $ModelPath")
    assert state_i < lock_call_i < invoke_i


def test_execution_contract_freezes_exact_16_member_schedule():
    lock = contract["execution_lock"]
    assert lock["matched_pair_count"] == 8
    assert lock["request_member_count"] == 16
    assert lock["execution_order_frozen"] is True
