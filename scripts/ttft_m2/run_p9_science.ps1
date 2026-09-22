param(
  [Parameter(Mandatory=$true)][string]$Model,
  [string]$PayloadDir=""
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Root=Split-Path -Parent (Split-Path -Parent $Here)
$AuthPath=Join-Path $Root "config\arcllm_ttft_m2_p9c_execution_authorization_v0.1.json"
$BindingPath=Join-Path $Root "config\arcllm_ttft_m2_p9c_a_execution_binding_v0.1.json"
$LockPath=Join-Path $Root "config\arcllm_ttft_m2_p9b_implementation_lock_v0.2.json"
$TargetPath=Join-Path $Root "config\arcllm_ttft_m2_p9a_target_environment_contract_v0.1.json"
if([string]::IsNullOrWhiteSpace($PayloadDir)){$PayloadDir=Join-Path $Root "artifacts\TTFT_M2\P9B\runtime_payload"}
$Exe=Join-Path $PayloadDir "ttft_m2_diagnostic.exe"
$NativeManifestPath=Join-Path $PayloadDir "NATIVE_BUILD.json"
$ShaderManifestPath=Join-Path $PayloadDir "SHADER_BUILD.json"
$ShaderDir=Join-Path $PayloadDir "shaders"
$OutDir=Join-Path $Root "results\ttft_m2_p9_science_raw"

function Fail([string]$m){throw "TTFT_M2_P9_EXECUTION_FAIL_CLOSED: $m"}
function Require([bool]$c,[string]$m){if(-not$c){Fail $m}}
function GitBlob([string]$p){
  $v=(& git -C $Root rev-parse ("HEAD:"+($p -replace '\\','/')) 2>$null)
  if($LASTEXITCODE-ne 0 -or [string]::IsNullOrWhiteSpace($v)){Fail "cannot resolve $p"}
  return $v.Trim()
}
function Sha256([string]$p){
  Require (Test-Path $p -PathType Leaf) ("missing file: "+$p)
  return (Get-FileHash $p -Algorithm SHA256).Hash.ToUpperInvariant()
}
function Norm([string]$s){return (($s -replace '\s+',' ').Trim())}

Write-Host "TTFT M2 P9 science runner"
Write-Host "Hard-gated: no execution without explicit P9C authorization."

Require (Test-Path $AuthPath -PathType Leaf) "P9C execution authorization missing"
Require (Test-Path $BindingPath -PathType Leaf) "P9C-A execution binding missing"
Require (Test-Path $LockPath -PathType Leaf) "P9B implementation lock v0.2 missing"
$Tracked=(& git -C $Root status --porcelain --untracked-files=no|Out-String)
Require ([string]::IsNullOrWhiteSpace($Tracked)) "tracked working tree is dirty"

$Auth=Get-Content $AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
$Binding=Get-Content $BindingPath -Raw -Encoding UTF8|ConvertFrom-Json
$Lock=Get-Content $LockPath -Raw -Encoding UTF8|ConvertFrom-Json
$Target=Get-Content $TargetPath -Raw -Encoding UTF8|ConvertFrom-Json

Require ($Auth.decision -eq "M2_P9C_FRESH_MECHANISM_IDENTIFICATION_EXECUTION_AUTHORIZED") "wrong P9C decision"
Require ([bool]$Auth.authorization.target_model_execution) "target execution not authorized"
Require ([bool]$Auth.authorization.model_load) "model load not authorized"
Require ([bool]$Auth.authorization.gpu_dispatch) "GPU dispatch not authorized"
Require ([bool]$Auth.authorization.performance_measurement) "timing not authorized"
Require ([bool]$Auth.authorization.fresh_ttft_observation) "fresh TTFT not authorized"

# F0 — provenance / execution identity / freshness, before any F1 classification or executable launch.
Require ((GitBlob "config/arcllm_ttft_m2_p9c_a_execution_binding_v0.1.json") -eq [string]$Auth.execution_binding_blob) "P9C-A binding mismatch"
Require ((GitBlob "config/arcllm_ttft_m2_p9b_implementation_lock_v0.2.json") -eq [string]$Auth.p9b_lock_blob) "P9B lock v0.2 binding mismatch"
Require ((GitBlob "scripts/ttft_m2/run_p9_science.ps1") -eq [string]$Auth.science_runner_blob) "science runner binding mismatch"
Require ([string]$Binding.p9b.implementation_lock_blob -eq [string]$Auth.p9b_lock_blob) "binding/auth lock disagreement"

foreach($P in $Lock.exact_git_blobs.PSObject.Properties){
  if($P.Name -eq "scripts/ttft_m2/run_p9_science.ps1"){continue}
  Require ((GitBlob $P.Name) -eq [string]$P.Value) ("P9B immutable Git blob mismatch: "+$P.Name)
}

Require (Test-Path $Exe -PathType Leaf) "P9B executable payload missing"
Require (Test-Path $NativeManifestPath -PathType Leaf) "P9B native build manifest missing"
Require (Test-Path $ShaderManifestPath -PathType Leaf) "P9B shader manifest missing"
Require (Test-Path $ShaderDir -PathType Container) "P9B shader directory missing"

$ExeHash=Sha256 $Exe
Require ($ExeHash -eq ([string]$Binding.p9b.qualified_executable_sha256).ToUpperInvariant()) "F0 executable SHA mismatch"
Require ((Get-Item $Exe).Length -eq [int64]$Binding.p9b.qualified_executable_bytes) "F0 executable byte-size mismatch"
Require ((Sha256 $NativeManifestPath) -eq ([string]$Binding.p9b.qualified_native_manifest_sha256).ToUpperInvariant()) "F0 native manifest SHA mismatch"
Require ((Sha256 $ShaderManifestPath) -eq ([string]$Binding.p9b.qualified_shader_manifest_sha256).ToUpperInvariant()) "F0 shader manifest SHA mismatch"

$NM=Get-Content $NativeManifestPath -Raw -Encoding UTF8|ConvertFrom-Json
$SM=Get-Content $ShaderManifestPath -Raw -Encoding UTF8|ConvertFrom-Json
Require (([string]$NM.executable_sha256).ToUpperInvariant() -eq $ExeHash) "F0 native manifest executable hash mismatch"
Require ([int64]$NM.executable_bytes -eq [int64]$Binding.p9b.qualified_executable_bytes) "F0 native manifest executable size mismatch"
Require ([int]$SM.common_shader_count -eq 16) "F0 common shader census mismatch"

foreach($Row in $SM.common){
  $SpvPath=Join-Path $ShaderDir ([string]$Row.spv)
  Require ((Sha256 $SpvPath) -eq ([string]$Row.spv_sha256).ToUpperInvariant()) ("F0 runtime SPIR-V mismatch: "+[string]$Row.spv)
}
$FastSpvPath=Join-Path $ShaderDir ([string]$SM.q4fast.spv)
Require ((Sha256 $FastSpvPath) -eq ([string]$SM.q4fast.spv_sha256).ToUpperInvariant()) "F0 runtime Q4FAST SPIR-V mismatch"

Require (Test-Path $Model -PathType Leaf) "F0 model missing"
Require ((Get-FileHash $Model -Algorithm SHA256).Hash.ToUpperInvariant() -eq ([string]$Target.required.model_sha256).ToUpperInvariant()) "F0 model SHA mismatch"
Require ((Get-Item $Model).Length -eq [int64]$Target.required.model_bytes) "F0 model byte-size mismatch"

$CPU=Norm ((Get-CimInstance Win32_Processor|Select-Object -First 1 -ExpandProperty Name))
Require ($CPU -eq (Norm ([string]$Target.required.cpu))) ("F0 CPU mismatch: "+$CPU)
$GPU=Get-CimInstance Win32_VideoController|Where-Object {$_.Name -like "*Arc*140V*"}|Select-Object -First 1
Require ($null -ne $GPU) "F0 target GPU not found"
Require ((Norm ([string]$GPU.Name)) -eq (Norm ([string]$Target.required.gpu))) ("F0 GPU mismatch: "+[string]$GPU.Name)
Require ([string]$GPU.DriverVersion -eq [string]$Target.required.driver) ("F0 driver mismatch: "+[string]$GPU.DriverVersion)
$CV=Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'
Require ([string]$CV.CurrentBuild -eq [string]$Target.required.os_build) ("F0 OS build mismatch: "+[string]$CV.CurrentBuild)
$Power=(& powercfg /getactivescheme|Out-String)
Require ($Power -match "381b4222-f694-41f0-9685-ff5bb260df2e" -or $Power -match "Balanced") "F0 Balanced power scheme not active"
$BS=Get-CimInstance -Namespace root\wmi -ClassName BatteryStatus -ErrorAction SilentlyContinue|Select-Object -First 1
Require ($null -ne $BS -and [bool]$BS.PowerOnline) "F0 AC-online state not verifiable/true"

# F1 — H-ART static identity, only after F0 has fully passed.
$SafeAuthPath=Join-Path $Root "config\q2_execution_authorization.json"
Require (Test-Path $SafeAuthPath -PathType Leaf) "historical SAFE artifact authority missing"
$Safe=Get-Content $SafeAuthPath -Raw -Encoding UTF8|ConvertFrom-Json
$HArtMatch=$true
foreach($Row in $SM.common){
  $expected=[string]$Safe.compiled_shader_sha256.($Row.spv)
  Require (-not[string]::IsNullOrWhiteSpace($expected)) ("SAFE hash absent: "+[string]$Row.spv)
  $actual=Sha256 (Join-Path $ShaderDir ([string]$Row.spv))
  if($actual -ne $expected.ToUpperInvariant()){$HArtMatch=$false}
}
if(-not $HArtMatch){
  New-Item -ItemType Directory -Force -Path $OutDir|Out-Null
  $o=[ordered]@{
    schema="arcllm.ttft_m2.p9.h_art_static.v0.1"
    f0="PASS"
    classification="H_ART_SUPPORTED_STATIC_STOP_TIMING"
    executable_launched=$false
    model_loaded=$false
    gpu_dispatch=$false
    performance_measurement=$false
    fresh_ttft_observations=0
  }
  [IO.File]::WriteAllText((Join-Path $OutDir "H_ART_STATIC.json"),($o|ConvertTo-Json -Depth 5),(New-Object Text.UTF8Encoding($false)))
  Write-Host "H_ART_SUPPORTED_STATIC_STOP_TIMING"
  exit 3
}

# F2–F7 raw fresh collection. Independent scientific adjudication occurs later.
if(Test-Path $OutDir){Remove-Item -Recurse -Force $OutDir}
New-Item -ItemType Directory -Force -Path $OutDir|Out-Null
$Head=(& git -C $Root rev-parse HEAD).Trim()

$Orders=[ordered]@{
 A=@("SP/W-S","QP/W-S","QF/W-S","SF/W-S","SF/W-C","QF/W-C","QP/W-C","SP/W-C")
 B=@("SF/W-S","QF/W-S","QP/W-S","SP/W-S","SP/W-C","QP/W-C","QF/W-C","SF/W-C")
}
function ArmArgs([string]$arm){
 switch($arm){
  "SP"{return @("SAFE","PREFILL_ONLY")}
  "SF"{return @("SAFE","FULL_INFERENCE")}
  "QP"{return @("Q4FAST","PREFILL_ONLY")}
  "QF"{return @("Q4FAST","FULL_INFERENCE")}
  default{Fail "unknown arm $arm"}
 }
}
foreach($Session in @("A","B")){
  $SDir=Join-Path $OutDir ("session_"+$Session)
  New-Item -ItemType Directory -Force -Path $SDir|Out-Null
  foreach($Cell in $Orders[$Session]){
    $parts=$Cell.Split("/")
    $arm=$parts[0];$workload=$parts[1];$aa=ArmArgs $arm
    $out=Join-Path $SDir ($arm+"_"+$workload+".json")
    & $Exe --model $Model --shader-dir $ShaderDir --implementation-commit $Head --workload $workload --decode-pipeline $aa[0] --conditioning $aa[1] --measured 5 --out $out
    if($LASTEXITCODE-ne 0){Fail "diagnostic cell failed: $Session $Cell"}
  }
}
$R=[ordered]@{
 schema="arcllm.ttft_m2.p9.raw_collection.v0.1"
 status="FRESH_COLLECTION_COMPLETE_AWAITING_INDEPENDENT_ADJUDICATION"
 git_head=$Head
 execution_binding_blob=(GitBlob "config/arcllm_ttft_m2_p9c_a_execution_binding_v0.1.json")
 science_runner_blob=(GitBlob "scripts/ttft_m2/run_p9_science.ps1")
 qualified_executable_sha256=$ExeHash
 sessions=@("A","B")
 planned_observations=80
 parent_observations_reused=0
 selective_rerun=$false
 h_art="H_ART_FALSIFIED_STATIC"
 f0="PASS"
 mechanism_adjudication_performed=$false
}
[IO.File]::WriteAllText((Join-Path $OutDir "RAW_COLLECTION_MANIFEST.json"),($R|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
Write-Host "TTFT_M2_P9_RAW_COLLECTION_COMPLETE_AWAITING_ADJUDICATION"
