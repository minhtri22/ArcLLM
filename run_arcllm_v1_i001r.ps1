param(
  [string]$ModelPath,
  [string]$OllamaModelsRoot
)
$ErrorActionPreference="Stop"; Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
$ContractPath=Join-Path $Root "config\arcllm_v1_i001r_profile_contract_v0.1.2.json"
if(-not(Test-Path $ContractPath)){throw "I001R contract missing"}
$C=Get-Content $ContractPath -Raw -Encoding UTF8 | ConvertFrom-Json
if([string]$C.schema -ne "arcllm.v1.i001r.profile_contract.v0.1.2" -or -not[bool]$C.execution_authorized){
  throw "I001R contract not authorized"
}

$Head=(& git -C $Root rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0){throw "Cannot resolve HEAD"}
$Branch=(& git -C $Root rev-parse --abbrev-ref HEAD).Trim()
if($Branch -ne "research/arcllm-v1"){throw "I001R requires research/arcllm-v1; got $Branch"}
$Tracked=(git -C $Root status --porcelain --untracked-files=no | Out-String)
if(-not [string]::IsNullOrWhiteSpace($Tracked)){throw "Tracked worktree is dirty"}

foreach($P in $C.critical_git_blobs.PSObject.Properties){
  $Got=(& git -C $Root rev-parse ("HEAD:"+$P.Name)).Trim()
  if($LASTEXITCODE -ne 0 -or $Got -ne [string]$P.Value){throw "Git blob mismatch: $($P.Name)"}
}

py -3 (Join-Path $Root "tests\test_arcllm_v1_i001r_package.py")
if($LASTEXITCODE -ne 0){throw "I001R static QA failed"}

