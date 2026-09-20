param(
  [string]$ModelPath,
  [string]$OllamaModelsRoot,
  [string]$LlamaDir,
  [string]$BuildDir
)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$Results=Join-Path $Here "results";New-Item -ItemType Directory -Force -Path $Results|Out-Null
$ExpectedModelHash="60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
$ExpectedModelBytes=[int64]4683074048
$BaselineCommit="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
$BaselineRelease="v0.4.1"
$Q2ImplementationCommit="43afd71161c4dc8c766c09c3b55d5eca48352bde"

$Head=(& git -C $Here rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0 -or -not $Head){throw "Q3 preflight cannot resolve HEAD"}
$Q3Critical=@(
 "run_q3.ps1","run_q3_preflight.ps1","package_q3.ps1",
 "tools/adjudicate_q3.py","tools/package_q3_evidence.py","tests/test_q3_package.py",
 "config/q3_confirmatory_design.json","docs/Q3_NO_PRACTICAL_ADVANTAGE_CONFIRMATORY_DESIGN.md","docs/Q3_IMPLEMENTATION.md"
)
$Governance=@("manifest.json","lineage.md","README.md")
& git -C $Here diff --quiet HEAD -- @($Q3Critical+$Governance)
if($LASTEXITCODE -ne 0){throw "Q3 preflight blocked: critical/governance working-tree drift"}
& git -C $Here diff --cached --quiet HEAD -- @($Q3Critical+$Governance)
if($LASTEXITCODE -ne 0){throw "Q3 preflight blocked: critical/governance index drift"}
if(Test-Path (Join-Path $Here "config\q3_execution_authorization.json")){throw "Q3 preflight requires execution authorization to remain absent"}
foreach($S in @("A","B")){if(Test-Path (Join-Path $Results ("q3_session_"+$S))){throw "Q3 preflight requires zero Q3 sessions"}}

$Manifest=Get-Content (Join-Path $Here "manifest.json") -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$Manifest.status -ne "Q3_IMPLEMENTATION_STATIC_LOCKED" -or [bool]$Manifest.q3_permitted -or [bool]$Manifest.target_run_permitted -or [bool]$Manifest.q3.execution_permitted){throw "Q3 preflight manifest gate is not closed"}

# Fail-fast environment qualification before expensive hashing/build/model load.
$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
$GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
if(@($CPU|Where-Object {$_.Name -like "*Ultra 7 258V*"}).Count -lt 1){throw "Q3 preflight frozen CPU mismatch"}
$ArcGPU=@($GPU|Where-Object {$_.Name -like "*Arc*140V*"})
if($ArcGPU.Count -lt 1 -or @($ArcGPU|Where-Object {$_.DriverVersion -eq "32.0.101.8860"}).Count -lt 1){throw "Q3 preflight frozen GPU/driver mismatch"}
$PowerScheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()
$PowerStatusType=@"
using System;
using System.Runtime.InteropServices;
public static class ArcLlmQ3PreflightPower {
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
if(-not ("ArcLlmQ3PreflightPower" -as [type])){Add-Type -TypeDefinition $PowerStatusType}
$SP=New-Object "ArcLlmQ3PreflightPower+SYSTEM_POWER_STATUS"
if(-not [ArcLlmQ3PreflightPower]::GetSystemPowerStatus([ref]$SP)){throw "Q3 preflight cannot query AC state"}
if([int]$SP.ACLineStatus -ne 1){throw "Q3 preflight requires ACLineStatus=1; observed $([int]$SP.ACLineStatus)"}

$DesignPath=Join-Path $Here "config\q3_confirmatory_design.json"
$Design=Get-Content $DesignPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$Design.schema -ne "arcllm.q3.no_practical_advantage_confirmatory_design.v1" -or [string]$Design.status -ne "DESIGN_FROZEN_EXECUTION_CLOSED"){throw "Q3 design contract mismatch"}
if([int]$Design.fresh_reproduction.total_measured_attempts -ne 40 -or [double]$Design.practical_effect_thresholds.speed_or_latency_minimum_relative_improvement -ne 0.1 -or [double]$Design.practical_effect_thresholds.working_set_minimum_relative_improvement -ne 0.15 -or [double]$Design.practical_effect_thresholds.maximum_allowed_blocking_harm -ne 0.1){throw "Q3 frozen threshold mismatch"}
$DesignHash=(Get-FileHash $DesignPath -Algorithm SHA256).Hash.ToUpperInvariant()

$Q2FormalPath=Join-Path $Here "inputs\q2_formal_result.authoritative.json"
$Q2Formal=Get-Content $Q2FormalPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$Q2Formal.classification -ne "Q2_MATCHED_CHARACTERIZATION_COMPLETE" -or [int]$Q2Formal.measured_attempts_recorded -ne 20 -or [int]$Q2Formal.successful_attempts -ne 20){throw "Q3 parent Q2 formal result mismatch"}
$Q2FormalHash=(Get-FileHash $Q2FormalPath -Algorithm SHA256).Hash.ToUpperInvariant()

