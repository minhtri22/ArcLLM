param(
  [string]$ModelPath,
  [string]$OllamaModelsRoot,
  [string]$BaselineExe,
  [string]$BaselineQualification,
  [string]$SessionDir
)
$ErrorActionPreference="Stop";Set-StrictMode -Version Latest
$Here=Split-Path -Parent $MyInvocation.MyCommand.Path
$ResultsRoot=Join-Path $Here "results";New-Item -ItemType Directory -Force -Path $ResultsRoot|Out-Null
if(-not $SessionDir){$SessionDir=Join-Path $ResultsRoot ("q2_"+(Get-Date).ToUniversalTime().ToString("yyyyMMdd-HHmmss"))}
New-Item -ItemType Directory -Force -Path $SessionDir|Out-Null

$ExpectedModelHash="60E05F2100071479F596B964F89F510F057CE397EA22F2833A0CFE029BFC2463"
$ExpectedModelBytes=[int64]4683074048
$ExpectedQ1Archive="DFB3E86C4F51D06290A2AE1ED1479C96F35AE0D329CFA5E2B7D50289DC641B43"
$BaselineCommit="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
$BaselineRelease="v0.4.1"

$Head=(& git -C $Here rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0 -or -not $Head){throw "Q2 cannot resolve ArcLLM implementation commit"}
$PreflightLockPath=Join-Path $ResultsRoot "q2_preflight_lock.json"
if(-not(Test-Path $PreflightLockPath)){throw "Q2 measurement blocked: q2_preflight_lock.json missing"}
$PF=Get-Content $PreflightLockPath -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$PF.schema -ne "arcllm.q2.preflight_lock.v1"){throw "Q2 measurement blocked: invalid preflight schema"}
$ImplementationCommit=[string]$PF.implementation_commit
& git -C $Here merge-base --is-ancestor $ImplementationCommit $Head
if($LASTEXITCODE -ne 0){throw "Q2 measurement blocked: preflight implementation commit is not an ancestor of current HEAD"}
if([string]$PF.static_qa -ne "PASS" -or [string]$PF.arcllm_build -ne "PASS" -or [string]$PF.baseline_build_api -ne "PASS" -or [string]$PF.baseline_runtime_qualification -ne "PASS"){throw "Q2 measurement blocked: preflight qualification incomplete"}
if([bool]$PF.measurements_executed -or [int]$PF.measured_attempts -ne 0 -or [bool]$PF.q3_started){throw "Q2 measurement blocked: preflight lock is not zero-measurement clean"}

$AuthorizationPath=Join-Path $Here "config\q2_execution_authorization.json"
if(-not(Test-Path $AuthorizationPath)){throw "Q2 measurement blocked: q2_execution_authorization.json missing; independent preflight adjudication/final execution-lock commit required"}
& git -C $Here ls-files --error-unmatch -- "config/q2_execution_authorization.json" *> $null
if($LASTEXITCODE -ne 0){throw "Q2 measurement blocked: execution authorization is not committed"}
& git -C $Here diff --quiet HEAD -- "config/q2_execution_authorization.json"
if($LASTEXITCODE -ne 0){throw "Q2 measurement blocked: execution authorization differs from committed HEAD"}
$Auth=Get-Content $AuthorizationPath -Raw -Encoding UTF8|ConvertFrom-Json
$PreflightLockHash=(Get-FileHash $PreflightLockPath -Algorithm SHA256).Hash.ToUpperInvariant()
if([string]$Auth.schema -ne "arcllm.q2.execution_authorization.v1" -or -not[bool]$Auth.authorized -or [string]$Auth.decision -ne "Q2_MEASUREMENT_AUTHORIZED"){throw "Q2 measurement blocked: invalid final execution authorization"}
if([string]$Auth.preflight_implementation_commit -ne $ImplementationCommit -or ([string]$Auth.preflight_lock_sha256).ToUpperInvariant() -ne $PreflightLockHash){throw "Q2 measurement blocked: authorization does not bind this preflight"}
if([bool]$Auth.measurements_executed -or [int]$Auth.measured_attempts -ne 0 -or [bool]$Auth.q3_started){throw "Q2 measurement blocked: authorization is not zero-measurement clean"}

