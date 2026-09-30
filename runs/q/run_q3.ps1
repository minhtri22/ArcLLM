param(
  [Parameter(Mandatory=$true)]
  [ValidateSet("A","B")]
  [string]$Session,
  [string]$ModelPath,
  [string]$OllamaModelsRoot,
  [string]$BaselineExe
)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=(Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "..\..")).Path
$ResultsRoot=Join-Path $Here "results"
New-Item -ItemType Directory -Force -Path $ResultsRoot|Out-Null
$SessionDir=Join-Path $ResultsRoot ("q3_session_"+$Session)
if(Test-Path $SessionDir){throw "Q3 session $Session already exists; frozen sessions cannot be silently rerun or overwritten"}

$ExpectedModelHash="60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
$ExpectedModelBytes=[int64]4683074048
$BaselineCommit="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
$BaselineRelease="v0.4.1"

$Head=(& git -C $Here rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0 -or -not $Head){throw "Q3 cannot resolve HEAD"}
$PreflightLockPath=Join-Path $ResultsRoot "q3_preflight_lock.json"
if(-not(Test-Path $PreflightLockPath)){throw "Q3 blocked: q3_preflight_lock.json missing"}
$PF=Get-Content $PreflightLockPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$PF.schema -ne "arcllm.q3.preflight_lock.v1"){throw "Q3 blocked: invalid preflight lock"}
$ImplementationCommit=[string]$PF.implementation_commit
& git -C $Here merge-base --is-ancestor $ImplementationCommit $Head
if($LASTEXITCODE -ne 0){throw "Q3 blocked: preflight implementation commit is not an ancestor of HEAD"}
if([bool]$PF.measurements_executed -or [int]$PF.measured_attempts -ne 0 -or @($PF.sessions_executed).Count -ne 0){throw "Q3 blocked: preflight is not zero-measurement clean"}

$AuthorizationPath=Join-Path $Here "config\q3_execution_authorization.json"
if(-not(Test-Path $AuthorizationPath)){throw "Q3 blocked: independent preflight adjudication and committed q3_execution_authorization.json required"}
& git -C $Here ls-files --error-unmatch -- "config/q3_execution_authorization.json" *> $null
if($LASTEXITCODE -ne 0){throw "Q3 blocked: execution authorization is not committed"}
& git -C $Here diff --quiet HEAD -- "config/q3_execution_authorization.json"
if($LASTEXITCODE -ne 0){throw "Q3 blocked: execution authorization differs from committed HEAD"}
$Auth=Get-Content $AuthorizationPath -Raw -Encoding UTF8|ConvertFrom-Json
$AuthorizationHash=(Get-FileHash $AuthorizationPath -Algorithm SHA256).Hash.ToUpperInvariant()
$PreflightLockHash=(Get-FileHash $PreflightLockPath -Algorithm SHA256).Hash.ToUpperInvariant()
if([string]$Auth.schema -ne "arcllm.q3.execution_authorization.v1" -or -not[bool]$Auth.authorized -or [string]$Auth.decision -ne "Q3_EXECUTION_AUTHORIZED"){throw "Q3 blocked: invalid execution authorization"}
if([string]$Auth.preflight_implementation_commit -ne $ImplementationCommit -or ([string]$Auth.preflight_lock_sha256).ToUpperInvariant() -ne $PreflightLockHash){throw "Q3 blocked: authorization does not bind this preflight"}
if([bool]$Auth.measurements_executed -or [int]$Auth.measured_attempts -ne 0 -or @($Auth.sessions_executed).Count -ne 0){throw "Q3 blocked: authorization is not fresh"}

$Manifest=Get-Content (Join-Path $Here "manifest.json") -Raw -Encoding UTF8|ConvertFrom-Json
if(-not[bool]$Manifest.q3_permitted -or -not[bool]$Manifest.target_run_permitted -or -not[bool]$Manifest.q3.execution_permitted){throw "Q3 blocked: manifest execution gate is closed"}
if([string]$Manifest.status -ne "Q3_EXECUTION_AUTHORIZED" -or [string]$Manifest.q3.status -ne "Q3_EXECUTION_AUTHORIZED"){throw "Q3 blocked: manifest authorization state mismatch"}

