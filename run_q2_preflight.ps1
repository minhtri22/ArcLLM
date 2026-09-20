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
$ExpectedQ1Archive="DFB3E86C4F51D06290A2AE1ED1479C96F35AE0D329CFA5E2B7D50289DC641B43"
$PinnedBaseline="b29c606e28a01b1bc8c1351026a0fa6e616bf6c4"
$PinnedRelease="v0.4.1"

$Head=(& git -C $Here rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0 -or -not $Head){throw "Q2 preflight cannot resolve HEAD"}
$ImplementationCritical=@(
 "src/q2_benchmark.cpp","src/p8c_segmented_access_correctness.cpp","src/gguf.cpp","src/gguf.h","src/tensor_store.cpp","src/tensor_store.h",
 "baseline/q2_llama_adapter.cpp","baseline/CMakeLists.txt",
 "tools/q2_resource_sampler.py","tools/q2_gpu_sampler.ps1","tools/summarize_q2.py",
 "tools/compile_q2_shaders.ps1","tools/build_q2.ps1","tools/qualify_q2_baseline.ps1",
 "run_q2_preflight.ps1","run_q2.ps1","tests/test_q2_package.py","config/q2_workloads.json",
 "docs/Q2_MATCHED_BENCHMARK_CONTRACT.md"
)
$GovernanceFiles=@("docs/Q2_IMPLEMENTATION.md","manifest.json","lineage.md",".github/workflows/q2-implementation-audit.yml")
$CleanFiles=@($ImplementationCritical+$GovernanceFiles)
& git -C $Here diff --quiet HEAD -- @CleanFiles
if($LASTEXITCODE -ne 0){throw "Q2 preflight blocked: critical working-tree files differ from HEAD"}
& git -C $Here diff --cached --quiet HEAD -- @CleanFiles
if($LASTEXITCODE -ne 0){throw "Q2 preflight blocked: critical index files differ from HEAD"}

# Fail-fast environment qualification before model hashing, builds, or baseline model loading.
$OS=Get-CimInstance Win32_OperatingSystem
$CPU=@(Get-CimInstance Win32_Processor|Select-Object Name,Manufacturer,NumberOfCores,NumberOfLogicalProcessors)
$GPU=@(Get-CimInstance Win32_VideoController|Select-Object Name,DriverVersion,AdapterRAM,VideoProcessor)
if(@($CPU|Where-Object {$_.Name -like "*Ultra 7 258V*"}).Count -lt 1){throw "Q2 preflight frozen CPU mismatch"}
$Arc=@($GPU|Where-Object {$_.Name -like "*Arc*140V*"})
if($Arc.Count -lt 1){throw "Q2 preflight Arc 140V not found"}
if(@($Arc|Where-Object {$_.DriverVersion -eq "32.0.101.8860"}).Count -lt 1){throw "Q2 preflight frozen Intel GPU driver mismatch"}
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

$Q1Archive=Join-Path $Here "inputs\q1_return_to_chatgpt.authoritative.zip"
if(-not(Test-Path $Q1Archive)){throw "Q2 Q1 archive missing"}
if((Get-FileHash $Q1Archive -Algorithm SHA256).Hash.ToUpperInvariant() -ne $ExpectedQ1Archive){throw "Q2 Q1 archive SHA mismatch"}

if(-not $ModelPath){
  $Resolved=@(& (Join-Path $Here "tools\resolve_p8_target.ps1") -OllamaModelsRoot $OllamaModelsRoot)
  if($Resolved.Count -lt 1){throw "Q2 preflight model resolver returned no path"}
  $ModelPath=[string]$Resolved[-1]
}
$mf=Get-Item $ModelPath
$ModelHash=(Get-FileHash $ModelPath -Algorithm SHA256).Hash.ToUpperInvariant()
if($mf.Length -ne $ExpectedModelBytes -or $ModelHash -ne $ExpectedModelHash){throw "Q2 preflight exact target mismatch"}

py -3 (Join-Path $Here "tests\test_q2_package.py")
if($LASTEXITCODE -ne 0){throw "Q2 preflight static QA failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\compile_q2_shaders.ps1")
if($LASTEXITCODE -ne 0){throw "Q2 preflight shader compile failed"}
powershell.exe -ExecutionPolicy Bypass -File (Join-Path $Here "tools\build_q2.ps1")
if($LASTEXITCODE -ne 0){throw "Q2 preflight ArcLLM native build failed"}