$Manifest=Get-Content (Join-Path $Here "manifest.json") -Raw -Encoding UTF8|ConvertFrom-Json
if(-not[bool]$Manifest.target_run_permitted -or -not[bool]$Manifest.q2.target_run_permitted -or -not[bool]$Manifest.q2.measurement_run_permitted -or [bool]$Manifest.q3_permitted){throw "Q2 measurement blocked: manifest final execution authorization is not open"}
if(-not[bool]$Manifest.q2.execution_authorization_committed -or [string]$Manifest.status -ne "Q2_MEASUREMENT_AUTHORIZED" -or [string]$Manifest.q2.status -ne "Q2_MEASUREMENT_AUTHORIZED"){throw "Q2 measurement blocked: manifest authorization state is incomplete"}

$ImplementationCritical=@(
 "src/q2_benchmark.cpp","src/p8c_segmented_access_correctness.cpp","src/gguf.cpp","src/gguf.h","src/tensor_store.cpp","src/tensor_store.h",
 "baseline/q2_llama_adapter.cpp","baseline/CMakeLists.txt",
 "tools/q2_resource_sampler.py","tools/q2_gpu_sampler.ps1","tools/summarize_q2.py",
 "tools/compile_q2_shaders.ps1","tools/build_q2.ps1","tools/qualify_q2_baseline.ps1",
 "run_q2_preflight.ps1","run_q2.ps1","tests/test_q2_package.py","config/q2_workloads.json",
 "docs/Q2_MATCHED_BENCHMARK_CONTRACT.md"
)
& git -C $Here diff --quiet HEAD -- @ImplementationCritical
if($LASTEXITCODE -ne 0){throw "Q2 implementation-critical working-tree files differ from HEAD"}
& git -C $Here diff --cached --quiet HEAD -- @ImplementationCritical
if($LASTEXITCODE -ne 0){throw "Q2 implementation-critical index files differ from HEAD"}
foreach($P in $PF.critical_file_sha256.PSObject.Properties){
  $Path=Join-Path $Here $P.Name
  if(-not(Test-Path $Path)){throw "Q2 measurement blocked: preflight critical file missing: $($P.Name)"}
  $Now=(Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant()
  if($Now -ne ([string]$P.Value).ToUpperInvariant()){throw "Q2 measurement blocked: critical file drift since preflight: $($P.Name)"}
  $AuthProp=$Auth.critical_file_sha256.PSObject.Properties[$P.Name]
  if($null -eq $AuthProp -or ([string]$AuthProp.Value).ToUpperInvariant() -ne ([string]$P.Value).ToUpperInvariant()){throw "Q2 measurement blocked: authorization critical-file binding mismatch: $($P.Name)"}
}

$Q1Archive=Join-Path $Here "inputs\q1_return_to_chatgpt.authoritative.zip"
if(-not(Test-Path $Q1Archive)){throw "Q2 authoritative Q1 archive missing"}
if((Get-FileHash $Q1Archive -Algorithm SHA256).Hash.ToUpperInvariant() -ne $ExpectedQ1Archive){throw "Q2 Q1 archive SHA mismatch"}

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "Q2 model resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$mf=Get-Item $ModelPath
if($mf.Length -ne $ExpectedModelBytes){throw "Q2 model size mismatch"}
$ModelHash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($ModelHash -ne $ExpectedModelHash){throw "Q2 model SHA mismatch"}
if($ModelHash -ne ([string]$PF.target_sha256).ToUpperInvariant() -or $mf.Length -ne [int64]$PF.target_size_bytes){throw "Q2 measurement blocked: target differs from preflight"}
if($ModelHash -ne ([string]$Auth.target_sha256).ToUpperInvariant() -or $mf.Length -ne [int64]$Auth.target_size_bytes){throw "Q2 measurement blocked: target differs from final authorization"}

if(-not $BaselineExe){$BaselineExe=Join-Path $Here "artifacts\q2_baseline\q2_llama_adapter.exe"}
if(-not $BaselineQualification){$BaselineQualification=Join-Path $Here "results\q2_baseline_qualification.json"}
if(-not(Test-Path $BaselineExe)){throw "Q2 qualified baseline executable missing"}
if(-not(Test-Path $BaselineQualification)){throw "Q2 baseline qualification JSON missing"}
$BQ=Get-Content $BaselineQualification -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$BQ.commit -ne $BaselineCommit -or [string]$BQ.release -ne $BaselineRelease -or [string]$BQ.qualification -ne "BUILD_API_QUALIFIED"){throw "Q2 baseline qualification pin mismatch"}
if(-not [bool]$BQ.raw_token_adapter -or [bool]$BQ.tokenizer_path_used -or [bool]$BQ.target_model_executed){throw "Q2 baseline qualification semantics mismatch"}
$BaselineExeHash=(Get-FileHash $BaselineExe -Algorithm SHA256).Hash.ToUpperInvariant()
if($BaselineExeHash -ne ([string]$BQ.adapter_sha256).ToUpperInvariant()){throw "Q2 baseline executable SHA mismatch"}
if($BaselineExeHash -ne ([string]$PF.baseline_exe_sha256).ToUpperInvariant()){throw "Q2 measurement blocked: baseline executable differs from preflight"}
if($BaselineExeHash -ne ([string]$Auth.baseline_exe_sha256).ToUpperInvariant()){throw "Q2 measurement blocked: baseline executable differs from final authorization"}
if([string]$PF.baseline_commit -ne $BaselineCommit -or [string]$PF.baseline_release -ne $BaselineRelease){throw "Q2 measurement blocked: baseline pin differs from preflight"}
if([string]$Auth.baseline_commit -ne $BaselineCommit -or [string]$Auth.baseline_release -ne $BaselineRelease){throw "Q2 measurement blocked: baseline pin differs from final authorization"}
$RuntimeQualifications=@($PF.runtime_qualifications)
if($RuntimeQualifications.Count -ne 2 -or @($RuntimeQualifications|ForEach-Object {$_.workload}|Sort-Object -Unique).Count -ne 2){throw "Q2 measurement blocked: baseline runtime qualification set incomplete"}
foreach($RQ in $RuntimeQualifications){
  if([string]$RQ.status -ne "QUALIFIED" -or -not[bool]$RQ.full_offload -or [bool]$RQ.decode_executed -or [int]$RQ.measured_attempts -ne 0){throw "Q2 measurement blocked: baseline runtime qualification invalid"}
}

