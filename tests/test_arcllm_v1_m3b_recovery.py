from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
r=(ROOT/"run_arcllm_v1_m3b.ps1").read_text(encoding="utf-8")
assert "$Lock.token_xray_contract.commit" not in r
assert "$Lock.token_xray_contract.contract_commit" in r
rec=(ROOT/"recover_arcllm_v1_m3b_bundle.ps1").read_text(encoding="utf-8")
for x in ["packaging_only=$true","collector_reexecuted=$false","counter_collection_reexecuted=$false","M3_B_RECOVERY=PASS","COLLECTION_HEAD","PACKAGING_HEAD","2814","M3B_COLLECTION_LOCK_v0.1.5.json"]:
    assert x in rec,x
assert "& $Exe" not in rec
assert "Start-Process" not in rec
assert "run_arcllm_v1_m3b.ps1" not in rec
print("ArcLLM v1 M3-B post-collection recovery static PASS")