$Q2AuthPath=Join-Path $Here "config\q2_execution_authorization.json"
$Q2Auth=Get-Content $Q2AuthPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$Q2Auth.preflight_implementation_commit -ne $Q2ImplementationCommit -or [string]$Q2Auth.baseline_commit -ne $BaselineCommit){throw "Q3 Q2 frozen-runtime source binding mismatch"}
$Q2Frozen=[ordered]@{}
foreach($P in $Q2Auth.critical_file_sha256.PSObject.Properties){
  $Path=Join-Path $Here $P.Name
  if(-not(Test-Path $Path)){throw "Q3 frozen Q2 critical file missing: $($P.Name)"}
  $Now=(Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant()
  if($Now -ne ([string]$P.Value).ToUpperInvariant()){throw "Q3 architecture/instrumentation changed since Q2: $($P.Name)"}
  $Q2Frozen[$P.Name]=$Now
}
foreach($P in $Q2Auth.shader_source_sha256.PSObject.Properties){
  $Path=Join-Path $Here ("shaders\"+$P.Name)
  if(-not(Test-Path $Path) -or (Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant() -ne ([string]$P.Value).ToUpperInvariant()){throw "Q3 shader source changed since Q2: $($P.Name)"}
}

py -3 (Join-Path $Here "tests\test_q3_package.py")
if($LASTEXITCODE -ne 0){throw "Q3 static package QA failed"}
$Static=[ordered]@{schema="arcllm.q3.static_qa.v1";status="PASS";implementation_commit=$Head;design_sha256=$DesignHash;measurements_executed=$false;measured_attempts=0}
$StaticPath=Join-Path $Results "q3_static_qa.json"
[IO.File]::WriteAllText($StaticPath,($Static|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "Q3 preflight model resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$mf=Get-Item $ModelPath
$ModelHash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($mf.Length -ne $ExpectedModelBytes -or $ModelHash -ne $ExpectedModelHash){throw "Q3 preflight exact target mismatch"}

powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_q2_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "Q3 preflight frozen shader compile failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_q2.ps1")
if($LASTEXITCODE -ne 0){throw "Q3 preflight frozen ArcLLM build failed"}
$QualArgs=@("-ExecutionPolicy","Bypass","-File",(Join-Path $Here "tools\qualify_q2_baseline.ps1"))
if($LlamaDir){$QualArgs+=@("-LlamaDir",$LlamaDir)}
if($BuildDir){$QualArgs+=@("-BuildDir",$BuildDir)}
& powershell.exe @QualArgs
if($LASTEXITCODE -ne 0){throw "Q3 preflight baseline build/API qualification failed"}

$BaselineExe=Join-Path $Here "artifacts\q2_baseline\q2_llama_adapter.exe"
$BuildQual=Join-Path $Results "q2_baseline_qualification.json"
$ArcExe=Join-Path $Here "arcllm_q2.exe"
$ShaderProv=Join-Path $Results "q2_arcllm_shader_provenance.json"
foreach($Path in @($BaselineExe,$BuildQual,$ArcExe,$ShaderProv)){if(-not(Test-Path $Path)){throw "Q3 preflight artifact missing: $Path"}}
$BQ=Get-Content $BuildQual -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$BQ.commit -ne $BaselineCommit -or [string]$BQ.release -ne $BaselineRelease -or [string]$BQ.qualification -ne "BUILD_API_QUALIFIED" -or [bool]$BQ.target_model_executed){throw "Q3 baseline build qualification mismatch"}

$RuntimeRows=@()
foreach($W in @("W-S","W-C")){
  $Out=Join-Path $Results ("q3_baseline_runtime_qualification_"+($W -replace "-","_")+".json")
  & $BaselineExe --model $ModelPath --workload $W --qualify-only --out $Out | Out-Host
  $Code=[int]$LASTEXITCODE
  if($Code -ne 0 -or -not(Test-Path $Out)){throw "Q3 baseline runtime qualification failed for $W"}
  $Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
  $ExpectedPromptHash=if($W -eq "W-S"){"93833ffb49890aba"}else{"5973d0cfd8ad6313"}
  if([string]$Obj.status -ne "QUALIFIED" -or [bool]$Obj.decode_executed -or [int]$Obj.measured_attempts -ne 0){throw "Q3 preflight must remain zero-decode/zero-measurement"}
  if(-not[bool]$Obj.runtime.vulkan_log_present -or -not[bool]$Obj.runtime.full_offload -or [int]$Obj.runtime.offloaded_layers -ne [int]$Obj.runtime.offloaded_layers_total){throw "Q3 baseline runtime not full-offload matched"}
  if(([string]$Obj.prompt_hash_fnv1a64).ToLowerInvariant() -ne $ExpectedPromptHash){throw "Q3 baseline prompt identity mismatch"}
  $RuntimeRows += [ordered]@{workload=$W;path=(Resolve-Path $Out).Path;sha256=(Get-FileHash $Out -Algorithm SHA256).Hash.ToUpperInvariant();status=$Obj.status;prompt_hash_fnv1a64=[string]$Obj.prompt_hash_fnv1a64;full_offload=[bool]$Obj.runtime.full_offload;offloaded_layers=[int]$Obj.runtime.offloaded_layers;offloaded_layers_total=[int]$Obj.runtime.offloaded_layers_total;decode_executed=[bool]$Obj.decode_executed;measured_attempts=[int]$Obj.measured_attempts}
}

$Prov=Get-Content $ShaderProv -Raw -Encoding UTF8|ConvertFrom-Json
if([int]$Prov.shader_count -ne 16){throw "Q3 shader provenance count mismatch"}
$ShaderSource=[ordered]@{};$Compiled=[ordered]@{}
foreach($Item in @($Prov.compiled)){
  $Src=Join-Path $Here ("shaders\"+[string]$Item.source);$Spv=Join-Path $Here ("compiled_shaders\"+[string]$Item.spv)
  $SrcHash=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant()
  $SpvHash=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
  $Q2Src=$Q2Auth.shader_source_sha256.PSObject.Properties[[string]$Item.source]
  if($null -eq $Q2Src -or $SrcHash -ne ([string]$Q2Src.Value).ToUpperInvariant()){throw "Q3 shader source no longer matches Q2 frozen source"}
  $ShaderSource[[string]$Item.source]=$SrcHash;$Compiled[[string]$Item.spv]=$SpvHash
}

$Q3Hashes=[ordered]@{}
foreach($Rel in $Q3Critical){$Q3Hashes[$Rel]=(Get-FileHash (Join-Path $Here $Rel) -Algorithm SHA256).Hash.ToUpperInvariant()}
$Lock=[ordered]@{
 schema="arcllm.q3.preflight_lock.v1";implementation_commit=$Head;created_utc=(Get-Date).ToUniversalTime().ToString("o")
 design_sha256=$DesignHash;q2_formal_result_sha256=$Q2FormalHash;q2_parent_classification=[string]$Q2Formal.classification
 target_sha256=$ModelHash;target_size_bytes=$mf.Length
 baseline_release=$BaselineRelease;baseline_commit=$BaselineCommit
 baseline_exe_sha256=(Get-FileHash $BaselineExe -Algorithm SHA256).Hash.ToUpperInvariant()
 arcllm_exe_sha256=(Get-FileHash $ArcExe -Algorithm SHA256).Hash.ToUpperInvariant()
 shader_provenance_sha256=(Get-FileHash $ShaderProv -Algorithm SHA256).Hash.ToUpperInvariant()
 shader_source_sha256=$ShaderSource;compiled_shader_sha256=$Compiled
 q2_frozen_runtime_file_sha256=$Q2Frozen;q3_critical_file_sha256=$Q3Hashes
 static_qa="PASS";arcllm_build="PASS";baseline_build_api="PASS";baseline_runtime_qualification="PASS";runtime_qualifications=$RuntimeRows
 hardware=[ordered]@{os_caption=$OS.Caption;os_version=$OS.Version;os_build=$OS.BuildNumber;cpu=$CPU;gpu=$GPU;power_scheme=$PowerScheme;ac_power_online=$true;ac_status_source="GetSystemPowerStatus"}
 q3_execution_authorization_required=$true;q3_execution_authorized=$false
 measurements_executed=$false;measured_attempts=0;sessions_executed=@()
}
$LockPath=Join-Path $Results "q3_preflight_lock.json"
[IO.File]::WriteAllText($LockPath,($Lock|ConvertTo-Json -Depth 14),(New-Object Text.UTF8Encoding($false)))
$Bundle=Join-Path $Results "q3_preflight_return_to_chatgpt.zip"
if(Test-Path $Bundle){Remove-Item -Force $Bundle}
$Pack=@($LockPath,$StaticPath,$DesignPath,$Q2FormalPath,$Q2AuthPath,$BuildQual,$ShaderProv,(Join-Path $Results "q3_baseline_runtime_qualification_W_S.json"),(Join-Path $Results "q3_baseline_runtime_qualification_W_C.json"))
Compress-Archive -Path $Pack -DestinationPath $Bundle -Force
Write-Host "Q3 PRE-FLIGHT PASS / ZERO DECODE / ZERO MEASURED ATTEMPTS"
Write-Host "  lock=$LockPath"
Write-Host "  bundle=$Bundle"
Write-Host "Q3 execution remains CLOSED until independent adjudication commits config/q3_execution_authorization.json."