$QualArgs=@("-ExecutionPolicy","Bypass","-File",(Join-Path $Here "tools\qualify_q2_baseline.ps1"))
if($LlamaDir){$QualArgs+=@("-LlamaDir",$LlamaDir)}
if($BuildDir){$QualArgs+=@("-BuildDir",$BuildDir)}
& powershell.exe @QualArgs
if($LASTEXITCODE -ne 0){throw "Q2 preflight baseline build/API qualification failed"}

$BaselineExe=Join-Path $Here "artifacts\q2_baseline\q2_llama_adapter.exe"
$BuildQual=Join-Path $Results "q2_baseline_qualification.json"
if(-not(Test-Path $BaselineExe) -or -not(Test-Path $BuildQual)){throw "Q2 preflight baseline build artifacts missing"}
$BQ=Get-Content $BuildQual -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$BQ.commit -ne $PinnedBaseline -or [string]$BQ.release -ne $PinnedRelease -or [string]$BQ.qualification -ne "BUILD_API_QUALIFIED"){throw "Q2 preflight baseline build qualification mismatch"}
if([bool]$BQ.target_model_executed){throw "Q2 preflight build qualification unexpectedly executed target model"}
$BaselineExeHash=(Get-FileHash $BaselineExe -Algorithm SHA256).Hash.ToUpperInvariant()
if($BaselineExeHash -ne ([string]$BQ.adapter_sha256).ToUpperInvariant()){throw "Q2 preflight baseline executable SHA mismatch"}

$RuntimeRows=@()
foreach($W in @("W-S","W-C")){
  $Out=Join-Path $Results ("q2_baseline_runtime_qualification_"+($W -replace "-","_")+".json")
  & $BaselineExe --model $ModelPath --workload $W --qualify-only --out $Out
  $Code=$LASTEXITCODE
  if(-not(Test-Path $Out)){throw "Q2 baseline runtime qualification output missing for $W"}
  $Obj=Get-Content $Out -Raw -Encoding UTF8|ConvertFrom-Json
  if($Code -ne 0 -or [string]$Obj.status -ne "QUALIFIED"){throw "Q2 baseline runtime qualification failed for $W"}
  if([string]$Obj.baseline_commit -ne $PinnedBaseline -or [string]$Obj.baseline_release -ne $PinnedRelease){throw "Q2 baseline runtime qualification pin mismatch for $W"}
  if(-not[bool]$Obj.raw_token_input -or [bool]$Obj.tokenizer_used -or [bool]$Obj.chat_template_used){throw "Q2 baseline runtime raw-token semantics mismatch for $W"}
  if([bool]$Obj.decode_executed -or [int]$Obj.measured_attempts -ne 0){throw "Q2 preflight must execute zero decode / zero measured attempts"}
  if(-not[bool]$Obj.runtime.vulkan_log_present -or -not[bool]$Obj.runtime.full_offload){throw "Q2 baseline runtime is not Vulkan full-offload matched for $W"}
  if([int]$Obj.resolved.n_ctx -ne 4096 -or [int]$Obj.resolved.n_batch -ne 256 -or [int]$Obj.resolved.n_ubatch -ne 256 -or [string]$Obj.resolved.kv_k -ne "F32" -or [string]$Obj.resolved.kv_v -ne "F32"){throw "Q2 baseline resolved context mismatch for $W"}
  $ExpectedPrompt=if($W -eq "W-S"){4}else{256}
  $ExpectedPromptHash=if($W -eq "W-S"){"93833ffb49890aba"}else{"5973d0cfd8ad6313"}
  if([int]$Obj.prompt_tokens -ne $ExpectedPrompt){throw "Q2 baseline prompt length mismatch for $W"}
  if(([string]$Obj.prompt_hash_fnv1a64).ToLowerInvariant() -ne $ExpectedPromptHash){throw "Q2 baseline exact raw-token materialization mismatch for $W"}
  $RuntimeRows += [ordered]@{workload=$W;path=(Resolve-Path $Out).Path;sha256=(Get-FileHash $Out -Algorithm SHA256).Hash.ToUpperInvariant();status=$Obj.status;prompt_hash_fnv1a64=[string]$Obj.prompt_hash_fnv1a64;vulkan_log_present=[bool]$Obj.runtime.vulkan_log_present;full_offload=[bool]$Obj.runtime.full_offload;offloaded_layers=[int]$Obj.runtime.offloaded_layers;offloaded_layers_total=[int]$Obj.runtime.offloaded_layers_total;decode_executed=[bool]$Obj.decode_executed;measured_attempts=[int]$Obj.measured_attempts}
}