py -3 (Join-Path $Here "tests\test_q2_package.py")
if($LASTEXITCODE -ne 0){throw "Q2 static contract failed"}

$ArcExe=Join-Path $Here "arcllm_q2.exe"
$ShaderProv=Join-Path $ResultsRoot "q2_arcllm_shader_provenance.json"
if(-not(Test-Path $ArcExe) -or -not(Test-Path $ShaderProv)){throw "Q2 measurement blocked: preflight-built ArcLLM artifacts missing"}
$ArcExeHash=(Get-FileHash $ArcExe -Algorithm SHA256).Hash.ToUpperInvariant()
$ShaderProvHash=(Get-FileHash $ShaderProv -Algorithm SHA256).Hash.ToUpperInvariant()
if($ArcExeHash -ne ([string]$PF.arcllm_exe_sha256).ToUpperInvariant() -or $ArcExeHash -ne ([string]$Auth.arcllm_exe_sha256).ToUpperInvariant()){throw "Q2 measurement blocked: ArcLLM executable differs from qualified/authorized artifact"}
if($ShaderProvHash -ne ([string]$PF.shader_provenance_sha256).ToUpperInvariant() -or $ShaderProvHash -ne ([string]$Auth.shader_provenance_sha256).ToUpperInvariant()){throw "Q2 measurement blocked: shader provenance differs from qualified/authorized artifact"}
foreach($P in $PF.shader_source_sha256.PSObject.Properties){
  $Path=Join-Path $Here ("shaders\"+$P.Name)
  if(-not(Test-Path $Path) -or (Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant() -ne ([string]$P.Value).ToUpperInvariant()){throw "Q2 measurement blocked: shader source drift since preflight: $($P.Name)"}
}
foreach($P in $PF.compiled_shader_sha256.PSObject.Properties){
  $Path=Join-Path $Here ("compiled_shaders\"+$P.Name)
  if(-not(Test-Path $Path) -or (Get-FileHash $Path -Algorithm SHA256).Hash.ToUpperInvariant() -ne ([string]$P.Value).ToUpperInvariant()){throw "Q2 measurement blocked: compiled shader drift since preflight: $($P.Name)"}
}
if((ConvertTo-Json $Auth.shader_source_sha256 -Compress) -ne (ConvertTo-Json $PF.shader_source_sha256 -Compress) -or (ConvertTo-Json $Auth.compiled_shader_sha256 -Compress) -ne (ConvertTo-Json $PF.compiled_shader_sha256 -Compress)){throw "Q2 measurement blocked: final authorization shader binding mismatch"}

$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
$GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
$TargetGPU=@($GPU|Where-Object {$_.Name -like "*Arc*140V*"})
if($TargetGPU.Count -lt 1){throw "Q2 target Arc 140V GPU not found"}
if(@($TargetGPU|Where-Object {$_.DriverVersion -eq "32.0.101.8860"}).Count -lt 1){throw "Q2 frozen Intel GPU driver mismatch"}
if(@($CPU|Where-Object {$_.Name -like "*Ultra 7 258V*"}).Count -lt 1){throw "Q2 frozen CPU mismatch"}
$PowerScheme=((powercfg /GETACTIVESCHEME 2>&1)|Out-String).Trim()
$PowerStatusType=@"
using System;
using System.Runtime.InteropServices;
public static class ArcLlmQ2Power {
  [StructLayout(LayoutKind.Sequential)]
  public struct SYSTEM_POWER_STATUS {
    public byte ACLineStatus;
    public byte BatteryFlag;
    public byte BatteryLifePercent;
    public byte SystemStatusFlag;
    public uint BatteryLifeTime;
    public uint BatteryFullLifeTime;
  }
  [DllImport("kernel32.dll", SetLastError=true)]
  [return: MarshalAs(UnmanagedType.Bool)]
  public static extern bool GetSystemPowerStatus(out SYSTEM_POWER_STATUS s);
}
"@
if(-not ("ArcLlmQ2Power" -as [type])){Add-Type -TypeDefinition $PowerStatusType}
$SystemPower=New-Object "ArcLlmQ2Power+SYSTEM_POWER_STATUS"
if(-not [ArcLlmQ2Power]::GetSystemPowerStatus([ref]$SystemPower)){throw "Q2 cannot query system AC power status"}
$AcLineStatus=[int]$SystemPower.ACLineStatus
if($AcLineStatus -eq 0){throw "Q2 requires AC power: GetSystemPowerStatus reports Offline"}
if($AcLineStatus -eq 255){throw "Q2 requires known AC power state: GetSystemPowerStatus reports Unknown"}
if($AcLineStatus -ne 1){throw "Q2 invalid ACLineStatus=$AcLineStatus"}
$AcPowerOnline=$true
$SystemPowerEvidence=[ordered]@{
  source="GetSystemPowerStatus"
  ac_line_status=$AcLineStatus
  ac_line_status_text="Online"
  battery_flag=[int]$SystemPower.BatteryFlag
  battery_life_percent=[int]$SystemPower.BatteryLifePercent
  system_status_flag=[int]$SystemPower.SystemStatusFlag
  battery_life_time=[uint32]$SystemPower.BatteryLifeTime
  battery_full_life_time=[uint32]$SystemPower.BatteryFullLifeTime
}
$Battery=@()
try{$Battery=@(Get-CimInstance -Namespace root\wmi -Class BatteryStatus -ErrorAction Stop|Select-Object PowerOnline,Charging,Discharging,RemainingCapacity)}catch{}
if([string]$OS.Version -ne [string]$PF.hardware.os_version -or [string]$OS.BuildNumber -ne [string]$PF.hardware.os_build){throw "Q2 measurement blocked: OS version/build drift since preflight"}
if($PowerScheme -ne [string]$PF.hardware.power_scheme){throw "Q2 measurement blocked: active power scheme drift since preflight"}
if($null -ne $PF.hardware.ac_power_online -and -not[bool]$PF.hardware.ac_power_online){throw "Q2 measurement blocked: preflight did not establish AC power"}

$Workloads=Join-Path $Here "config\q2_workloads.json"
$WorkloadsHash=(Get-FileHash $Workloads -Algorithm SHA256).Hash.ToUpperInvariant()
$Env=[ordered]@{
 schema="arcllm.q2.environment.v1"
 captured_utc=(Get-Date).ToUniversalTime().ToString("o")
 implementation_commit=$ImplementationCommit
 execution_authorization_commit=$Head
 preflight_lock_sha256=$PreflightLockHash
 os=[ordered]@{caption=$OS.Caption;version=$OS.Version;build_number=$OS.BuildNumber;architecture=$OS.OSArchitecture}
 cpu=$CPU
 gpu=$GPU
 power_scheme=$PowerScheme
 system_power=$SystemPowerEvidence
 battery_device_status=$Battery
 ac_power_online=$AcPowerOnline
 model_path=$ModelPath
 model_sha256=$ModelHash
 model_size_bytes=$mf.Length
 q1_archive_sha256=$ExpectedQ1Archive
 q2_workloads_sha256=$WorkloadsHash
 baseline=[ordered]@{release=$BaselineRelease;commit=$BaselineCommit;adapter_sha256=$BaselineExeHash;qualification_path=(Resolve-Path $BaselineQualification).Path}
 llm_process_snapshot=@(Get-Process -ErrorAction SilentlyContinue|Where-Object {$_.ProcessName -match "ollama|llama|arcllm"}|Select-Object ProcessName,Id,CPU,WorkingSet64)
}
$EnvPath=Join-Path $SessionDir "q2_environment.json"
[IO.File]::WriteAllText($EnvPath,($Env|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))

$Sampler=Join-Path $Here "tools\q2_resource_sampler.py"
$GpuSampler=Join-Path $Here "tools\q2_gpu_sampler.ps1"

function Invoke-Q2Cell([string]$Key,[string]$System,[string]$Workload,[string]$Exe,[string[]]$ChildArgs){
  $Result=Join-Path $SessionDir ("q2_"+$Key+"_result.json")
  $Trace=Join-Path $SessionDir ("q2_"+$Key+"_resources.json")
  $Log=Join-Path $SessionDir ("q2_"+$Key+".log")
  $Args=@("-3",$Sampler,"--system",$System,"--workload",$Workload,"--trace",$Trace,"--log",$Log,"--gpu-script",$GpuSampler,"--sample-ms","100","--",$Exe)
  $Args += $ChildArgs
  $Args += @("--workload",$Workload,"--warmups","1","--measured","5","--out",$Result)
  Write-Host ""
  Write-Host "Q2 cell $Key : $System $Workload"
  & py @Args
  $Code=$LASTEXITCODE
  if(-not(Test-Path $Trace)){throw "Q2 resource trace missing for $Key"}
  if(-not(Test-Path $Result)){
    $Fail=[ordered]@{schema="arcllm.q2.cell_failure.v1";system=$System;workload=$Workload;process_exit_code=$Code;error="child result JSON missing"}
    [IO.File]::WriteAllText($Result,($Fail|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
  }
  return $Code
}

$CellExit=[ordered]@{}
# Frozen order: W-S Arc -> baseline; W-C baseline -> Arc.
$CellExit.arcllm_ws=Invoke-Q2Cell "arcllm_ws" "ArcLLM" "W-S" $ArcExe @("--model",$ModelPath,"--shader-dir",(Join-Path $Here "compiled_shaders"),"--implementation-commit",$ImplementationCommit)
$CellExit.baseline_ws=Invoke-Q2Cell "baseline_ws" "llama.cpp" "W-S" $BaselineExe @("--model",$ModelPath)
$CellExit.baseline_wc=Invoke-Q2Cell "baseline_wc" "llama.cpp" "W-C" $BaselineExe @("--model",$ModelPath)
$CellExit.arcllm_wc=Invoke-Q2Cell "arcllm_wc" "ArcLLM" "W-C" $ArcExe @("--model",$ModelPath,"--shader-dir",(Join-Path $Here "compiled_shaders"),"--implementation-commit",$ImplementationCommit)

$QualCopy=Join-Path $SessionDir "q2_baseline_qualification.json"
Copy-Item -Force $BaselineQualification $QualCopy
$ShaderProvCopy=Join-Path $SessionDir "q2_arcllm_shader_provenance.json"
Copy-Item -Force $ShaderProv $ShaderProvCopy
$PreflightCopy=Join-Path $SessionDir "q2_preflight_lock.json"
$AuthorizationCopy=Join-Path $SessionDir "q2_execution_authorization.json"
Copy-Item -Force $PreflightLockPath $PreflightCopy
Copy-Item -Force $AuthorizationPath $AuthorizationCopy
foreach($N in @("q2_baseline_runtime_qualification_W_S.json","q2_baseline_runtime_qualification_W_C.json")){
  $Src=Join-Path $ResultsRoot $N
  if(-not(Test-Path $Src)){throw "Q2 runtime qualification evidence missing at measurement packaging: $N"}
  Copy-Item -Force $Src (Join-Path $SessionDir $N)
}

$RunMeta=[ordered]@{
 schema="arcllm.q2.run_meta.v1"
 implementation_commit=$ImplementationCommit
 execution_authorization_commit=$Head
 preflight_lock_sha256=$PreflightLockHash
 execution_order=@("arcllm_ws","baseline_ws","baseline_wc","arcllm_wc")
 cell_exit_codes=$CellExit
 warmups_per_cell=1
 measured_attempts_per_cell=5
 expected_measured_attempts=20
 advantage_adjudicated=$false
 q3_started=$false
}
[IO.File]::WriteAllText((Join-Path $SessionDir "q2_run_meta.json"),($RunMeta|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))

py -3 (Join-Path $Here "tools\summarize_q2.py") --results-dir $SessionDir --contract (Join-Path $Here "docs\Q2_MATCHED_BENCHMARK_CONTRACT.md") --workloads $Workloads --environment $EnvPath --baseline-qualification $QualCopy --preflight-lock $PreflightCopy --execution-authorization $AuthorizationCopy --implementation-commit $ImplementationCommit --model-sha256 $ModelHash
if($LASTEXITCODE -ne 0){throw "Q2 summarizer failed"}

$Bundle=Join-Path $SessionDir "q2_return_to_chatgpt.zip"
$Pack=@(Get-ChildItem -File $SessionDir|Where-Object {$_.Name -ne "q2_return_to_chatgpt.zip"}|Select-Object -ExpandProperty FullName)
Compress-Archive -Path $Pack -DestinationPath $Bundle -Force
Write-Host ""
Write-Host "Q2 session: $SessionDir"
Write-Host "Return bundle: $Bundle"
Write-Host "Q2 remains characterization-only; no Q3 decision is made by this runner."
