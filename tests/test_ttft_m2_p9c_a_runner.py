from pathlib import Path
import json, subprocess

ROOT=Path(__file__).resolve().parents[1]

def txt(p):
    return (ROOT/p).read_text(encoding="utf-8")

def blob(p):
    return subprocess.check_output(["git","-C",str(ROOT),"rev-parse",f"HEAD:{p}"],text=True).strip()

runner=txt("scripts/ttft_m2/run_p9_science.ps1")
binding=json.loads(txt("config/arcllm_ttft_m2_p9c_a_execution_binding_v0.1.json"))

# Exact canonical inputs remain unchanged.
assert blob("config/arcllm_ttft_m2_p9a_scientific_design_manifest_v0.1.json")=="abce0545cec33367f9b3a82f15d5ffd4fb026f64"
assert blob("config/arcllm_ttft_m2_p9b_implementation_lock_v0.2.json")=="7b06da63fe09597ebdcf641d1cd992d4a973075c"
assert blob("artifacts/TTFT_M2/TTFT_M2_P9B_BUILDONLY_ADJUDICATION_v0.1.json")=="d6a9947694cb35044251bac363236f69b06e4d9f"

# G1: active v0.2 lock only.
assert "arcllm_ttft_m2_p9b_implementation_lock_v0.2.json" in runner
assert "arcllm_ttft_m2_p9b_implementation_lock_v0.1.json" not in runner
assert "7b06da63fe09597ebdcf641d1cd992d4a973075c"==binding["p9b"]["implementation_lock_blob"]

# G2: exact qualified executable identity enforced before any launch.
assert binding["p9b"]["qualified_executable_sha256"]=="9E0C0A7CCCBB2DA767B4DE90354EC7ADF4463286264187843521419113C019C7"
assert binding["p9b"]["qualified_executable_bytes"]==441856
p_exe_hash=runner.index("$ExeHash=Sha256 $Exe")
p_exe_size=runner.index("F0 executable byte-size mismatch")
p_launch=runner.index("& $Exe --model")
assert p_exe_hash < p_exe_size < p_launch

# G3: both qualified manifest byte identities enforced.
assert binding["p9b"]["qualified_native_manifest_sha256"]=="0E51867BE03AFD60E90B823D2A6FE19F22ACBF6B628E9CE37E30E673C6AF91E4"
assert binding["p9b"]["qualified_shader_manifest_sha256"]=="9298D5EE2838B3FE5D39FC968DC937DC4183F168517CEE1F30F54E9CA86093FA"
assert "F0 native manifest SHA mismatch" in runner
assert "F0 shader manifest SHA mismatch" in runner

# G4: actual runtime SPIR-V files are rehashed before F1 and launch.
assert "F0 runtime SPIR-V mismatch" in runner
assert "F0 runtime Q4FAST SPIR-V mismatch" in runner
p_spv=runner.index("F0 runtime SPIR-V mismatch")
p_f1=runner.index("# F1")
assert p_spv < p_f1 < p_launch

# G5: canonical gate order F0 -> F1 -> F2-F7.
p_f0=runner.index("# F0")
p_f1=runner.index("# F1")
p_f2=runner.index("# F2")
assert p_f0 < p_f1 < p_f2 < p_launch

# Corrected-runner supersession is prospective and narrow.
assert 'if($P.Name -eq "scripts/ttft_m2/run_p9_science.ps1"){continue}' in runner
assert binding["p9b_lock_supersession_boundary"]["preserve_p9b_lock_immutable"] is True
assert binding["p9b_lock_supersession_boundary"]["skip_only_p9b_lock_member_during_runtime_git_reverification"]=="scripts/ttft_m2/run_p9_science.ps1"

# Still hard-gated; this step itself does not create execution authorization.
assert "arcllm_ttft_m2_p9c_execution_authorization_v0.1.json" in runner
assert "P9C execution authorization missing" in runner
assert "M2_P9C_FRESH_MECHANISM_IDENTIFICATION_EXECUTION_AUTHORIZED" in runner

# Frozen science design remains exact.
for x in ["SP/W-S","QP/W-S","QF/W-S","SF/W-S","SF/W-C","QF/W-C","QP/W-C","SP/W-C"]:
    assert x in runner
assert "--measured 5" in runner
assert "planned_observations=80" in runner
assert "parent_observations_reused=0" in runner
assert "selective_rerun=$false" in runner
assert "mechanism_adjudication_performed=$false" in runner

print("TTFT M2 P9C-A corrected runner static revalidation: PASS")