$CriticalHashes=[ordered]@{}
foreach($Rel in $ImplementationCritical){$P=Join-Path $Here $Rel;$CriticalHashes[$Rel]=(Get-FileHash $P -Algorithm SHA256).Hash.ToUpperInvariant()}
$ArcExe=Join-Path $Here "arcllm_q2.exe"
$ShaderProv=Join-Path $Results "q2_arcllm_shader_provenance.json"
if(-not(Test-Path $ArcExe) -or -not(Test-Path $ShaderProv)){throw "Q2 preflight ArcLLM build artifacts missing"}
$Prov=Get-Content $ShaderProv -Raw -Encoding UTF8|ConvertFrom-Json
if([string]$Prov.schema -ne "arcllm.q2.arcllm_shader_provenance.v1" -or [int]$Prov.shader_count -ne 16){throw "Q2 preflight shader provenance invalid"}
$ShaderSourceHashes=[ordered]@{};$CompiledShaderHashes=[ordered]@{}
foreach($Item in @($Prov.compiled)){
  $Src=Join-Path $Here ("shaders\"+[string]$Item.source)
  $Spv=Join-Path $Here ("compiled_shaders\"+[string]$Item.spv)
  if(-not(Test-Path $Src) -or -not(Test-Path $Spv)){throw "Q2 preflight shader artifact missing"}
  $SrcHash=(Get-FileHash $Src -Algorithm SHA256).Hash.ToUpperInvariant()
  $SpvHash=(Get-FileHash $Spv -Algorithm SHA256).Hash.ToUpperInvariant()
  if($SrcHash -ne ([string]$Item.source_sha256).ToUpperInvariant() -or $SpvHash -ne ([string]$Item.spv_sha256).ToUpperInvariant()){throw "Q2 preflight shader provenance/hash mismatch"}
  $ShaderSourceHashes[[string]$Item.source]=$SrcHash
  $CompiledShaderHashes[[string]$Item.spv]=$SpvHash
}
$Lock=[ordered]@{
 schema="arcllm.q2.preflight_lock.v1"
 implementation_commit=$Head
 created_utc=(Get-Date).ToUniversalTime().ToString("o")
 target_sha256=$ModelHash
 target_size_bytes=$mf.Length
 q1_archive_sha256=$ExpectedQ1Archive
 baseline_release=$PinnedRelease
 baseline_commit=$PinnedBaseline
 baseline_exe_sha256=$BaselineExeHash
 arcllm_exe_sha256=(Get-FileHash $ArcExe -Algorithm SHA256).Hash.ToUpperInvariant()
 shader_provenance_sha256=(Get-FileHash $ShaderProv -Algorithm SHA256).Hash.ToUpperInvariant()
 shader_source_sha256=$ShaderSourceHashes
 compiled_shader_sha256=$CompiledShaderHashes
 static_qa="PASS"
 arcllm_build="PASS"
 baseline_build_api="PASS"
 baseline_runtime_qualification="PASS"
 runtime_qualifications=$RuntimeRows
 hardware=[ordered]@{os_caption=$OS.Caption;os_version=$OS.Version;os_build=$OS.BuildNumber;cpu=$CPU;gpu=$GPU;power_scheme=$PowerScheme;system_power=$SystemPowerEvidence;battery_device_status=$Battery;ac_power_online=$AcPowerOnline}
 critical_file_sha256=$CriticalHashes
 measurement_authorization_required=$true
 measurement_authorized=$false
 measurements_executed=$false
 measured_attempts=0
 q3_started=$false
}
$LockPath=Join-Path $Results "q2_preflight_lock.json"
[IO.File]::WriteAllText($LockPath,($Lock|ConvertTo-Json -Depth 12),(New-Object Text.UTF8Encoding($false)))
$Bundle=Join-Path $Results "q2_preflight_return_to_chatgpt.zip"
if(Test-Path $Bundle){Remove-Item -Force $Bundle}
Compress-Archive -Path @($LockPath,$BuildQual,(Join-Path $Results "q2_baseline_runtime_qualification_W_S.json"),(Join-Path $Results "q2_baseline_runtime_qualification_W_C.json"),$ShaderProv) -DestinationPath $Bundle -Force
Write-Host "Q2 PRE-FLIGHT PASS / ZERO MEASURED ATTEMPTS"
Write-Host "  lock=$LockPath"
Write-Host "  bundle=$Bundle"