$DesignPath=Join-Path $Here "config\q3_confirmatory_design.json"
$DesignHash=(Get-FileHash $DesignPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($DesignHash -ne ([string]$PF.design_sha256).ToUpperInvariant() -or $DesignHash -ne ([string]$Auth.design_sha256).ToUpperInvariant()){throw "Q3 blocked: frozen design drift"}
$Design=Get-Content $DesignPath -Raw -Encoding UTF8|ConvertFrom-Json

$Q3Critical=@(
 "runs/q/run_q3.ps1","runs/q/run_q3_preflight.ps1","runs/q/package_q3.ps1",
 "tools/adjudicate_q3.py","tools/package_q3_evidence.py","tests/test_q3_package.py",
 "config/q3_confirmatory_design.json","docs/Q3_NO_PRACTICAL_ADVANTAGE_CONFIRMATORY_DESIGN.md","docs/Q3_IMPLEMENTATION.md"
)
& git -C $Here diff --quiet HEAD -- @Q3Critical
if($LASTEXITCODE -ne 0){throw "Q3 blocked: implementation-critical working tree drift"}
& git -C $Here diff --cached --quiet HEAD -- @Q3Critical
if($LASTEXITCODE -ne 0){throw "Q3 blocked: implementation-critical index drift"}
foreach($P in $PF.q3_critical_file_sha256.PSObject.Properties){
  $Path=Join-Path $Here $P.Name
  $Now=(Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant()
  if($Now -ne ([string]$P.Value).ToUpperInvariant()){throw "Q3 blocked: critical file drift: $($P.Name)"}
  $A=$Auth.q3_critical_file_sha256.PSObject.Properties[$P.Name]
  if($null -eq $A -or ([string]$A.Value).ToUpperInvariant() -ne ([string]$P.Value).ToUpperInvariant()){throw "Q3 blocked: authorization critical binding mismatch: $($P.Name)"}
}
foreach($P in $PF.q2_frozen_runtime_file_sha256.PSObject.Properties){
  $Path=Join-Path $Here $P.Name
  $Now=(Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant()
  if($Now -ne ([string]$P.Value).ToUpperInvariant()){throw "Q3 blocked: frozen Q2 runtime path changed: $($P.Name)"}
  $A=$Auth.q2_frozen_runtime_file_sha256.PSObject.Properties[$P.Name]
  if($null -eq $A -or ([string]$A.Value).ToUpperInvariant() -ne ([string]$P.Value).ToUpperInvariant()){throw "Q3 blocked: authorization Q2-runtime binding mismatch: $($P.Name)"}
}

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "Q3 model resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$mf=Get-Item $ModelPath
$ModelHash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($mf.Length -ne $ExpectedModelBytes -or $ModelHash -ne $ExpectedModelHash){throw "Q3 exact target mismatch"}
if($ModelHash -ne ([string]$PF.target_sha256).ToUpperInvariant() -or $ModelHash -ne ([string]$Auth.target_sha256).ToUpperInvariant()){throw "Q3 target differs from lock/authorization"}

if(-not $BaselineExe){$BaselineExe=Join-Path $Here "artifacts\q2_baseline\q2_llama_adapter.exe"}
$ArcExe=Join-Path $Here "arcllm_q2.exe"
$ShaderProv=Join-Path $ResultsRoot "q2_arcllm_shader_provenance.json"
foreach($Path in @($ArcExe,$BaselineExe,$ShaderProv)){if(-not(Test-Path $Path)){throw "Q3 preflight-built artifact missing: $Path"}}
$ArcHash=(Get-FileHash $ArcExe -Algorithm SHA256).Hash.ToUpperInvariant()
$BaseHash=(Get-FileHash $BaselineExe -Algorithm SHA256).Hash.ToUpperInvariant()
$ProvHash=(Get-FileHash $ShaderProv -Algorithm SHA256).Hash.ToUpperInvariant()
if($ArcHash -ne ([string]$PF.arcllm_exe_sha256).ToUpperInvariant() -or $ArcHash -ne ([string]$Auth.arcllm_exe_sha256).ToUpperInvariant()){throw "Q3 Arc executable drift"}
if($BaseHash -ne ([string]$PF.baseline_exe_sha256).ToUpperInvariant() -or $BaseHash -ne ([string]$Auth.baseline_exe_sha256).ToUpperInvariant()){throw "Q3 baseline executable drift"}
if($ProvHash -ne ([string]$PF.shader_provenance_sha256).ToUpperInvariant() -or $ProvHash -ne ([string]$Auth.shader_provenance_sha256).ToUpperInvariant()){throw "Q3 shader provenance drift"}
foreach($P in $PF.compiled_shader_sha256.PSObject.Properties){
  $Path=Join-Path $Here ("compiled_shaders\"+$P.Name)
  if(-not(Test-Path $Path) -or (Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant() -ne ([string]$P.Value).ToUpperInvariant()){throw "Q3 compiled shader drift: $($P.Name)"}
  $A=$Auth.compiled_shader_sha256.PSObject.Properties[$P.Name]
  if($null -eq $A -or ([string]$A.Value).ToUpperInvariant() -ne ([string]$P.Value).ToUpperInvariant()){throw "Q3 authorization compiled-shader binding mismatch: $($P.Name)"}
}
py -3 (Join-Path $Here "tests\test_q3_package.py")
if($LASTEXITCODE -ne 0){throw "Q3 static package failed in authorized state"}

$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
$GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
if(@($CPU|Where-Object {$_.Name -like "*Ultra 7 258V*"}).Count -lt 1){throw "Q3 frozen CPU mismatch"}
$ArcGPU=@($GPU|Where-Object {$_.Name -like "*Arc*140V*"})
if($ArcGPU.Count -lt 1 -or @($ArcGPU|Where-Object {$_.DriverVersion -eq "32.0.101.8860"}).Count -lt 1){throw "Q3 frozen GPU/driver mismatch"}
$PowerScheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()
$PowerStatusType=@"
using System;
using System.Runtime.InteropServices;
public static class ArcLlmQ3Power {
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
if(-not ("ArcLlmQ3Power" -as [type])){Add-Type -TypeDefinition $PowerStatusType}
$SystemPower=New-Object "ArcLlmQ3Power+SYSTEM_POWER_STATUS"
if(-not [ArcLlmQ3Power]::GetSystemPowerStatus([ref]$SystemPower)){throw "Q3 cannot query system AC status"}
$AcLineStatus=[int]$SystemPower.ACLineStatus
if($AcLineStatus -ne 1){throw "Q3 requires ACLineStatus=1; observed $AcLineStatus"}
if([string]$OS.Version -ne [string]$PF.hardware.os_version -or [string]$OS.BuildNumber -ne [string]$PF.hardware.os_build){throw "Q3 OS drift since preflight"}
if($PowerScheme -ne [string]$PF.hardware.power_scheme){throw "Q3 power scheme drift since preflight"}

# Create the immutable session directory only after every gate has passed.
New-Item -ItemType Directory -Path $SessionDir|Out-Null

$Environment=[ordered]@{
 schema="arcllm.q3.environment.v1";session=$Session;captured_utc=(Get-Date).ToUniversalTime().ToString("o")
 runner_process_id=$PID;implementation_commit=$ImplementationCommit;execution_authorization_commit=$Head
 preflight_lock_sha256=$PreflightLockHash;execution_authorization_sha256=$AuthorizationHash;design_sha256=$DesignHash
 os=[ordered]@{caption=$OS.Caption;version=$OS.Version;build_number=$OS.BuildNumber;architecture=$OS.OSArchitecture}
 cpu=$CPU;gpu=$GPU;power_scheme=$PowerScheme
 system_power=[ordered]@{source="GetSystemPowerStatus";ac_line_status=$AcLineStatus;ac_line_status_text="Online"}
 model_sha256=$ModelHash;model_size_bytes=$mf.Length
 baseline=[ordered]@{release=$BaselineRelease;commit=$BaselineCommit;adapter_sha256=$BaseHash}
}
$EnvPath=Join-Path $SessionDir "q3_environment.json"
[IO.File]::WriteAllText($EnvPath,($Environment|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))

$Sampler=Join-Path $Here "tools\q2_resource_sampler.py"
$GpuSampler=Join-Path $Here "tools\q2_gpu_sampler.ps1"
function Invoke-Q3Cell([string]$Key,[string]$System,[string]$Workload,[string]$Exe,[string[]]$ChildArgs){
  $Result=Join-Path $SessionDir ($Key+"_result.json")
  $Trace=Join-Path $SessionDir ($Key+"_resources.json")
  $Log=Join-Path $SessionDir ($Key+".log")
  $Args=@("-3",$Sampler,"--system",$System,"--workload",$Workload,"--trace",$Trace,"--log",$Log,"--gpu-script",$GpuSampler,"--sample-ms","100","--",$Exe)
  $Args += $ChildArgs
  $Args += @("--workload",$Workload,"--warmups","1","--measured","5","--out",$Result)
  Write-Host "";Write-Host "Q3 Session $Session cell $Key : $System $Workload"
  & py @Args | Out-Host
  $Code=[int]$LASTEXITCODE
  if(-not(Test-Path $Trace)){throw "Q3 resource trace missing for $Key"}
  if(-not(Test-Path $Result)){
    $Fail=[ordered]@{schema="arcllm.q3.cell_failure.v1";system=$System;workload=$Workload;process_exit_code=$Code;error="child result JSON missing"}
    [IO.File]::WriteAllText($Result,($Fail|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
  }
  return [int]$Code
}

$ArcArgs=@("--model",$ModelPath,"--shader-dir",(Join-Path $Here "compiled_shaders"),"--implementation-commit",$ImplementationCommit)
$BaseArgs=@("--model",$ModelPath)
$CellExit=[ordered]@{}
$Order=@()
if($Session -eq "A"){
  $Order=@("arcllm_ws","baseline_ws","baseline_wc","arcllm_wc")
  $CellExit.arcllm_ws=Invoke-Q3Cell "arcllm_ws" "ArcLLM" "W-S" $ArcExe $ArcArgs
  $CellExit.baseline_ws=Invoke-Q3Cell "baseline_ws" "llama.cpp" "W-S" $BaselineExe $BaseArgs
  $CellExit.baseline_wc=Invoke-Q3Cell "baseline_wc" "llama.cpp" "W-C" $BaselineExe $BaseArgs
  $CellExit.arcllm_wc=Invoke-Q3Cell "arcllm_wc" "ArcLLM" "W-C" $ArcExe $ArcArgs
}else{
  $Order=@("baseline_ws","arcllm_ws","arcllm_wc","baseline_wc")
  $CellExit.baseline_ws=Invoke-Q3Cell "baseline_ws" "llama.cpp" "W-S" $BaselineExe $BaseArgs
  $CellExit.arcllm_ws=Invoke-Q3Cell "arcllm_ws" "ArcLLM" "W-S" $ArcExe $ArcArgs
  $CellExit.arcllm_wc=Invoke-Q3Cell "arcllm_wc" "ArcLLM" "W-C" $ArcExe $ArcArgs
  $CellExit.baseline_wc=Invoke-Q3Cell "baseline_wc" "llama.cpp" "W-C" $BaselineExe $BaseArgs
}

$Meta=[ordered]@{
 schema="arcllm.q3.session_meta.v1";session=$Session;runner_process_id=$PID
 started_from_fresh_session_dir=$true;execution_order=$Order;cell_exit_codes=$CellExit
 warmups_per_cell=1;measured_attempts_per_cell=5;expected_measured_attempts=20
 implementation_commit=$ImplementationCommit;execution_authorization_commit=$Head
 preflight_lock_sha256=$PreflightLockHash;execution_authorization_sha256=$AuthorizationHash;design_sha256=$DesignHash
 q3_candidate_adjudicated=$false
}
$MetaPath=Join-Path $SessionDir "q3_session_meta.json"
[IO.File]::WriteAllText($MetaPath,($Meta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

$Hashes=[ordered]@{}
foreach($F in Get-ChildItem -File $SessionDir|Sort-Object Name){$Hashes[$F.Name]=(Get-FileHash $F.FullName -Algorithm SHA256).Hash.ToUpperInvariant()}
$Complete=[ordered]@{
 schema="arcllm.q3.session_complete.v1";session=$Session;completed_utc=(Get-Date).ToUniversalTime().ToString("o")
 runner_process_id=$PID;expected_measured_attempts=20;file_sha256=$Hashes
}
$CompletePath=Join-Path $SessionDir "q3_session_complete.json"
[IO.File]::WriteAllText($CompletePath,($Complete|ConvertTo-Json -Depth 10),(New-Object Text.UTF8Encoding($false)))
Write-Host "";Write-Host "Q3 Session $Session complete: $SessionDir"
Write-Host "Do not rerun this session. Run the other session in a new PowerShell invocation, then runs/q/package_q3.ps1."
