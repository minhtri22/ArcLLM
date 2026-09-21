param(
  [switch]$InternalSession,
  [ValidateSet("A","B")]
  [string]$Session,
  [string]$ModelPath,
  [string]$OllamaModelsRoot
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$AuthPath=Join-Path $Root "config\anl64_p6_execution_authorization_v0.1.json"
$LockPath=Join-Path $Root "config\anl64_p6_execution_lock_v0.1.json"
$ResultsRoot=Join-Path $Root "results\anl64_p6_confirmatory"
$Bundle=Join-Path $Root "results\anl64_p6_confirmatory_return_to_chatgpt.zip"
$CandidateExe=Join-Path $Root "artifacts\ANL64\P4\anl64_p4.exe"
$CandidateShaderDir=Join-Path $Root "artifacts\ANL64\P4\compiled_shaders"
$ReferenceExe=Join-Path $Root "arcllm_q2.exe"
$ReferenceShaderDir=Join-Path $Root "compiled_shaders"

function Fail([string]$Message){throw "ANL64_P6_FAIL_CLOSED: $Message"}
function Require([bool]$Condition,[string]$Message){if(-not $Condition){Fail $Message}}
function GitBlob([string]$Path){
  $p=$Path -replace '\\','/'
  $v=(& git -C $Root rev-parse ("HEAD:"+$p) 2>$null)
  if($LASTEXITCODE -ne 0){Fail "cannot resolve Git blob for $Path"}
  return $v.Trim()
}
function Sha256([string]$Path){
  Require (Test-Path $Path -PathType Leaf) "missing file: $Path"
  return (Get-FileHash -Algorithm SHA256 $Path).Hash.ToUpperInvariant()
}
function Get-PowerState(){
  $PowerScheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()
  $PowerType=@"
using System;
using System.Runtime.InteropServices;
public static class Anl64P6Power {
  [StructLayout(LayoutKind.Sequential)]
  public struct SYSTEM_POWER_STATUS {
    public byte ACLineStatus; public byte BatteryFlag; public byte BatteryLifePercent; public byte SystemStatusFlag;
    public uint BatteryLifeTime; public uint BatteryFullLifeTime;
  }
  [DllImport("kernel32.dll", SetLastError=true)]
  [return: MarshalAs(UnmanagedType.Bool)]
  public static extern bool GetSystemPowerStatus(out SYSTEM_POWER_STATUS s);
}
"@
  if(-not ("Anl64P6Power" -as [type])){Add-Type -TypeDefinition $PowerType}
  $S=New-Object "Anl64P6Power+SYSTEM_POWER_STATUS"
  Require ([Anl64P6Power]::GetSystemPowerStatus([ref]$S)) "cannot query AC state"
  return [ordered]@{scheme=$PowerScheme;ac_line_status=[int]$S.ACLineStatus}
}
function Verify-Frozen(){
  Require (Test-Path $AuthPath -PathType Leaf) "authorization missing"
  Require (Test-Path $LockPath -PathType Leaf) "execution lock missing"
  $Tracked=(& git -C $Root status --porcelain --untracked-files=no | Out-String)
  Require ([string]::IsNullOrWhiteSpace($Tracked)) "tracked working tree is dirty"
  $Auth=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
  $Lock=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
  Require ($Auth.decision -eq "P6_MATCHED_E2E_EXECUTION_AUTHORIZED") "wrong execution authorization"
  Require ([bool]$Auth.authorization.performance_measurement) "performance measurement not authorized"
  Require (-not[bool]$Auth.authorization.production_source_mutation) "production mutation must be forbidden"
  Require ($Lock.status -eq "P6_EXECUTION_LOCKED_NOT_YET_RUN") "execution lock status mismatch"
  foreach($P in $Lock.exact_git_blobs.PSObject.Properties){
    Require ((GitBlob $P.Name) -eq [string]$P.Value) ("Git blob mismatch: "+$P.Name)
  }
  Require ((GitBlob "run_anl64_p6_confirmatory.ps1") -eq [string]$Lock.runner_git_blob) "runner self blob mismatch"
  return [ordered]@{auth=$Auth;lock=$Lock}
}
function Resolve-Target($Auth){
  if(-not $script:ModelPath){
    $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $script:OllamaModelsRoot)
    Require ($Resolved.Count -ge 1) "model resolver returned no path"
    $script:ModelPath=[string]$Resolved[-1]
  }
  Require (Test-Path $script:ModelPath -PathType Leaf) "model path missing"
  $MF=Get-Item $script:ModelPath
  $MH=Sha256 $script:ModelPath
  Require ($MF.Length -eq [int64]$Auth.environment.model_bytes) "model bytes mismatch"
  Require ($MH -eq ([string]$Auth.environment.model_sha256).ToUpperInvariant()) "model SHA256 mismatch"
  $OS=Get-CimInstance Win32_OperatingSystem
  $CPU=@(Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
  $GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
  Require ([string]$OS.BuildNumber -eq [string]$Auth.environment.os_build) "OS build mismatch"
  Require (@($CPU|Where-Object {$_.Name -like ("*"+[string]$Auth.environment.cpu_substring+"*")}).Count -ge 1) "CPU mismatch"
  $Arc=@($GPU|Where-Object {$_.Name -like ("*"+[string]$Auth.environment.gpu_substring+"*")})
  Require ($Arc.Count -ge 1) "GPU mismatch"
  Require (@($Arc|Where-Object {$_.DriverVersion -eq [string]$Auth.environment.driver}).Count -ge 1) "GPU driver mismatch"
  $Power=Get-PowerState
  Require ($Power.ac_line_status -eq 1) "AC power required"
  Require ($Power.scheme -like ("*"+[string]$Auth.environment.active_power_scheme_contains+"*")) "power scheme mismatch"
  return [ordered]@{model_sha256=$MH;model_bytes=$MF.Length;os=$OS;cpu=$CPU;gpu=$GPU;power=$Power}
}
function Verify-Artifacts($Auth){
  Require ((Sha256 $CandidateExe) -eq ([string]$Auth.exact_candidate.executable_sha256).ToUpperInvariant()) "candidate executable SHA mismatch"
  Require (Test-Path $CandidateShaderDir -PathType Container) "candidate shader dir missing"
  Require ((Sha256 (Join-Path $CandidateShaderDir "anl64_q4_fast.spv")) -eq ([string]$Auth.exact_candidate.q4_fast_spv_sha256).ToUpperInvariant()) "candidate Q4_FAST SPV mismatch"
  Require ((Sha256 (Join-Path $CandidateShaderDir "p7_q4k_gemm_2d.spv")) -eq ([string]$Auth.exact_candidate.q4_safe_spv_sha256).ToUpperInvariant()) "candidate Q4-safe SPV mismatch"
  Require ((Sha256 (Join-Path $CandidateShaderDir "p7_q6k_gemm_2d.spv")) -eq ([string]$Auth.exact_candidate.q6_safe_spv_sha256).ToUpperInvariant()) "candidate Q6-safe SPV mismatch"
  Require (Test-Path $ReferenceExe -PathType Leaf) "reference executable missing"
  Require (Test-Path $ReferenceShaderDir -PathType Container) "reference shader dir missing"
  Require ((Sha256 (Join-Path $ReferenceShaderDir "p7_q4k_gemm_2d.spv")) -eq ([string]$Auth.exact_reference.q4_safe_spv_sha256).ToUpperInvariant()) "reference Q4-safe SPV mismatch"
  Require ((Sha256 (Join-Path $ReferenceShaderDir "p7_q6k_gemm_2d.spv")) -eq ([string]$Auth.exact_reference.q6_safe_spv_sha256).ToUpperInvariant()) "reference Q6-safe SPV mismatch"
}
function Invoke-Cell([string]$Key,[string]$Exe,[string]$ShaderDir,[string]$Implementation,[string]$Workload,[string]$OutPath){
  Write-Host ("P6 cell "+$Key)
  & $Exe --model $script:ModelPath --shader-dir $ShaderDir --implementation-commit $Implementation --workload $Workload --warmups 1 --measured 5 --out $OutPath | Out-Host
  $Code=[int]$LASTEXITCODE
  if(-not(Test-Path $OutPath -PathType Leaf)){
    $FailObj=[ordered]@{schema="arcllm.anl64.p6.missing_cell.v0.1";status="MISSING_RESULT";key=$Key;process_exit_code=$Code}
    [IO.File]::WriteAllText($OutPath,($FailObj|ConvertTo-Json -Depth 5),(New-Object Text.UTF8Encoding($false)))
  }
  return $Code
}

$Frozen=Verify-Frozen
$Auth=$Frozen.auth
$Target=Resolve-Target $Auth

if($InternalSession){
  Require ($Session -in @("A","B")) "internal session must be A or B"
  Require (Test-Path $ResultsRoot -PathType Container) "results root missing"
  $PreflightPath=Join-Path $ResultsRoot "P6_PREFLIGHT.json"
  Require (Test-Path $PreflightPath -PathType Leaf) "preflight artifact missing"
  $Preflight=Get-Content $PreflightPath -Raw -Encoding UTF8|ConvertFrom-Json
  Verify-Artifacts $Auth
  Require ((Sha256 $ReferenceExe) -eq ([string]$Preflight.reference_executable_sha256).ToUpperInvariant()) "reference executable changed after preflight"
  $SessionDir=Join-Path $ResultsRoot ("session_"+$Session)
  Require (-not(Test-Path $SessionDir)) "session directory already exists; selective rerun forbidden"
  New-Item -ItemType Directory -Path $SessionDir|Out-Null

  $Env=[ordered]@{
    schema="arcllm.anl64.p6.session_environment.v0.1"
    session=$Session
    runner_process_id=$PID
    git_head=((& git -C $Root rev-parse HEAD).Trim())
    model_sha256=$Target.model_sha256
    model_bytes=$Target.model_bytes
    os=[ordered]@{caption=$Target.os.Caption;version=$Target.os.Version;build=$Target.os.BuildNumber}
    cpu=$Target.cpu
    gpu=$Target.gpu
    power=$Target.power
    candidate_executable_sha256=Sha256 $CandidateExe
    reference_executable_sha256=Sha256 $ReferenceExe
  }
  $EnvPath=Join-Path $SessionDir "P6_SESSION_ENVIRONMENT.json"
  [IO.File]::WriteAllText($EnvPath,($Env|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

  $Paths=[ordered]@{
    reference_ws=Join-Path $SessionDir "reference_W_S.json"
    candidate_ws=Join-Path $SessionDir "candidate_W_S.json"
    reference_wc=Join-Path $SessionDir "reference_W_C.json"
    candidate_wc=Join-Path $SessionDir "candidate_W_C.json"
  }
  $Exit=[ordered]@{}
  if($Session -eq "A"){
    $Order=@("ARC_SAFE_REFERENCE/W-S","ANL64_P4_LOCKED/W-S","ANL64_P4_LOCKED/W-C","ARC_SAFE_REFERENCE/W-C")
    $Exit.reference_ws=Invoke-Cell "ARC_SAFE_REFERENCE/W-S" $ReferenceExe $ReferenceShaderDir ([string]$Auth.exact_reference.historical_implementation_commit) "W-S" $Paths.reference_ws
    $Exit.candidate_ws=Invoke-Cell "ANL64_P4_LOCKED/W-S" $CandidateExe $CandidateShaderDir ([string]$Auth.exact_candidate.implementation_identity) "W-S" $Paths.candidate_ws
    $Exit.candidate_wc=Invoke-Cell "ANL64_P4_LOCKED/W-C" $CandidateExe $CandidateShaderDir ([string]$Auth.exact_candidate.implementation_identity) "W-C" $Paths.candidate_wc
    $Exit.reference_wc=Invoke-Cell "ARC_SAFE_REFERENCE/W-C" $ReferenceExe $ReferenceShaderDir ([string]$Auth.exact_reference.historical_implementation_commit) "W-C" $Paths.reference_wc
  }else{
    $Order=@("ANL64_P4_LOCKED/W-S","ARC_SAFE_REFERENCE/W-S","ARC_SAFE_REFERENCE/W-C","ANL64_P4_LOCKED/W-C")
    $Exit.candidate_ws=Invoke-Cell "ANL64_P4_LOCKED/W-S" $CandidateExe $CandidateShaderDir ([string]$Auth.exact_candidate.implementation_identity) "W-S" $Paths.candidate_ws
    $Exit.reference_ws=Invoke-Cell "ARC_SAFE_REFERENCE/W-S" $ReferenceExe $ReferenceShaderDir ([string]$Auth.exact_reference.historical_implementation_commit) "W-S" $Paths.reference_ws
    $Exit.reference_wc=Invoke-Cell "ARC_SAFE_REFERENCE/W-C" $ReferenceExe $ReferenceShaderDir ([string]$Auth.exact_reference.historical_implementation_commit) "W-C" $Paths.reference_wc
    $Exit.candidate_wc=Invoke-Cell "ANL64_P4_LOCKED/W-C" $CandidateExe $CandidateShaderDir ([string]$Auth.exact_candidate.implementation_identity) "W-C" $Paths.candidate_wc
  }

  $RawSha=[ordered]@{}
  foreach($K in $Paths.Keys){$RawSha[$K]=Sha256 $Paths[$K]}
  $Meta=[ordered]@{
    schema="arcllm.anl64.p6.session_meta.v0.1"
    session=$Session
    runner_process_id=$PID
    execution_order=$Order
    cell_exit_codes=$Exit
    warmups_per_cell=1
    measured_attempts_per_cell=5
    measured_attempts_expected=20
    raw_sha256=$RawSha
  }
  $MetaPath=Join-Path $SessionDir "P6_SESSION_META.json"
  [IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
  Write-Host ("ANL64_P6_SESSION_"+$Session+"_COMPLETE")
  exit 0
}

Write-Host "ANL64 P6 fresh matched E2E confirmatory runner"
Write-Host "P5 timings are forbidden and are not read."

Require (-not(Test-Path $ResultsRoot)) "P6 results root already exists; rerun/salvage forbidden"
Require (-not(Test-Path $Bundle)) "P6 bundle already exists; rerun forbidden"

Write-Host "P6 preflight BuildOnly: exact safe reference"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_q2_shaders.ps1")
Require ($LASTEXITCODE -eq 0) "reference shader compile failed"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\build_q2.ps1")
Require ($LASTEXITCODE -eq 0) "reference native build failed"
Verify-Artifacts $Auth

New-Item -ItemType Directory -Path $ResultsRoot|Out-Null
$Preflight=[ordered]@{
  schema="arcllm.anl64.p6.preflight.v0.1"
  git_head=((& git -C $Root rev-parse HEAD).Trim())
  authorization_blob=(GitBlob "config/anl64_p6_execution_authorization_v0.1.json")
  execution_lock_blob=(GitBlob "config/anl64_p6_execution_lock_v0.1.json")
  runner_blob=(GitBlob "run_anl64_p6_confirmatory.ps1")
  model_sha256=$Target.model_sha256
  model_bytes=$Target.model_bytes
  candidate_executable_sha256=Sha256 $CandidateExe
  reference_executable_sha256=Sha256 $ReferenceExe
  candidate_q4_fast_spv_sha256=Sha256 (Join-Path $CandidateShaderDir "anl64_q4_fast.spv")
  candidate_q4_safe_spv_sha256=Sha256 (Join-Path $CandidateShaderDir "p7_q4k_gemm_2d.spv")
  reference_q4_safe_spv_sha256=Sha256 (Join-Path $ReferenceShaderDir "p7_q4k_gemm_2d.spv")
  reference_q6_safe_spv_sha256=Sha256 (Join-Path $ReferenceShaderDir "p7_q6k_gemm_2d.spv")
  environment=[ordered]@{
    os_build=$Target.os.BuildNumber
    cpu=$Target.cpu
    gpu=$Target.gpu
    power=$Target.power
  }
  p5_timing_reused=$false
  performance_measurement_started=$false
}
$PreflightPath=Join-Path $ResultsRoot "P6_PREFLIGHT.json"
[IO.File]::WriteAllText($PreflightPath,($Preflight|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

$Self=$MyInvocation.MyCommand.Path
Write-Host "P6 Session A in separate child process"
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Self -InternalSession -Session A -ModelPath $ModelPath
Require ($LASTEXITCODE -eq 0) "P6 Session A failed"

Write-Host "P6 Session B in separate child process"
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Self -InternalSession -Session B -ModelPath $ModelPath
Require ($LASTEXITCODE -eq 0) "P6 Session B failed"

$MetaA=Get-Content (Join-Path $ResultsRoot "session_A\P6_SESSION_META.json") -Raw -Encoding UTF8|ConvertFrom-Json
$MetaB=Get-Content (Join-Path $ResultsRoot "session_B\P6_SESSION_META.json") -Raw -Encoding UTF8|ConvertFrom-Json
Require ([int]$MetaA.runner_process_id -ne [int]$MetaB.runner_process_id) "sessions did not use distinct runner processes"

$CandidateResult=Join-Path $ResultsRoot "P6_CANDIDATE_RESULT.json"
& py -3 (Join-Path $Root "tools\adjudicate_anl64_p6.py") --root $ResultsRoot --out $CandidateResult
Require ($LASTEXITCODE -eq 0) "P6 candidate adjudicator failed"
Require (Test-Path $CandidateResult -PathType Leaf) "P6 candidate result missing"

Copy-Item $AuthPath (Join-Path $ResultsRoot "anl64_p6_execution_authorization_v0.1.json")
Copy-Item $LockPath (Join-Path $ResultsRoot "anl64_p6_execution_lock_v0.1.json")
Copy-Item (Join-Path $Root "config\anl64_p6_matched_e2e_spec_v0.1.json") (Join-Path $ResultsRoot "anl64_p6_matched_e2e_spec_v0.1.json")
Copy-Item (Join-Path $Root "artifacts\ANL64\ANL64_P6_SPECIFICATION_QA_v0.1.json") (Join-Path $ResultsRoot "ANL64_P6_SPECIFICATION_QA_v0.1.json")
Copy-Item (Join-Path $Root "artifacts\ANL64\ANL64_P5_INTEGRATION_ADJUDICATION_v0.1.json") (Join-Path $ResultsRoot "ANL64_P5_INTEGRATION_ADJUDICATION_v0.1.json")

Compress-Archive -Path (Join-Path $ResultsRoot "*") -DestinationPath $Bundle -CompressionLevel Optimal
$BundleHash=Sha256 $Bundle
$CR=Get-Content $CandidateResult -Raw -Encoding UTF8|ConvertFrom-Json

Write-Host ""
Write-Host "ANL64_P6_EXECUTION_COMPLETE_AWAITING_INDEPENDENT_ADJUDICATION"
Write-Host ("CANDIDATE_CLASSIFICATION="+[string]$CR.candidate_classification)
Write-Host "P7_NOT_AUTHORIZED"
Write-Host ("bundle="+$Bundle)
Write-Host ("SHA256="+$BundleHash)
