param([string]$VulkanSdkRoot,[switch]$PackageExisting)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path;$Results=Join-Path $Root "results";$BuildDir=Join-Path $Root "artifacts\SA1_K1\build"
New-Item -ItemType Directory -Force $Results,$BuildDir|Out-Null
$ParentLock="60b0fdf91ffe859c918055afa9e3b1071142f6c4"
$Critical=@(
 "shaders/sa1_q4k_subgroup_splitk.comp","src/sa1_component_benchmark.cpp","tools/compile_sa1_component.ps1","tools/build_sa1_component.ps1",
 "run_sa1_component_preflight.ps1","run_sa1_component_q4.ps1","tools/adjudicate_sa1_component.py","tests/test_sa1_component_package.py",
 "config/sa1_k1_implementation_contract_v0.1.json","config/sa1p_implementation_lock_v0.1.json","shaders/p7_q4k_gemm_2d.comp"
)
foreach($Rel in $Critical){& git -C $Root diff --quiet HEAD -- $Rel;if($LASTEXITCODE -ne 0){throw "SA1 critical worktree drift: $Rel"};& git -C $Root diff --cached --quiet HEAD -- $Rel;if($LASTEXITCODE -ne 0){throw "SA1 critical index drift: $Rel"}}
$ExecutionCommit="b45c2cee60e99e4b7700ff032489e77df9fbd3c3"
$CurrentHead=(& git -C $Root rev-parse HEAD).Trim()
$RecoveryMode=[bool]$PackageExisting
if($RecoveryMode){
  $RequiredExisting=@(
    (Join-Path $Results "sa1_k1_preflight_raw.json"),
    (Join-Path $BuildDir "shader_build.json"),
    (Join-Path $BuildDir "native_build.json"),
    (Join-Path $BuildDir "sa1_component_benchmark.exe"),
    (Join-Path $BuildDir "p7_q4k_gemm_2d.spv"),
    (Join-Path $BuildDir "sa1_q4k_subgroup_splitk.spv")
  )
  foreach($P in $RequiredExisting){if(-not(Test-Path $P)){throw "SA1 recovery packaging missing existing artifact: $P"}}
  $NB=Get-Content (Join-Path $BuildDir "native_build.json") -Raw -Encoding UTF8|ConvertFrom-Json
  if([string]$NB.git_head -ne $ExecutionCommit){throw "SA1 recovery native-build execution commit mismatch"}
  $ExpectedCandidateBlob=((& git -C $Root rev-parse ($ExecutionCommit+":shaders/sa1_q4k_subgroup_splitk.comp")).Trim())
  $CurrentCandidateBlob=((& git -C $Root rev-parse "HEAD:shaders/sa1_q4k_subgroup_splitk.comp").Trim())
  $ExpectedHarnessBlob=((& git -C $Root rev-parse ($ExecutionCommit+":src/sa1_component_benchmark.cpp")).Trim())
  $CurrentHarnessBlob=((& git -C $Root rev-parse "HEAD:src/sa1_component_benchmark.cpp").Trim())
  if($ExpectedCandidateBlob -ne $CurrentCandidateBlob -or $ExpectedHarnessBlob -ne $CurrentHarnessBlob){throw "SA1 recovery blocked: implementation source drift since executed preflight"}
}
$C=Get-Content (Join-Path $Root "config\sa1_k1_implementation_contract_v0.1.json") -Raw|ConvertFrom-Json
if([string]$C.parent_implementation_lock_commit -ne $ParentLock){throw "SA1 parent lock mismatch"}
if([bool]$C.measurement_permitted -or [bool]$C.target_model_execution_permitted -or [bool]$C.q6_implementation_permitted){throw "SA1 preflight authorization boundary violation"}
py -3 (Join-Path $Root "tests\test_sa1_component_package.py");if($LASTEXITCODE -ne 0){throw "SA1 static QA failed"}
$OS=Get-CimInstance Win32_OperatingSystem;$CPU=@(Get-CimInstance Win32_Processor|Select Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors);$GPU=@(Get-CimInstance Win32_VideoController|Select Name,DriverVersion,AdapterRAM,VideoProcessor)
if(@($CPU|Where-Object{$_.Name -like "*Ultra 7 258V*"}).Count-lt1){throw "SA1 exact CPU mismatch"};$Arc=@($GPU|Where-Object{$_.Name -like "*Arc*140V*"});if($Arc.Count-lt1){throw "SA1 Arc 140V missing"};if(@($Arc|Where-Object{$_.DriverVersion-eq"32.0.101.8860"}).Count-lt1){throw "SA1 driver mismatch"}
$PowerScheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()
$PowerType=@"
using System;using System.Runtime.InteropServices;
public static class ArcLlmSa1Power{
[StructLayout(LayoutKind.Sequential)] public struct S{public byte ACLineStatus;public byte BatteryFlag;public byte BatteryLifePercent;public byte SystemStatusFlag;public uint BatteryLifeTime;public uint BatteryFullLifeTime;}
[DllImport("kernel32.dll",SetLastError=true)][return:MarshalAs(UnmanagedType.Bool)] public static extern bool GetSystemPowerStatus(out S s);
}
"@
if(-not("ArcLlmSa1Power"-as[type])){Add-Type -TypeDefinition $PowerType};$PS=New-Object "ArcLlmSa1Power+S";if(-not[ArcLlmSa1Power]::GetSystemPowerStatus([ref]$PS)){throw "SA1 power query failed"};if([int]$PS.ACLineStatus-ne1){throw "SA1 requires AC online"}
$Exe=Join-Path $BuildDir "sa1_component_benchmark.exe";$Base=Join-Path $BuildDir "p7_q4k_gemm_2d.spv";$Cand=Join-Path $BuildDir "sa1_q4k_subgroup_splitk.spv";$Raw=Join-Path $Results "sa1_k1_preflight_raw.json"
if(-not $RecoveryMode){
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\compile_sa1_component.ps1");if($LASTEXITCODE-ne0){throw "SA1 compile failed"}
  $BArgs=@("-NoProfile","-ExecutionPolicy","Bypass","-File",(Join-Path $Root "tools\build_sa1_component.ps1"));if($VulkanSdkRoot){$BArgs+=@("-VulkanSdkRoot",$VulkanSdkRoot)};& powershell.exe @BArgs;if($LASTEXITCODE-ne0){throw "SA1 build failed"}
  & $Exe --mode preflight --baseline-spv $Base --candidate-spv $Cand --out $Raw;if($LASTEXITCODE-ne0){throw "SA1 correctness preflight failed"}
}else{
  Write-Host "SA1-K1 recovery mode: reusing existing correctness/build artifacts; no shader compile, native build, or GPU dispatch will run."
}
$R=Get-Content $Raw -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$R.status-ne"PASS_CORRECTNESS_ZERO_MEASUREMENT"){throw "SA1 preflight status mismatch"};if([bool]$R.performance_measurement-or[int]$R.timestamp_queries-ne0-or[int]$R.measured_pairs-ne0-or[bool]$R.performance_gate_evaluated){throw "SA1 zero-measurement invariant violated"};if([bool]$R.model_loaded){throw "SA1 model load forbidden"}
function Get-Sha256([string]$P){(Get-FileHash -Algorithm SHA256 -LiteralPath $P).Hash.ToUpperInvariant()}
$Head=(& git -C $Root rev-parse HEAD).Trim();$ShaderBuild=Join-Path $BuildDir "shader_build.json";$NativeBuild=Join-Path $BuildDir "native_build.json"
$EvidenceExecutionCommit=if($RecoveryMode){$ExecutionCommit}else{$Head}
$Lock=[ordered]@{
 schema="arcllm.sa1.k1.preflight_lock.v0.1"
 status="PASS_ZERO_MEASUREMENT_PREFLIGHT_PENDING_INDEPENDENT_ADJUDICATION"
 created_utc=(Get-Date).ToUniversalTime().ToString("o")
 implementation_commit=$EvidenceExecutionCommit
 packaging_commit=$Head
 packaging_recovery=$RecoveryMode
 packaging_recovery_reason=if($RecoveryMode){"F0_POWERSHELL_HASH_HELPER_ALIAS_COLLISION_AFTER_VALID_ZERO_MEASUREMENT_PREFLIGHT"}else{$null}
 parent_lock_commit=$ParentLock
 environment=[ordered]@{os=$OS.Caption;os_version=$OS.Version;cpu=$CPU;gpu=$GPU;power_scheme=$PowerScheme;ac_line_status=[int]$PS.ACLineStatus}
 hashes=[ordered]@{
   raw_sha256=(Get-Sha256 $Raw)
   shader_build_sha256=(Get-Sha256 $ShaderBuild)
   native_build_sha256=(Get-Sha256 $NativeBuild)
   executable_sha256=(Get-Sha256 $Exe)
   baseline_spv_sha256=(Get-Sha256 $Base)
   candidate_spv_sha256=(Get-Sha256 $Cand)
 }
 zero_measurement=[ordered]@{model_loaded=$false;performance_measurement=$false;timestamp_queries=0;measured_pairs=0;performance_gate_evaluated=$false}
 execution_authorized=$false
 q6_implementation_authorized=$false
 next="Return bundle for independent adjudication. Do not run run_sa1_component_q4.ps1."
}
$LockPath=Join-Path $Results "sa1_k1_preflight_lock.json";[IO.File]::WriteAllText($LockPath,($Lock|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))
$Zip=Join-Path $Results "sa1_k1_preflight_return_to_chatgpt.zip";if(Test-Path$Zip){Remove-Item -Force $Zip}
Compress-Archive -Path @($Raw,$LockPath,$ShaderBuild,$NativeBuild,$Exe,$Base,$Cand,(Join-Path $Root "shaders\sa1_q4k_subgroup_splitk.comp"),(Join-Path $Root "config\sa1_k1_implementation_contract_v0.1.json"),(Join-Path $Root "config\sa1p_implementation_lock_v0.1.json")) -DestinationPath $Zip -Force
Write-Host "SA1-K1 ZERO-MEASUREMENT PREFLIGHT PASS";Write-Host "bundle=$Zip";Write-Host "SHA256=$(Get-Sha256 $Zip)";Write-Host "MEASUREMENT REMAINS FORBIDDEN"
