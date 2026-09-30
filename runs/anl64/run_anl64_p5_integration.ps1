param(
  [string]$ModelPath,
  [string]$OllamaModelsRoot
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Root=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
. (Join-Path $Root "runs\_relocation_compat.ps1")
$AuthPath=Join-Path $Root "config\anl64_p5_execution_authorization_v0.1.json"
$LockPath=Join-Path $Root "config\anl64_p5_execution_lock_v0.1.json"
$ResultsDir=Join-Path $Root "results\anl64_p5_integration"
$Bundle=Join-Path $Root "results\anl64_p5_integration_return_to_chatgpt.zip"

function Fail([string]$Message){throw "ANL64_P5_FAIL_CLOSED: $Message"}
function Require([bool]$Condition,[string]$Message){if(-not $Condition){Fail $Message}}
function GitBlob([string]$Path){
  return Get-RunLogicalGitBlob -RepoRoot $Root -Path $Path
}
function Sha256([string]$Path){
  Require (Test-Path $Path -PathType Leaf) "missing file: $Path"
  return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToUpperInvariant()
}

Write-Host "ANL64 P5 bounded semantic integration runner"
Write-Host "Performance fields emitted by locked runtimes are quarantined and have no P5/P6 decision role."

Require (Test-Path $AuthPath -PathType Leaf) "authorization missing"
Require (Test-Path $LockPath -PathType Leaf) "execution lock missing"
$Tracked=(& git -C $Root status --porcelain --untracked-files=no | Out-String)
Require ([string]::IsNullOrWhiteSpace($Tracked)) "tracked working tree is dirty"

$Auth=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
$Lock=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
Require ($Auth.decision -eq "P5_BOUNDED_SEMANTIC_INTEGRATION_EXECUTION_AUTHORIZED") "wrong authorization"
Require ([bool]$Auth.authorization.p5_target_model_load) "model load not authorized"
Require ([bool]$Auth.authorization.p5_gpu_dispatch) "GPU semantic execution not authorized"
Require (-not[bool]$Auth.authorization.p5_performance_adjudication) "performance adjudication must remain forbidden"
Require ($Lock.status -eq "P5_EXECUTION_LOCKED_NOT_YET_RUN") "execution lock status mismatch"

foreach($P in $Lock.exact_git_blobs.PSObject.Properties){
  Require ((GitBlob $P.Name) -eq [string]$P.Value) ("Git blob mismatch: "+$P.Name)
}
Require ((GitBlob "runs/anl64/run_anl64_p5_integration.ps1") -eq [string]$Lock.runner_git_blob) "runner self blob mismatch"

Require ((GitBlob "src/anl64_runtime.cpp") -eq "dbcb7afed5a08e7aff3ca02a1bd95bd985076f70") "ANL64 runtime drift"
Require ((GitBlob "src/anl64_plan.hpp") -eq "157be15c63363ba2d55093af829ca68be9107e27") "ANL64 plan drift"
Require ((GitBlob "src/q2_benchmark.cpp") -eq "ea1e986e22f6921e7f6c52a4fa5935121cfec663") "safe reference drift"
Require ((GitBlob "shaders/sa1_q4k_subgroup_splitk.comp") -eq "56999d88dc1bef6486e7e1908982f6de4b0f9f6a") "Q4_FAST source drift"
Require ((GitBlob "shaders/p7_q4k_gemm_2d.comp") -eq "fb1fb14192ff7d275a4af38c6dd9be7d1b500a7a") "Q4-safe source drift"

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  Require ($Resolved.Count -ge 1) "model resolver returned no path"
  $ModelPath=[string]$Resolved[-1]
}
Require (Test-Path $ModelPath -PathType Leaf) "model path missing"
$ModelFile=Get-Item $ModelPath
$ModelHash=Sha256 $ModelPath
Require ($ModelFile.Length -eq [int64]$Auth.exact_target.model_bytes) "model byte size mismatch"
Require ($ModelHash -eq ([string]$Auth.exact_target.model_sha256).ToUpperInvariant()) "model SHA256 mismatch"