& (Join-Path $Root "tools\compile_q2_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "I001R shader compile failed"}

foreach($P in $C.compiled_shader_sha256.PSObject.Properties){
  $Spv=Join-Path $Root ("compiled_shaders\"+$P.Name)
  if(-not(Test-Path $Spv)){throw "Missing SPIR-V: $($P.Name)"}
  $Got=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
  if($Got -ne ([string]$P.Value).ToUpperInvariant()){throw "SPIR-V mismatch: $($P.Name)"}
}

& (Join-Path $Root "tools\build_arcllm_v1_i001r.ps1")
if($LASTEXITCODE -ne 0){throw "I001R native build failed"}
$Exe=Join-Path $Root "arcllm_v1_i001r_profile.exe"
if(-not(Test-Path $Exe)){throw "I001R executable missing"}

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Root "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "I001R model resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$ModelFile=Get-Item $ModelPath
$ModelHash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($ModelFile.Length -ne [int64]$C.target_model.bytes -or $ModelHash -ne ([string]$C.target_model.sha256).ToUpperInvariant()){
  throw "I001R target model mismatch"
}

$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor)
$GPU=@(Get-CimInstance Win32_VideoController)
if([string]$OS.BuildNumber -ne [string]$C.environment.os_build){throw "OS build mismatch"}
if(@($CPU|Where-Object {$_.Name -like "*Ultra 7 258V*"}).Count -lt 1){throw "CPU mismatch"}
$Arc=@($GPU|Where-Object {$_.Name -like "*Arc*140V*"})
if($Arc.Count -lt 1){throw "Arc 140V not found"}
if(@($Arc|Where-Object {$_.DriverVersion -eq [string]$C.environment.gpu_driver}).Count -lt 1){throw "GPU driver mismatch"}

$PowerScheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()
if($PowerScheme -notmatch [regex]::Escape([string]$C.environment.power_scheme_guid)){throw "Power scheme mismatch"}

$PowerType=@"
using System;
using System.Runtime.InteropServices;
public static class ArcLlmI001RPower {
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
if(-not ("ArcLlmI001RPower" -as [type])){Add-Type -TypeDefinition $PowerType}
$PS=New-Object "ArcLlmI001RPower+SYSTEM_POWER_STATUS"
if(-not [ArcLlmI001RPower]::GetSystemPowerStatus([ref]$PS)){throw "Cannot query AC status"}
if([int]$PS.ACLineStatus -ne 1){throw "I001R requires AC online"}

$Stamp=(Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssfffZ")
$Dir=Join-Path $Root ("results\arcllm_v1_i001r_"+$Stamp)
New-Item -ItemType Directory -Force -Path $Dir | Out-Null
Copy-Item $ContractPath (Join-Path $Dir "arcllm_v1_i001r_profile_contract_v0.1.2.json") -Force

$Build=[ordered]@{
  schema="arcllm.v1.i001r.build_manifest.v0.1"
  git_head=$Head
  branch=$Branch
  executable_sha256=(Get-FileHash $Exe -Algorithm SHA256).Hash.ToUpperInvariant()
  executable_bytes=(Get-Item $Exe).Length
  profiler_source_blob=(& git -C $Root rev-parse "HEAD:src/arcllm_v1_i001r_7b_decode_profile.cpp").Trim()
  runtime_source_blob=(& git -C $Root rev-parse "HEAD:src/arcllm_v1_i001r_p8c_profile_runtime.cpp").Trim()
  q2_parent_source_blob=(& git -C $Root rev-parse "HEAD:src/q2_benchmark.cpp").Trim()
  shader_provenance_sha256=(Get-FileHash (Join-Path $Root "results\q2_arcllm_shader_provenance.json") -Algorithm SHA256).Hash.ToUpperInvariant()
}
[IO.File]::WriteAllText((Join-Path $Dir "I001R_BUILD_MANIFEST.json"),($Build|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Env=[ordered]@{
  schema="arcllm.v1.i001r.environment.v0.1"
  captured_utc=(Get-Date).ToUniversalTime().ToString("o")
  git_head=$Head
  os=[ordered]@{caption=$OS.Caption;version=$OS.Version;build=$OS.BuildNumber;architecture=$OS.OSArchitecture}
  cpu=@($CPU|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
  gpu=@($GPU|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
  power_scheme=$PowerScheme
  ac_online=$true
  model_path=$ModelPath
  model_sha256=$ModelHash
  model_bytes=$ModelFile.Length
}
[IO.File]::WriteAllText((Join-Path $Dir "I001R_ENVIRONMENT.json"),($Env|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))

$RunStatus="RUNNING"
$Failure=""
$Executed=@()
function Invoke-I001RCell([string]$Name,[string]$Workload){
  $Out=Join-Path $Dir ("i001r_"+$Name+".json")
  Write-Host "I001R cell $Name ($Workload)"
  & $Exe --model $ModelPath --shader-dir (Join-Path $Root "compiled_shaders") --implementation-commit $Head --workload $Workload --warmups 1 --measured 5 --out $Out
  $Code=$LASTEXITCODE
  $script:Executed += $Name
  if($Code -ne 0){throw "I001R cell $Name failed exit=$Code"}
  if(-not(Test-Path $Out)){throw "I001R cell $Name result missing"}
}

try {
  Invoke-I001RCell "A_WS" "W-S"
  Invoke-I001RCell "A_WC" "W-C"
  Invoke-I001RCell "B_WC" "W-C"
  Invoke-I001RCell "B_WS" "W-S"
  $RunStatus="COLLECTION_COMPLETE"
} catch {
  $RunStatus="FAIL_CLOSED_PARTIAL_COLLECTION"
  $Failure=$_.Exception.Message
}

$Summary=Join-Path $Dir "I001R_PROFILE_SUMMARY.json"
& py -3 (Join-Path $Root "tools\summarize_arcllm_v1_i001r.py") --results-dir $Dir --out $Summary
$SummaryExit=$LASTEXITCODE
if($SummaryExit -ne 0 -and $RunStatus -eq "COLLECTION_COMPLETE"){
  $RunStatus="FAIL_CLOSED_ADJUDICATION_INPUT_INVALID"
  $Failure="profile summarizer rejected complete collection"
}

$Meta=[ordered]@{
  schema="arcllm.v1.i001r.run_meta.v0.1"
  git_head=$Head
  status=$RunStatus
  failure=$Failure
  cell_order=@("A_WS","A_WC","B_WC","B_WS")
  executed_cells=$Executed
  warmups_per_cell=1
  measured_attempts_per_cell=5
  expected_measured_attempts=20
  probe_decode_indices=@(0,15,30)
  expected_profiled_decode_steps=60
  expected_normal_lifecycle_steps=560
  automatic_rerun_performed=$false
  selective_rerun_performed=$false
  production_optimization_performed=$false
  pdep_implementation_performed=$false
  replacement_intervention_selected=$false
}
[IO.File]::WriteAllText((Join-Path $Dir "I001R_RUN_META.json"),($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Bundle=Join-Path $Dir "arcllm_v1_i001r_return_to_chatgpt.zip"
$Files=@(Get-ChildItem -File $Dir|Where-Object {$_.Name -ne [IO.Path]::GetFileName($Bundle)}|Select-Object -ExpandProperty FullName)
Compress-Archive -Path $Files -DestinationPath $Bundle -Force
$BundleHash=(Get-FileHash $Bundle -Algorithm SHA256).Hash.ToUpperInvariant()

Write-Host ""
Write-Host "=============================================="
Write-Host "ARCLLM V1 I001R TERMINATED"
Write-Host "=============================================="
Write-Host "STATUS=$RunStatus"
Write-Host "RETURN_BUNDLE=$Bundle"
Write-Host "RETURN_BUNDLE_SHA256=$BundleHash"
Write-Host "AUTOMATIC_RERUN=false"
Write-Host "PDEP_IMPLEMENTATION=false"

if($RunStatus -ne "COLLECTION_COMPLETE"){exit 2}