$CPU=@(Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
$GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
Require (@($CPU|Where-Object {$_.Name -like ("*"+[string]$Auth.exact_target.cpu_substring+"*")}).Count -ge 1) "CPU mismatch"
$Arc=@($GPU|Where-Object {$_.Name -like ("*"+[string]$Auth.exact_target.gpu_substring+"*")})
Require ($Arc.Count -ge 1) "Arc 140V target not found"
Require (@($Arc|Where-Object {$_.DriverVersion -eq [string]$Auth.exact_target.driver}).Count -ge 1) "GPU driver mismatch"

$CandidateExe=Join-Path $Root ([string]$Auth.exact_candidate.executable_path)
$CandidateShaderDir=Join-Path $Root ([string]$Auth.exact_candidate.shader_dir)
Require ((Sha256 $CandidateExe) -eq ([string]$Auth.exact_candidate.executable_sha256).ToUpperInvariant()) "candidate executable SHA mismatch"
Require (Test-Path $CandidateShaderDir -PathType Container) "candidate shader directory missing"
Require ((Sha256 (Join-Path $CandidateShaderDir "anl64_q4_fast.spv")) -eq ([string]$Auth.exact_candidate.q4_fast_spv_sha256).ToUpperInvariant()) "candidate Q4_FAST SPIR-V mismatch"
Require ((Sha256 (Join-Path $CandidateShaderDir "p7_q4k_gemm_2d.spv")) -eq ([string]$Auth.exact_candidate.q4_safe_spv_sha256).ToUpperInvariant()) "candidate Q4-safe SPIR-V mismatch"
Require ((Sha256 (Join-Path $CandidateShaderDir "p7_q6k_gemm_2d.spv")) -eq ([string]$Auth.exact_candidate.q6_safe_spv_sha256).ToUpperInvariant()) "candidate Q6-safe SPIR-V mismatch"

Write-Host "P5 pre-science reference BuildOnly"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_q2_shaders.ps1")
Require ($LASTEXITCODE -eq 0) "safe reference shader compile failed"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_q2.ps1")
Require ($LASTEXITCODE -eq 0) "safe reference native build failed"
$ReferenceExe=Join-Path $Root "arcllm_q2.exe"
$ReferenceShaderDir=Join-Path $Root "compiled_shaders"
Require (Test-Path $ReferenceExe -PathType Leaf) "safe reference executable missing"
Require ((Sha256 (Join-Path $ReferenceShaderDir "p7_q4k_gemm_2d.spv")) -eq ([string]$Auth.exact_reference.q4_safe_spv_sha256).ToUpperInvariant()) "reference Q4-safe SPIR-V mismatch"
Require ((Sha256 (Join-Path $ReferenceShaderDir "p7_q6k_gemm_2d.spv")) -eq ([string]$Auth.exact_reference.q6_safe_spv_sha256).ToUpperInvariant()) "reference Q6-safe SPIR-V mismatch"
$ReferenceExeHash=Sha256 $ReferenceExe

if(Test-Path $ResultsDir){Remove-Item -Recurse -Force $ResultsDir}
if(Test-Path $Bundle){Remove-Item -Force $Bundle}
New-Item -ItemType Directory -Force -Path $ResultsDir|Out-Null

$Paths=[ordered]@{
  reference_ws=Join-Path $ResultsDir "reference_W_S.json"
  candidate_ws=Join-Path $ResultsDir "candidate_W_S.json"
  reference_wc=Join-Path $ResultsDir "reference_W_C.json"
  candidate_wc=Join-Path $ResultsDir "candidate_W_C.json"
}
$Exit=[ordered]@{}

function Invoke-Cell([string]$Key,[string]$Exe,[string]$ShaderDir,[string]$Implementation,[string]$Workload,[string]$OutPath){
  Write-Host ("P5 cell "+$Key)
  & $Exe --model $ModelPath --shader-dir $ShaderDir --implementation-commit $Implementation --workload $Workload --warmups 1 --measured 5 --out $OutPath
  $Code=$LASTEXITCODE
  if(-not(Test-Path $OutPath -PathType Leaf)){
    $FailObj=[ordered]@{schema="arcllm.anl64.p5.missing_cell.v0.1";status="MISSING_RESULT";key=$Key;process_exit_code=$Code}
    [IO.File]::WriteAllText($OutPath,($FailObj|ConvertTo-Json -Depth 5),(New-Object Text.UTF8Encoding($false)))
  }
  return $Code
}

$Exit.reference_ws=Invoke-Cell "ARC_SAFE_REFERENCE/W-S" $ReferenceExe $ReferenceShaderDir ([string]$Auth.exact_reference.historical_implementation_commit) "W-S" $Paths.reference_ws
$Exit.candidate_ws=Invoke-Cell "ANL64_P4_LOCKED/W-S" $CandidateExe $CandidateShaderDir ([string]$Auth.exact_candidate.implementation_identity) "W-S" $Paths.candidate_ws
$Exit.reference_wc=Invoke-Cell "ARC_SAFE_REFERENCE/W-C" $ReferenceExe $ReferenceShaderDir ([string]$Auth.exact_reference.historical_implementation_commit) "W-C" $Paths.reference_wc
$Exit.candidate_wc=Invoke-Cell "ANL64_P4_LOCKED/W-C" $CandidateExe $CandidateShaderDir ([string]$Auth.exact_candidate.implementation_identity) "W-C" $Paths.candidate_wc

$Semantic=Join-Path $ResultsDir "P5_SEMANTIC_EVIDENCE.json"
& py -3 (Join-Path $Root "tools\extract_anl64_p5_semantics.py") --reference-ws $Paths.reference_ws --candidate-ws $Paths.candidate_ws --reference-wc $Paths.reference_wc --candidate-wc $Paths.candidate_wc --out $Semantic
Require ($LASTEXITCODE -eq 0) "semantic extractor failed"
Require (Test-Path $Semantic -PathType Leaf) "semantic evidence missing"

$OS=Get-CimInstance Win32_OperatingSystem
$Env=[ordered]@{
  schema="arcllm.anl64.p5.execution_environment.v0.1"
  git_head=((& git -C $Root rev-parse HEAD).Trim())
  authorization_blob=(GitBlob "config/anl64_p5_execution_authorization_v0.1.json")
  execution_lock_blob=(GitBlob "config/anl64_p5_execution_lock_v0.1.json")
  runner_blob=(GitBlob "runs/anl64/run_anl64_p5_integration.ps1")
  model_path=$ModelPath
  model_sha256=$ModelHash
  model_bytes=$ModelFile.Length
  os=[ordered]@{caption=$OS.Caption;version=$OS.Version;build=$OS.BuildNumber}
  cpu=$CPU
  gpu=$GPU
  candidate_executable_sha256=Sha256 $CandidateExe
  reference_executable_sha256=$ReferenceExeHash
  cell_exit_codes=$Exit
  performance_adjudication=$false
  p5_timing_reusable_for_p6=$false
  p6_authorized=$false
}
$EnvPath=Join-Path $ResultsDir "P5_EXECUTION_ENVIRONMENT.json"
[IO.File]::WriteAllText($EnvPath,($Env|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

Copy-Item $AuthPath (Join-Path $ResultsDir "anl64_p5_execution_authorization_v0.1.json")
Copy-Item $LockPath (Join-Path $ResultsDir "anl64_p5_execution_lock_v0.1.json")
Copy-Item (Join-Path $Root "config\anl64_p5_integration_validation_spec_v0.1.json") (Join-Path $ResultsDir "anl64_p5_integration_validation_spec_v0.1.json")
Copy-Item (Join-Path $Root "artifacts\ANL64\ANL64_P5_SPECIFICATION_QA_v0.1.json") (Join-Path $ResultsDir "ANL64_P5_SPECIFICATION_QA_v0.1.json")
Copy-Item (Join-Path $Root "artifacts\ANL64\ANL64_P4_BUILDONLY_ADJUDICATION_v0.1.json") (Join-Path $ResultsDir "ANL64_P4_BUILDONLY_ADJUDICATION_v0.1.json")

Compress-Archive -Path (Join-Path $ResultsDir "*") -DestinationPath $Bundle -CompressionLevel Optimal
$BundleHash=Sha256 $Bundle

Write-Host ""
Write-Host "ANL64_P5_EXECUTION_COMPLETE_AWAITING_INDEPENDENT_ADJUDICATION"
Write-Host "PERFORMANCE_FIELDS_QUARANTINED"
Write-Host "P6_NOT_AUTHORIZED"
Write-Host ("bundle="+$Bundle)
Write-Host ("SHA256="+$BundleHash)
